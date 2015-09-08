#include <memory>
#include <windows.h>
#include <windowsx.h>

#include "resource.h"

#include "Renderer.hpp"
#include "parser.hpp"
#include "Output/Continent.hpp"

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
                    break;
            }

            break;
        }
    }

    return DefWindowProc(hWnd, message, wParam, lParam);
}

HWND InitializeWindow(HINSTANCE hInstance)
{
    HWND hWnd;
    WNDCLASSEX wc;

    ZeroMemory(&wc, sizeof(WNDCLASSEX));

    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)COLOR_WINDOW;
    wc.lpszClassName = L"WindowClass";
    wc.hIconSm = (HICON)LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_APPICON), IMAGE_ICON, 16, 16, 0);
    wc.hIcon = (HICON)LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_APPICON), IMAGE_ICON, 32, 32, 0);
    wc.lpszMenuName = MAKEINTRESOURCE(IDR_MENU1);

    RegisterClassEx(&wc);

    RECT wr = { 0, 0, 1200, 800 };
    AdjustWindowRect(&wr, WS_OVERLAPPEDWINDOW, FALSE);

    hWnd = CreateWindowEx(NULL,
        L"WindowClass",
        L"CMaNGOS Map Debugging Interface",
        WS_OVERLAPPEDWINDOW,
        200,
        100,
        wr.right - wr.left,
        wr.bottom - wr.top,
        NULL,
        NULL,
        hInstance,
        NULL);

    return hWnd;
}

// the entry point for any Windows program
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    HWND window = InitializeWindow(hInstance);

    ShowWindow(window, nCmdShow);

    // set up and initialize Direct3D
    Renderer renderer(window);

    ::parser::Parser::Initialize();

    ::parser::Continent azeroth("Azeroth");

    auto adt = azeroth.LoadAdt(32, 48);
    auto chunk0 = adt->GetChunk(0, 0);

    std::vector<VERTEX> vertices;
    vertices.reserve(chunk0->m_terrainVertices.size());

    for (unsigned int i = 0; i < chunk0->m_terrainVertices.size(); ++i)
        vertices.push_back({ chunk0->m_terrainVertices[i].X , chunk0->m_terrainVertices[i].Y, chunk0->m_terrainVertices[i].Z, { 0.f, 1.f, 0.f, 1.f } });

    renderer.InitializeVertexBuffer(vertices, chunk0->m_terrainIndices);

    // enter the main loop:

    MSG msg;

    while (TRUE)
    {
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);

            if (msg.message == WM_QUIT)
                break;
        }

        renderer.Render();
    }

    return msg.wParam;
}