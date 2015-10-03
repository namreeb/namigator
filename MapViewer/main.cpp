#include <memory>
#include <windows.h>
#include <windowsx.h>

#include "resource.h"

#include "Renderer.hpp"
#include "CommonControl.hpp"
#include "parser.hpp"
#include "Output/Continent.hpp"

Renderer *gRenderer;

// this is the main message handler for the program
LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
        case WM_CLOSE:
        case WM_DESTROY:
        {
            PostQuitMessage(0);
            return 0;
        } break;

        case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
                case ID_FILE_EXIT:
                    PostQuitMessage(0);
                    return 0;

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
                    MessageBox(NULL, L"Load Kalimdor", L"Load Kalimdor", 0);
                    break;
            }

            break;
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
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)COLOR_WINDOW;
    wc.lpszClassName = L"DXWindow";
    wc.hIconSm = (HICON)LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_APPICON), IMAGE_ICON, 16, 16, 0);
    wc.hIcon = (HICON)LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_APPICON), IMAGE_ICON, 32, 32, 0);
    wc.lpszMenuName = MAKEINTRESOURCE(IDR_MENU1);

    RegisterClassEx(&wc);

    RECT wr = { 0, 0, 1200, 800 };
    AdjustWindowRect(&wr, WS_OVERLAPPEDWINDOW, true);

    guiWindow = CreateWindowEx(NULL,
        L"DXWindow",
        L"CMaNGOS Map Debugging Interface",
        WS_OVERLAPPEDWINDOW,
        100,
        100,
        wr.right - wr.left,
        wr.bottom - wr.top,
        NULL,
        NULL,
        hInstance,
        NULL);

    ZeroMemory(&wc, sizeof(WNDCLASSEX));

    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WindowProc;
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
        WS_OVERLAPPEDWINDOW,
        1300,
        100,
        300,
        300,
        NULL,
        NULL,
        hInstance,
        NULL);
}

// the entry point for any Windows program
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    ::parser::Parser::Initialize();

    HWND guiWindow, controlWindow;
    InitializeWindows(hInstance, guiWindow, controlWindow);

    ShowWindow(guiWindow, nCmdShow);
    ShowWindow(controlWindow, nCmdShow);

    // set up and initialize Direct3D
    
    gRenderer = new Renderer(guiWindow);

    // set up and initialize our Windows common control API for the control window
    CommonControl controls(hInstance, controlWindow);

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

            switch (msg.message)
            {
                case WM_QUIT:
                    break;
            }
        }
    };

    delete gRenderer;

    return msg.wParam;
}