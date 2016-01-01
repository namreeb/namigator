#define NOMINMAX

#include <memory>
#include <windows.h>
#include <windowsx.h>
#include <sstream>
#include <cassert>

#include "resource.h"

#include "Renderer.hpp"
#include "CommonControl.hpp"
#include "parser.hpp"
#include "Output/Continent.hpp"

#define START_X             100
#define START_Y             100
#define START_WIDTH         1200
#define START_HEIGHT        800

#define CONTROL_WIDTH       300
#define CONTROL_HEIGHT      250

#define CAMERA_STEP         2.f

// FIXME: Amount to shift control window leftwards.  Find out proper solution for this later!
#define MAGIC_LEFT_SHIFT    15

HWND gGuiWindow, gControlWindow;

std::unique_ptr<Renderer> gRenderer;
std::unique_ptr<CommonControl> gControls;
std::unique_ptr<::parser::Continent> gContinent;

int gMovingUp = 0;
int gMovingVertical = 0;
int gMovingRight = 0;
int gMovingForward = 0;

// this is the main message handler for the program
LRESULT CALLBACK GuiWindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
        case WM_CLOSE:
        case WM_DESTROY:
        {
            PostQuitMessage(0);
            return TRUE;
        }

        case WM_MOVING:
        {
            const RECT *rect = (const RECT *)lParam;

            MoveWindow(gControlWindow, rect->right - MAGIC_LEFT_SHIFT, rect->top, CONTROL_WIDTH, CONTROL_HEIGHT, FALSE);

            return TRUE;
        }

        case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
                case ID_FILE_EXIT:
                    PostQuitMessage(0);
                    return TRUE;
            }

            break;
        }

        case WM_KEYDOWN:
        {
            switch (static_cast<char>(wParam))
            {
                case ' ':
                    gMovingVertical = +1;
                    return TRUE;
                case 'X':
                    gMovingVertical = -1;
                    return TRUE;
                case 'Q':
                    gMovingUp = +1;
                    return TRUE;
                case 'E':
                    gMovingUp = -1;
                    return TRUE;
                case 'D':
                    gMovingRight = +1;
                    return TRUE;
                case 'A':
                    gMovingRight = -1;
                    return TRUE;
                case 'W':
                    gMovingForward = +1;
                    return TRUE;
                case 'S':
                    gMovingForward = -1;
                    return TRUE;
            }
                
            break;
        }

        case WM_KEYUP:
        {
            switch (static_cast<char>(wParam))
            {
            case ' ':
            case 'X':
                gMovingVertical = 0;
                return TRUE;
            case 'Q':
            case 'E':
                gMovingUp = 0;
                return TRUE;
            case 'D':
            case 'A':
                gMovingRight = 0;
                return TRUE;
            case 'W':
            case 'S':
                gMovingForward = 0;
                return TRUE;
            }
            break;
        }

        case WM_MOUSEWHEEL:
        {
            const short distance = static_cast<short>(wParam >> 16);

            gRenderer->m_camera.MoveIn(0.1f * distance);

            return TRUE;
        }

        case WM_RBUTTONDOWN:
        {
            POINT point;
            GetCursorPos(&point);

            gRenderer->m_camera.BeginMousePan(point.x, point.y);
            ShowCursor(FALSE);

            return TRUE;
        }

        case WM_MOUSEMOVE:
        {
            if (!!(wParam & MK_RBUTTON) && gRenderer->m_camera.IsMousePanning())
            {
                POINT point;
                GetCursorPos(&point);

                int x, y;
                gRenderer->m_camera.GetMousePanStart(x, y);

                // only if there is movement to avoid an infinite loop of window messages
                if (x != point.x || y != point.y)
                {
                    gRenderer->m_camera.UpdateMousePan(point.x, point.y);

                    SetCursorPos(x, y);
                }

                return TRUE;
            }

            break;
        }

        case WM_RBUTTONUP:
        {
            if (gRenderer->m_camera.IsMousePanning())
            {
                gRenderer->m_camera.EndMousePan();
                
                ShowCursor(TRUE);

                return TRUE;
            }

            break;
        }
    }

    return DefWindowProc(hWnd, message, wParam, lParam);
}

enum Buttons : int
{
    LoadPosition,
    LoadADT
};

LRESULT CALLBACK ControlWindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
        case WM_COMMAND:
        {
            switch (wParam)
            {
                case Buttons::LoadPosition:
                {
                    MessageBox(hWnd, L"Not implemented yet.", L"DEBUG", 0);
                    
                    return TRUE;
                }

                case Buttons::LoadADT:
                {
                    auto const adtX = std::stoi(gControls->GetText(L"ADT_X"));
                    auto const adtY = std::stoi(gControls->GetText(L"ADT_Y"));
                    auto const continentChoice = gControls->GetText(L"CONTINENT");
                    auto const continentName = continentChoice.substr(continentChoice.find(' ') + 1);

                    // if we are switching continents, unload our buffers
                    if (gContinent && gContinent->Name != continentName)
                        gRenderer->ClearBuffers();
                    // if we are loading the first continent, intialize it first
                    else if (!gContinent)
                        gContinent.reset(new ::parser::Continent(continentName));

                    auto const adt = gContinent->LoadAdt(adtX, adtY);
                    
                    for (int x = 0; x < 16; ++x)
                        for (int y = 0; y < 16; ++y)
                        {
                            auto const chunk = adt->GetChunk(x, y);

                            gRenderer->AddTerrain(chunk->m_terrainVertices, chunk->m_terrainIndices);
                            gRenderer->AddLiquid(chunk->m_liquidVertices, chunk->m_liquidIndices);

                            for (auto &d : chunk->m_doodads)
                            {
                                auto const doodad = gContinent->GetDoodad(d);

                                assert(doodad);

                                gRenderer->AddDoodad(d, doodad->Vertices, doodad->Indices);
                            }

                            for (auto &w : chunk->m_wmos)
                            {
                                auto const wmo = gContinent->GetWmo(w);

                                assert(wmo);

                                gRenderer->AddWmo(w, wmo->Vertices, wmo->Indices);
                                gRenderer->AddDoodad(w, wmo->DoodadVertices, wmo->DoodadIndices);
                                gRenderer->AddLiquid(wmo->LiquidVertices, wmo->LiquidIndices);
                            }
                        }

                    float cx = (adt->MaxX + adt->MinX) / 2.f;
                    float cy = (adt->MaxY + adt->MinY) / 2.f;
                    float cz = (adt->MaxZ + adt->MinZ) / 2.f;

                    gRenderer->m_camera.Move(cx + 300.f, cy + 300.f, cz + 300.f);
                    gRenderer->m_camera.LookAt(cx, cy, cz);

                    return TRUE;
                }
            }
        }
    }

    return DefWindowProc(hWnd, message, wParam, lParam);
}

void InitializeWindows(HINSTANCE hInstance, HWND &guiWindow, HWND &controlWindow)
{
    WNDCLASSEX wc;

    ZeroMemory(&wc, sizeof(WNDCLASSEX));

    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = GuiWindowProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)COLOR_WINDOW;
    wc.lpszClassName = L"DXWindow";
    wc.hIconSm = (HICON)LoadImage(GetModuleHandle(nullptr), MAKEINTRESOURCE(IDI_APPICON), IMAGE_ICON, 16, 16, 0);
    wc.hIcon = (HICON)LoadImage(GetModuleHandle(nullptr), MAKEINTRESOURCE(IDI_APPICON), IMAGE_ICON, 32, 32, 0);
    wc.lpszMenuName = MAKEINTRESOURCE(IDR_MENU1);

    RegisterClassEx(&wc);

    RECT wr = { START_X, START_Y, START_X + START_WIDTH, START_Y + START_HEIGHT };
    AdjustWindowRect(&wr, WS_OVERLAPPEDWINDOW, true);

    guiWindow = CreateWindowEx(WS_EX_RIGHTSCROLLBAR,
        L"DXWindow",
        L"CMaNGOS Map Debugging Interface",
        WS_OVERLAPPEDWINDOW,
        wr.left,
        wr.top,
        wr.right - wr.left,
        wr.bottom - wr.top,
        HWND_DESKTOP,
        nullptr,
        hInstance,
        nullptr);

    ZeroMemory(&wc, sizeof(WNDCLASSEX));

    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = ControlWindowProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)COLOR_WINDOW;
    wc.lpszClassName = L"ControlWindow";
    wc.hIconSm = (HICON)LoadImage(GetModuleHandle(nullptr), MAKEINTRESOURCE(IDI_APPICON), IMAGE_ICON, 16, 16, 0);
    wc.hIcon = (HICON)LoadImage(GetModuleHandle(nullptr), MAKEINTRESOURCE(IDI_APPICON), IMAGE_ICON, 32, 32, 0);

    RegisterClassEx(&wc);

    controlWindow = CreateWindowEx(WS_EX_RIGHTSCROLLBAR,
        L"ControlWindow",
        L"Control",
        (WS_BORDER | WS_CAPTION) & (~WS_ICONIC),
        wr.right - MAGIC_LEFT_SHIFT,
        wr.top,
        CONTROL_WIDTH,
        CONTROL_HEIGHT,
        HWND_DESKTOP,
        nullptr,
        hInstance,
        nullptr);
}

// the entry point for any Windows program
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    ::parser::Parser::Initialize();

    InitializeWindows(hInstance, gGuiWindow, gControlWindow);

    ShowWindow(gGuiWindow, nCmdShow);
    ShowWindow(gControlWindow, nCmdShow);

    // set up and initialize Direct3D
    
    gRenderer.reset(new Renderer(gGuiWindow));

    // set up and initialize our Windows common control API for the control window
    gControls.reset(new CommonControl(gControlWindow));

    gControls->AddLabel(L"Select Continent:", 10, 12, 100, 20);

    std::vector<std::wstring> continents;
    continents.push_back(L"000 Azeroth");
    continents.push_back(L"001 Kalimdor");
    continents.push_back(L"013 Test");
    continents.push_back(L"025 Scott test");
    continents.push_back(L"029 Test");
    continents.push_back(L"030 PVPzone01 (Alterac Valley)");
    continents.push_back(L"033 Shadowfang");
    continents.push_back(L"034 stormwindjail");
    continents.push_back(L"035 stormwindprison");
    continents.push_back(L"036 deadminesInstance");
    continents.push_back(L"037 PVPZone02 (Azshara Crater)");
    continents.push_back(L"043 WailingCaverns");
    continents.push_back(L"489 PVPzone03 (Warsong Gulch)");
    continents.push_back(L"529 PVPzone04 (Arathi Basin)");
    continents.push_back(L"530 expansion01 (Outland");
    continents.push_back(L"571 Northrend");

    gControls->AddComboBox(L"CONTINENT", continents, 115, 10, 155, 150);

    gControls->AddLabel(L"Position:", 10, 10, 300, 20);

    gControls->AddLabel(L"X:", 10, 47, 20, 20);
    gControls->AddTextBox(L"X", L"", 25, 45, 75, 20);

    gControls->AddLabel(L"Y:", 10, 72, 20, 20);
    gControls->AddTextBox(L"Y", L"", 25, 70, 75, 20);

    gControls->AddLabel(L"Z:", 10, 97, 20, 20);
    gControls->AddTextBox(L"Z", L"", 25, 95, 75, 20);

    gControls->AddButton(L"Load Position", Buttons::LoadPosition, 115, 92, 100, 25);

    gControls->AddLabel(L"X:", 10, 147, 20, 20);
    gControls->AddTextBox(L"ADT_X", L"30", 25, 145, 75, 20);

    gControls->AddLabel(L"Y:", 10, 172, 20, 20);
    gControls->AddTextBox(L"ADT_Y", L"48", 25, 170, 75, 20);

    gControls->AddButton(L"Load ADT", Buttons::LoadADT, 115, 167, 100, 25);

    // enter the main loop:

    MSG msg;

    while (true)
    {
        if (gMovingForward)
            gRenderer->m_camera.MoveIn(CAMERA_STEP * gMovingForward);
        if (gMovingRight)
            gRenderer->m_camera.MoveRight(CAMERA_STEP * gMovingRight);
        if (gMovingUp)
            gRenderer->m_camera.MoveUp(CAMERA_STEP * gMovingUp);
        if (gMovingVertical)
            gRenderer->m_camera.MoveVertical(CAMERA_STEP * gMovingVertical);

        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);

            if (msg.message == WM_QUIT)
                break;
        }

        gRenderer->Render();
        Sleep(5);
    };

    return msg.wParam;
}