#include "Renderer.hpp"
#include "CommonControl.hpp"
#include "DetourDebugDraw.hpp"

#include "parser/Include/parser.hpp"
#include "parser/Include/Map/Map.hpp"
#include "parser/Include/Adt/Adt.hpp"
#include "parser/Include/Wmo/WmoInstance.hpp"

#include "utility/Include/Directory.hpp"

#include "pathfind/Include/Map.hpp"

#include "DebugUtils/Include/DetourDebugDraw.h"
#include "RecastDetourBuild/Include/Common.hpp"

#include "resource.h"

#include <memory>
#include <windows.h>
#include <windowsx.h>
#include <sstream>
#include <cassert>
#include <vector>

#define START_X             100
#define START_Y             100
#define START_WIDTH         1200
#define START_HEIGHT        800

#define CONTROL_WIDTH       355
#define CONTROL_HEIGHT      280

#define CAMERA_STEP         2.f

// FIXME: Amount to shift control window leftwards.  Find out proper solution for this later!
#define MAGIC_LEFT_SHIFT    15

HWND gGuiWindow, gControlWindow;

std::unique_ptr<Renderer> gRenderer;
std::unique_ptr<CommonControl> gControls;
std::unique_ptr<parser::Map> gMap;
std::unique_ptr<pathfind::Map> gNavMesh;

int gMovingUp = 0;
int gMovingVertical = 0;
int gMovingRight = 0;
int gMovingForward = 0;

bool gHasStart;
utility::Vertex gStart;

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
            auto const distance = static_cast<short>(wParam >> 16);
            gRenderer->m_camera.MoveIn(0.1f * distance);

            return TRUE;
        }

        case WM_LBUTTONDOWN:
        {
            utility::Vertex hit;
            if (gRenderer->HitTest(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), hit))
            {
                gRenderer->ClearSprites();

                if (gHasStart)
                {
                    std::vector<utility::Vertex> path;

                    gHasStart = false;

                    if (gNavMesh->FindPath(gStart, hit, path, true))
                        gRenderer->AddPath(path);
                    else
                        MessageBox(nullptr, L"FindPath failed", L"Path Find", 0);
                }
                else
                {
                    gHasStart = true;
                    gStart = hit;
                    gRenderer->AddSphere(gStart, 6.f);
                }

                return TRUE;
            }

            break;
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

enum Controls : int
{
    MapsCombo,
    ADTX,
    ADTY,
    LoadADT,
    Wireframe,
    RenderADT,
    RenderLiquid,
    RenderWMO,
    RenderDoodad,
    RenderMesh,
};

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
    //wc.lpszMenuName = MAKEINTRESOURCE(IDR_MENU1);

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
    wc.lpfnWndProc = DefWindowProc;
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

void GetMapName(const std::string &inName, std::string &outName)
{
    outName = inName.substr(inName.find(' ') + 1);
    outName = outName.substr(0, outName.find('('));
    outName.erase(std::remove(outName.begin(), outName.end(), ' '));
}

void ChangeMap(const std::string &cn)
{
    gHasStart = false;

    if (gMap)
        gRenderer->ClearBuffers();

    if (cn == "")
        return;

    std::string mapName;

    GetMapName(cn, mapName);

    gMap = std::make_unique<parser::Map>(mapName);
    gNavMesh = std::make_unique<pathfind::Map>(".\\Maps", mapName);

    // if the loaded map has no ADTs, but instead a global WMO, load it now
    if (auto const wmo = gMap->GetGlobalWmoInstance())
    {
        std::vector<utility::Vertex> vertices;
        std::vector<int> indices;

        wmo->BuildTriangles(vertices, indices);
        gRenderer->AddWmo(0, vertices, indices);

        wmo->BuildLiquidTriangles(vertices, indices);
        gRenderer->AddLiquid(vertices, indices);

        wmo->BuildDoodadTriangles(vertices, indices);
        gRenderer->AddDoodad(0, vertices, indices);

        auto const cx = (wmo->Bounds.MaxCorner.X + wmo->Bounds.MinCorner.X) / 2.f;
        auto const cy = (wmo->Bounds.MaxCorner.Y + wmo->Bounds.MinCorner.Y) / 2.f;
        auto const cz = (wmo->Bounds.MaxCorner.Z + wmo->Bounds.MinCorner.Z) / 2.f;

        gRenderer->m_camera.Move(cx + 300.f, cy + 300.f, cz + 300.f);
        gRenderer->m_camera.LookAt(cx, cy, cz);

        gControls->Enable(Controls::ADTX, false);
        gControls->Enable(Controls::ADTY, false);
        gControls->Enable(Controls::LoadADT, false);
    }
    else
    {
        gControls->Enable(Controls::ADTX, true);
        gControls->Enable(Controls::ADTY, true);
        gControls->Enable(Controls::LoadADT, true);
    }
}

void LoadADTFromGUI()
{
    // if we have not yet loaded the Map, do so now
    if (!gMap)
        ChangeMap(gControls->GetText(Controls::MapsCombo));

    // if the current Map has only a global WMO, do nothing further
    if (gMap->GetGlobalWmoInstance())
        return;

    auto const adtX = std::stoi(gControls->GetText(Controls::ADTX));
    auto const adtY = std::stoi(gControls->GetText(Controls::ADTY));

    auto const adt = gMap->GetAdt(adtX, adtY);

    for (int x = 0; x < MeshSettings::ChunksPerAdt; ++x)
        for (int y = 0; y < MeshSettings::ChunksPerAdt; ++y)
        {
            auto const chunk = adt->GetChunk(x, y);

            gRenderer->AddTerrain(chunk->m_terrainVertices, chunk->m_terrainIndices);
            gRenderer->AddLiquid(chunk->m_liquidVertices, chunk->m_liquidIndices);

            for (auto &d : chunk->m_doodadInstances)
            {
                if (gRenderer->HasDoodad(d))
                    continue;

                auto const doodad = gMap->GetDoodadInstance(d);

                assert(doodad);

                std::vector<utility::Vertex> vertices;
                std::vector<int> indices;

                doodad->BuildTriangles(vertices, indices);
                gRenderer->AddDoodad(d, vertices, indices);
            }

            for (auto &w : chunk->m_wmoInstances)
            {
                if (gRenderer->HasWmo(w))
                    continue;

                auto const wmo = gMap->GetWmoInstance(w);

                assert(wmo);

                std::vector<utility::Vertex> vertices;
                std::vector<int> indices;

                wmo->BuildTriangles(vertices, indices);
                gRenderer->AddWmo(w, vertices, indices);

                wmo->BuildLiquidTriangles(vertices, indices);
                gRenderer->AddLiquid(vertices, indices);

                if (gRenderer->HasDoodad(w))
                    continue;

                wmo->BuildDoodadTriangles(vertices, indices);
                gRenderer->AddDoodad(w, vertices, indices);
            }
        }

    for (auto y = adtY * MeshSettings::TilesPerADT; y < (adtY + 1)*MeshSettings::TilesPerADT; ++y)
        for (auto x = adtX * MeshSettings::TilesPerADT; x < (adtX + 1)*MeshSettings::TilesPerADT; ++x)
            if (gNavMesh->LoadTile(x, y))
            {
                DetourDebugDraw dd(gRenderer.get());

                duDebugDrawNavMeshWithClosedList(&dd, gNavMesh->GetNavMesh(), gNavMesh->GetNavMeshQuery(), 0);
            }

    const float cx = (adt->Bounds.MaxCorner.X + adt->Bounds.MinCorner.X) / 2.f;
    const float cy = (adt->Bounds.MaxCorner.Y + adt->Bounds.MinCorner.Y) / 2.f;
    const float cz = (adt->Bounds.MaxCorner.Z + adt->Bounds.MinCorner.Z) / 2.f;

    gRenderer->m_camera.Move(cx + 300.f, cy + 300.f, cz + 300.f);
    gRenderer->m_camera.LookAt(cx, cy, cz);
}

// the entry point for any Windows program
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/, LPSTR /*lpCmdLine*/, int nCmdShow)
{
    if (!utility::Directory::Exists(".\\Data"))
    {
        MessageBox(NULL, L"Data folder does not exist", L"ERROR", 0);
        return EXIT_FAILURE;
    }

    if (!utility::Directory::Exists(".\\Maps"))
    {
        MessageBox(NULL, L"Maps folder does not exist", L"ERROR", 0);
        return EXIT_FAILURE;
    }

    gHasStart = false;

    parser::Parser::Initialize(".\\Data");

    InitializeWindows(hInstance, gGuiWindow, gControlWindow);

    ShowWindow(gGuiWindow, nCmdShow);
    ShowWindow(gControlWindow, nCmdShow);

    // set up and initialize Direct3D
    gRenderer = std::make_unique<Renderer>(gGuiWindow);

    // set up and initialize our Windows common control API for the control window
    gControls = std::make_unique<CommonControl>(gControlWindow);

    gControls->AddLabel(L"Select Map:", 10, 12);

    std::vector<std::wstring> Maps;
    Maps.push_back(L"000 Azeroth");
    Maps.push_back(L"001 Kalimdor");
    Maps.push_back(L"013 Test");
    Maps.push_back(L"025 Scott Test");
    Maps.push_back(L"029 Test");
    Maps.push_back(L"030 PVPZone01 (Alterac Valley)");
    Maps.push_back(L"033 Shadowfang");
    Maps.push_back(L"034 StormwindJail (Stockades)");
    //Maps.push_back(L"035 StormwindPrison");
    Maps.push_back(L"036 DeadminesInstance");
    Maps.push_back(L"037 PVPZone02 (Azshara Crater)");
    Maps.push_back(L"043 WailingCaverns");
    Maps.push_back(L"489 PVPzone03 (Warsong Gulch)");
    Maps.push_back(L"529 PVPzone04 (Arathi Basin)");
    Maps.push_back(L"530 Expansion01 (Outland");
    Maps.push_back(L"571 Northrend");

    gControls->AddComboBox(Controls::MapsCombo, Maps, 115, 10, ChangeMap);

    gControls->AddLabel(L"X:", 10, 35);
    gControls->AddTextBox(Controls::ADTX, L"38", 25, 35, 75, 20);

    gControls->AddLabel(L"Y:", 10, 60);
    gControls->AddTextBox(Controls::ADTY, L"40", 25, 60, 75, 20);

    gControls->AddButton(Controls::LoadADT, L"Load ADT", 115, 57, 100, 25, LoadADTFromGUI);

    gControls->Enable(Controls::ADTX, false);
    gControls->Enable(Controls::ADTY, false);
    gControls->Enable(Controls::LoadADT, false);

    gControls->AddCheckBox(Controls::Wireframe, L"Wireframe", 10, 85, false, [](bool checked) { gRenderer->SetWireframe(checked); });
    gControls->AddCheckBox(Controls::RenderADT, L"Render ADT", 10, 110, true, [](bool checked) { gRenderer->SetRenderADT(checked); });
    gControls->AddCheckBox(Controls::RenderLiquid, L"Render Liquid", 10, 135, true, [](bool checked) { gRenderer->SetRenderLiquid(checked); });
    gControls->AddCheckBox(Controls::RenderWMO, L"Render WMO", 10, 160, true, [](bool checked) { gRenderer->SetRenderWMO(checked); });
    gControls->AddCheckBox(Controls::RenderDoodad, L"Render Doodad", 10, 185, true, [](bool checked) { gRenderer->SetRenderDoodad(checked); });
    gControls->AddCheckBox(Controls::RenderMesh, L"Render Mesh", 10, 210, true, [](bool checked) { gRenderer->SetRenderMesh(checked); });

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

    return static_cast<int>(msg.wParam);
}