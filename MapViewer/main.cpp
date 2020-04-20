#include "Renderer.hpp"
#include "CommonControl.hpp"
#include "DetourDebugDraw.hpp"

#include "parser/MpqManager.hpp"
#include "parser/Map/Map.hpp"
#include "parser/Adt/Adt.hpp"
#include "parser/Wmo/WmoInstance.hpp"

#include "utility/MathHelper.hpp"
#include "utility/Exception.hpp"
#include "utility/Vector.hpp"
#include "utility/Quaternion.hpp"

#include "pathfind/Map.hpp"

#include "recastnavigation/DebugUtils/Include/DetourDebugDraw.h"
#include "RecastDetourBuild/Common.hpp"

#include "resource.h"

#include <memory>
#include <windows.h>
#include <shellapi.h>
#include <windowsx.h>
#include <sstream>
#include <cassert>
#include <vector>
#include <experimental/filesystem>

#define START_X             100
#define START_Y             100
#define START_WIDTH         1200
#define START_HEIGHT        800

#define CONTROL_WIDTH       355
#define CONTROL_HEIGHT      360

#define CAMERA_STEP         2.f

// FIXME: Amount to shift control window leftwards.  Find out proper solution for this later!
#define MAGIC_LEFT_SHIFT    15

namespace
{
HWND gGuiWindow, gControlWindow;

fs::path gNavData;

std::unique_ptr<Renderer> gRenderer;
std::unique_ptr<CommonControl> gControls;
std::unique_ptr<parser::Map> gMap;
std::unique_ptr<pathfind::Map> gNavMesh;

struct MouseDoodad
{
    std::uint64_t Guid;
    std::shared_ptr<pathfind::Model> Model;

    unsigned int DisplayId = 0;

    math::Vertex Position;
    math::Quaternion Rotation;
    float Scale = 1.0f;

    math::Matrix Transform;
};

std::unique_ptr<MouseDoodad> gMouseDoodad;

void UpdateMouseDoodadTransform()
{
    gMouseDoodad->Transform = math::Matrix::CreateFromQuaternion(gMouseDoodad->Rotation)
        * math::Matrix::CreateScalingMatrix(gMouseDoodad->Scale)
        * math::Matrix::CreateTranslationMatrix(gMouseDoodad->Position);
}

void MoveMouseDoodad(math::Vertex const& position)
{
    gMouseDoodad->Position = position;
    UpdateMouseDoodadTransform();

    auto vertices = gMouseDoodad->Model->m_aabbTree.Vertices();
    for (auto& vertex : vertices)
        vertex = math::Vector3::Transform(vertex, gMouseDoodad->Transform);

    gRenderer->ClearGameObjects();
    gRenderer->AddGameObject(vertices, gMouseDoodad->Model->m_aabbTree.Indices());
}

int gMovingUp = 0;
int gMovingVertical = 0;
int gMovingRight = 0;
int gMovingForward = 0;

bool gHasStart;
math::Vertex gStart;

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
            std::uint32_t param = 0;
            math::Vertex hit;

            if (!!(wParam & MK_SHIFT))
            {
                if (gRenderer->HitTest(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), Renderer::TerrainGeometryFlag, hit, param))
                {
                    std::stringstream ss;
                    ss << "AreaID: " << param << std::endl;
                    OutputDebugStringA(ss.str().c_str());
                }
            }
            else
            {
                if (!!gMouseDoodad)
                {
                    gNavMesh->AddGameObject(gMouseDoodad->Guid, gMouseDoodad->DisplayId, gMouseDoodad->Position, gMouseDoodad->Rotation);
                    gMouseDoodad.reset();

                    DetourDebugDraw dd(gRenderer.get());

                    duDebugDrawNavMeshWithClosedList(&dd, gNavMesh->GetNavMesh(), gNavMesh->GetNavMeshQuery(), 0);

                    return TRUE;
                }

                if (gRenderer->HitTest(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), Renderer::NavMeshGeometryFlag, hit, param))
                {
                    gRenderer->ClearSprites();

                    if (gHasStart)
                    {
                        std::vector<math::Vertex> path;

                        gHasStart = false;

                        if (gNavMesh->FindPath(gStart, hit, path, false))
                            gRenderer->AddPath(path);
                        else
                            MessageBox(nullptr, "FindPath failed", "Path Find", 0);
                    }
                    else
                    {
                        gHasStart = true;
                        gStart = hit;
                        gRenderer->AddSphere(gStart, 3.f);
                    }

                    return TRUE;
                }
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
            else if (!!gMouseDoodad)
            {
                std::uint32_t param = 0;
                math::Vertex hit;

                if (gRenderer->HitTest(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), Renderer::CollidableGeometryFlag, hit, param))
                {
                    MoveMouseDoodad(hit);
                    return TRUE;
                }
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

        default:
            break;
    }

    return DefWindowProc(hWnd, message, wParam, lParam);
}

enum Controls : int
{
    MapsCombo,
    PositionX,
    PositionY,
    Load,
    ZSearch,
    Wireframe,
    RenderADT,
    RenderLiquid,
    RenderWMO,
    RenderDoodad,
    RenderMesh,
    SpawnDoodadEdit,
    SpawnDoodadButton,
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
    wc.lpszClassName = "DXWindow";
    wc.hIconSm = (HICON)LoadImage(GetModuleHandle(nullptr), MAKEINTRESOURCE(IDI_APPICON), IMAGE_ICON, 16, 16, 0);
    wc.hIcon = (HICON)LoadImage(GetModuleHandle(nullptr), MAKEINTRESOURCE(IDI_APPICON), IMAGE_ICON, 32, 32, 0);
    //wc.lpszMenuName = MAKEINTRESOURCE(IDR_MENU1);

    RegisterClassEx(&wc);

    RECT wr = { START_X, START_Y, START_X + START_WIDTH, START_Y + START_HEIGHT };
    AdjustWindowRect(&wr, WS_OVERLAPPEDWINDOW, true);

    guiWindow = CreateWindowEx(WS_EX_RIGHTSCROLLBAR,
        "DXWindow",
        "namigator testing interface",
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
    wc.lpszClassName = "ControlWindow";
    wc.hIconSm = (HICON)LoadImage(GetModuleHandle(nullptr), MAKEINTRESOURCE(IDI_APPICON), IMAGE_ICON, 16, 16, 0);
    wc.hIcon = (HICON)LoadImage(GetModuleHandle(nullptr), MAKEINTRESOURCE(IDI_APPICON), IMAGE_ICON, 32, 32, 0);

    RegisterClassEx(&wc);

    controlWindow = CreateWindowEx(WS_EX_RIGHTSCROLLBAR,
        "ControlWindow",
        "Control",
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

void LoadAdt(const parser::Adt *adt)
{
    for (int chunkX = 0; chunkX < MeshSettings::ChunksPerAdt; ++chunkX)
        for (int chunkY = 0; chunkY < MeshSettings::ChunksPerAdt; ++chunkY)
        {
            auto const chunk = adt->GetChunk(chunkX, chunkY);

            gRenderer->AddTerrain(chunk->m_terrainVertices, chunk->m_terrainIndices, chunk->m_areaId);
            gRenderer->AddLiquid(chunk->m_liquidVertices, chunk->m_liquidIndices);

            for (auto &d : chunk->m_doodadInstances)
            {
                if (gRenderer->HasDoodad(d))
                    continue;

                auto const doodad = gMap->GetDoodadInstance(d);

                assert(doodad);

                std::vector<math::Vertex> vertices;
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

                std::vector<math::Vertex> vertices;
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

    if (!!gNavMesh && gNavMesh->LoadADT(adt->X, adt->Y))
    {
        DetourDebugDraw dd(gRenderer.get());

        duDebugDrawNavMeshWithClosedList(&dd, gNavMesh->GetNavMesh(), gNavMesh->GetNavMeshQuery(), 0);
    }
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

    try
    {
        gMap = std::make_unique<parser::Map>(mapName);
        gNavMesh = std::make_unique<pathfind::Map>(gNavData.string(), mapName);
    }
    catch (utility::exception const &e)
    {
        MessageBoxA(nullptr, e.what(), "ERROR", MB_ICONERROR);

        if (!gMap)
            return;
    }

    // UlduarRaid?  Load it all \o/
    if (gMap->Id == 603)
    {
        float avgX = 0.f, avgY = 0.f, avgZ = 0.f;
        int count = 0;
        bool looked = false;

        for (auto y = 0; y < MeshSettings::Adts; ++y)
            for (auto x = 0; x < MeshSettings::Adts; ++x)
                if (gMap->HasAdt(x, y))
                {
                    auto const adt = gMap->GetAdt(x, y);

                    ++count;

                    const float cx = (adt->Bounds.MaxCorner.X + adt->Bounds.MinCorner.X) / 2.f;
                    const float cy = (adt->Bounds.MaxCorner.Y + adt->Bounds.MinCorner.Y) / 2.f;
                    const float cz = (adt->Bounds.MaxCorner.Z + adt->Bounds.MinCorner.Z) / 2.f;

                    avgX += cx;
                    avgY += cy;
                    avgZ += cz;

                    if (!looked)
                    {
                        gRenderer->m_camera.Move(cx + 300.f, cy + 300.f, cz + 300.f);
                        gRenderer->m_camera.LookAt(cx, cy, cz);
                        looked = true;
                    }
                }

        avgX /= count;
        avgY /= count;
        avgZ /= count;

//        gRenderer->m_camera.Move(avgX + 300.f, avgY + 300.f, avgZ + 300.f);
//        gRenderer->m_camera.LookAt(avgX, avgY, avgZ);
    }

    // if the loaded map has no ADTs, but instead a global WMO, load it now, including all mesh tiles
    if (auto const wmo = gMap->GetGlobalWmoInstance())
    {
        std::vector<math::Vertex> vertices;
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

        gControls->Enable(Controls::PositionX, false);
        gControls->Enable(Controls::PositionY, false);
        gControls->Enable(Controls::Load, false);

        DetourDebugDraw dd(gRenderer.get());

        duDebugDrawNavMeshWithClosedList(&dd, gNavMesh->GetNavMesh(), gNavMesh->GetNavMeshQuery(), 0);
    }
    else
    {
        gControls->Enable(Controls::PositionX, true);
        gControls->Enable(Controls::PositionY, true);
        gControls->Enable(Controls::Load, true);
    }
}

void LoadPositionFromGUI()
{
    // if we have not yet loaded the Map, do so now
    if (!gMap)
        ChangeMap(gControls->GetText(Controls::MapsCombo));

    // if the current Map has only a global WMO, do nothing further
    if (gMap->GetGlobalWmoInstance())
        return;

    int x, y;

    // if there is a decimal, it is a world coordinate
    if (gControls->GetText(Controls::PositionX).find('.') != std::string::npos ||
        gControls->GetText(Controls::PositionY).find('.') != std::string::npos)
    {
        auto const posX = std::stof(gControls->GetText(Controls::PositionX));
        auto const posY = std::stof(gControls->GetText(Controls::PositionY));

        math::Convert::WorldToAdt({ posX, posY, 0.f }, x, y);
    }
    else
    {
        auto const intX = std::stoi(gControls->GetText(Controls::PositionX));
        auto const intY = std::stoi(gControls->GetText(Controls::PositionY));

        // if either is negative, it is a world coordinate, or
        // if either is greater than or equal to 64, it is a world coordinate
        if (intX < 0 || intY < 0 || intX >= MeshSettings::Adts || intY >= MeshSettings::Adts)
            math::Convert::WorldToAdt({ static_cast<float>(intX), static_cast<float>(intY), 0.f }, x, y);
        // otherwise, it is an adt coordinate
        else
        {
            x = intX;
            y = intY;
        }
    }

    // if both the x and y values are integers less than 64, assume the coordinates are ADT coordinates
    if (x < MeshSettings::Adts && y < MeshSettings::Adts && x >= 0 && y >= 0)
    {
        if (!gMap->HasAdt(x, y))
        {
            MessageBox(nullptr, "Map does not have the specified ADT tile", "Error", MB_OK | MB_ICONEXCLAMATION);
            return;
        }

        try
        {
            auto const adt = gMap->GetAdt(x, y);

            LoadAdt(adt);

            const float cx = (adt->Bounds.MaxCorner.X + adt->Bounds.MinCorner.X) / 2.f;
            const float cy = (adt->Bounds.MaxCorner.Y + adt->Bounds.MinCorner.Y) / 2.f;
            const float cz = (adt->Bounds.MaxCorner.Z + adt->Bounds.MinCorner.Z) / 2.f;

            gRenderer->m_camera.Move(cx + 300.f, cy + 300.f, cz + 300.f);
            gRenderer->m_camera.LookAt(cx, cy, cz);
        }
        catch (const std::exception &e)
        {
            MessageBox(nullptr, e.what(), "Error loading ADT", MB_OK | MB_ICONERROR);
        }
    }
}

void SearchZValues()
{
    auto const posX = std::stof(gControls->GetText(Controls::PositionX));
    auto const posY = std::stof(gControls->GetText(Controls::PositionY));

    std::vector<float> output;
    auto const adt = gNavMesh->FindHeights(posX, posY, output);

    std::stringstream result;

    result << "Heights at (" << posX << ", " << posY << "):\n";
    for (auto const h : output)
        result << h << "\n";

    MessageBoxA(nullptr, result.str().c_str(), "Z Search Results", 0);
}

void SpawnGOFromGUI()
{
    if (!gNavMesh)
        return;

    auto const displayId = std::stoi(gControls->GetText(Controls::SpawnDoodadEdit));
    auto const model = gNavMesh->GetOrLoadModelByDisplayId(displayId);

    if (!model)
        return;

    gMouseDoodad = std::make_unique<MouseDoodad>();
    gMouseDoodad->Guid = static_cast<std::uint64_t>(rand());
    gMouseDoodad->DisplayId = displayId;
    gMouseDoodad->Model = model;
    gMouseDoodad->Scale = 1.0f;
    gMouseDoodad->Rotation = math::Quaternion(0, 0, 0, 1);

    UpdateMouseDoodadTransform();
}
}

// the entry point for any Windows program
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/, LPSTR lpCmdLine, int nCmdShow)
{
    int argc;
    auto argv = ::CommandLineToArgvW(GetCommandLineW(), &argc);

    if (!argv || argc != 3)
    {
        MessageBox(nullptr, "Usage: <nav data> <wow data>", "ERROR", MB_ICONERROR);
        return EXIT_FAILURE;
    }

    gNavData = fs::path(argv[1]);

    if (!fs::is_directory(gNavData))
    {
        MessageBox(nullptr, "Navigation data directory not found", "ERROR", MB_ICONERROR);
        return EXIT_FAILURE;
    }

    const fs::path wow_path(argv[2]);

    if (!fs::is_directory(wow_path))
    {
        MessageBox(nullptr, "Wow data directory not found", "ERROR", MB_ICONERROR);
        return EXIT_FAILURE;
    }

    gHasStart = false;

    parser::sMpqManager.Initialize(wow_path.string());

    InitializeWindows(hInstance, gGuiWindow, gControlWindow);

    ShowWindow(gGuiWindow, nCmdShow);
    ShowWindow(gControlWindow, nCmdShow);

    // set up and initialize Direct3D
    gRenderer = std::make_unique<Renderer>(gGuiWindow);

    // set up and initialize our Windows common control API for the control window
    gControls = std::make_unique<CommonControl>(gControlWindow);

    gControls->AddLabel("Select Map:", 10, 12);

    std::vector<std::string> maps;
    maps.push_back("000 Azeroth");
    maps.push_back("001 Kalimdor");
    maps.push_back("013 Test");
    maps.push_back("025 Scott Test");
    maps.push_back("029 Test");
    maps.push_back("030 PVPZone01 (Alterac Valley)");
    maps.push_back("033 Shadowfang");
    maps.push_back("034 StormwindJail (Stockades)");
    //Maps.push_back("035 StormwindPrison");
    maps.push_back("036 DeadminesInstance");
    maps.push_back("037 PVPZone02 (Azshara Crater)");
    maps.push_back("043 WailingCaverns");
    maps.push_back("489 PVPzone03 (Warsong Gulch)");
    maps.push_back("529 PVPzone04 (Arathi Basin)");
    maps.push_back("530 Expansion01 (Outland)");
    maps.push_back("571 Northrend");
    maps.push_back("603 UlduarRaid");

    gControls->AddComboBox(Controls::MapsCombo, maps, 115, 10, ChangeMap);

    gControls->AddLabel("X:", 10, 35);
    gControls->AddTextBox(Controls::PositionX, "1753.842285", 25, 35, 75, 20);

    gControls->AddLabel("Y:", 10, 60);
    gControls->AddTextBox(Controls::PositionY, "-662.430908", 25, 60, 75, 20);

    gControls->AddButton(Controls::Load, "Load", 115, 57, 75, 25, LoadPositionFromGUI);
    gControls->AddButton(Controls::ZSearch, "Z Search", 200, 57, 75, 25, SearchZValues);

    gControls->Enable(Controls::PositionX, false);
    gControls->Enable(Controls::PositionY, false);
    gControls->Enable(Controls::Load, false);

    gControls->AddCheckBox(Controls::Wireframe, "Wireframe", 10, 85, false, [](bool checked) { gRenderer->SetWireframe(checked); });
    gControls->AddCheckBox(Controls::RenderADT, "Render ADT", 10, 110, true, [](bool checked) { gRenderer->SetRenderADT(checked); });
    gControls->AddCheckBox(Controls::RenderLiquid, "Render Liquid", 10, 135, true, [](bool checked) { gRenderer->SetRenderLiquid(checked); });
    gControls->AddCheckBox(Controls::RenderWMO, "Render WMO", 10, 160, true, [](bool checked) { gRenderer->SetRenderWMO(checked); });
    gControls->AddCheckBox(Controls::RenderDoodad, "Render Doodad", 10, 185, true, [](bool checked) { gRenderer->SetRenderDoodad(checked); });
    gControls->AddCheckBox(Controls::RenderMesh, "Render Mesh", 10, 210, true, [](bool checked) { gRenderer->SetRenderMesh(checked); });

    gControls->AddTextBox(Controls::SpawnDoodadEdit, "Display ID", 10, 245, 90, 20);
    gControls->AddButton(Controls::SpawnDoodadButton, "Spawn GO", 115, 242, 100, 25, SpawnGOFromGUI);

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