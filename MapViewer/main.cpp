#include <memory>
#include <windows.h>
#include <windowsx.h>
#include <sstream>

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
#define CONTROL_HEIGHT      300

// FIXME: Amount to shift control window leftwards.  Find out proper solution for this later!
#define MAGIC_LEFT_SHIFT    15

HWND gGuiWindow, gControlWindow;

Renderer *gRenderer;

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

            MoveWindow(gControlWindow, rect->right - MAGIC_LEFT_SHIFT, rect->top, 300, 300, FALSE);

            return TRUE;
        }

        case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
                case ID_FILE_EXIT:
                    PostQuitMessage(0);
                    return TRUE;

                case ID_LOADCONTINENT_AZEROTH:
                {
                    ::parser::Continent azeroth("Azeroth");

                    auto adt = azeroth.LoadAdt(32, 48);
                    auto chunk0 = adt->GetChunk(0, 0);

                    std::vector<ColoredVertex> vertices;
                    vertices.reserve(chunk0->m_terrainVertices.size());

                    for (unsigned int i = 0; i < chunk0->m_terrainVertices.size(); ++i)
                        vertices.push_back({ chunk0->m_terrainVertices[i].X , chunk0->m_terrainVertices[i].Y, chunk0->m_terrainVertices[i].Z, { 0.f, 1.f, 0.f, 1.f } });

                    gRenderer->AddGeometry(vertices, chunk0->m_terrainIndices);

                    break;
                }

                case ID_LOADCONTINENT_KALIMDOR:
                {
                    RECT guiRect, controlRect;

                    if (!GetWindowRect(gGuiWindow, &guiRect) || !GetWindowRect(gControlWindow, &controlRect))
                    {
                        MessageBox(NULL, L"FOOBAR!", L"DEBUG", 0);
                        PostQuitMessage(0);
                        
                        return TRUE;
                    }

                    std::wstringstream str;

                    str << "GUI Window:" << std::endl;
                    str << "    Left:" << guiRect.left << std::endl;
                    str << "   Right:" << guiRect.right << std::endl;

                    str << "Control Window:" << std::endl;
                    str << "    Left:" << controlRect.left << std::endl;
                    str << "   Right:" << controlRect.right << std::endl;

                    MessageBox(gGuiWindow, str.str().c_str(), L"DEBUG", 0);

                    break;
                }
            }

            break;
        }
    }

    return DefWindowProc(hWnd, message, wParam, lParam);
}

LRESULT CALLBACK ControlWindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
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
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)COLOR_WINDOW;
    wc.lpszClassName = L"DXWindow";
    wc.hIconSm = (HICON)LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_APPICON), IMAGE_ICON, 16, 16, 0);
    wc.hIcon = (HICON)LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_APPICON), IMAGE_ICON, 32, 32, 0);
    wc.lpszMenuName = MAKEINTRESOURCE(IDR_MENU1);

    RegisterClassEx(&wc);

    RECT wr = { START_X, START_Y, START_X + START_WIDTH, START_Y + START_HEIGHT };
    AdjustWindowRect(&wr, WS_OVERLAPPEDWINDOW, true);

    guiWindow = CreateWindowEx(NULL,
        L"DXWindow",
        L"CMaNGOS Map Debugging Interface",
        WS_OVERLAPPEDWINDOW,
        wr.left,
        wr.top,
        wr.right - wr.left,
        wr.bottom - wr.top,
        NULL,
        NULL,
        hInstance,
        NULL);

    ZeroMemory(&wc, sizeof(WNDCLASSEX));

    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = ControlWindowProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)COLOR_WINDOW;
    wc.lpszClassName = L"ControlWindow";
    wc.hIconSm = (HICON)LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_APPICON), IMAGE_ICON, 16, 16, 0);
    wc.hIcon = (HICON)LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_APPICON), IMAGE_ICON, 32, 32, 0);

    RegisterClassEx(&wc);

    controlWindow = CreateWindowEx(NULL,
        L"ControlWindow",
        L"Control",
        (WS_BORDER | WS_CAPTION) & (~WS_ICONIC),
        wr.right - MAGIC_LEFT_SHIFT,
        wr.top,
        CONTROL_WIDTH,
        CONTROL_HEIGHT,
        NULL,
        NULL,
        hInstance,
        NULL);
}

// the entry point for any Windows program
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    ::parser::Parser::Initialize();

    InitializeWindows(hInstance, gGuiWindow, gControlWindow);

    ShowWindow(gGuiWindow, nCmdShow);
    ShowWindow(gControlWindow, nCmdShow);

    // set up and initialize Direct3D
    
    gRenderer = new Renderer(gGuiWindow);

    // set up and initialize our Windows common control API for the control window
    CommonControl controls(hInstance, gControlWindow);

    controls.AddLabel(L"Label1", L"Label1Text", 20, 20, 115, 20);
    controls.AddLabel(L"Label2", L"Label2Text", 20, 40, 115, 20);

    // enter the main loop:

    MSG msg;

    while (true)
    {
        gRenderer->Render();

        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);

            if (msg.message == WM_QUIT)
                break;
        }
    };

    delete gRenderer;

    return msg.wParam;
}