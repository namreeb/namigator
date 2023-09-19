#include "Common.hpp"
#include "CommonControl.hpp"
#include "DetourDebugDraw.hpp"
#include "Renderer.hpp"
#include "parser/Adt/Adt.hpp"
#include "parser/Map/Map.hpp"
#include "parser/MpqManager.hpp"
#include "parser/Wmo/WmoInstance.hpp"
#include "pathfind/Map.hpp"
#include "recastnavigation/DebugUtils/Include/DetourDebugDraw.h"
#include "resource.h"
#include "utility/Exception.hpp"
#include "utility/MathHelper.hpp"
#include "utility/Quaternion.hpp"
#include "utility/Vector.hpp"

#include <cassert>
#include <memory>
#include <shellapi.h>
#include <sstream>
#include <vector>
#include <windows.h>
#include <windowsx.h>

#define START_X      100
#define START_Y      100
#define START_WIDTH  1200
#define START_HEIGHT 800

#define CONTROL_WIDTH  355
#define CONTROL_HEIGHT 360

#define CAMERA_STEP 2.f

// FIXME: Amount to shift control window leftwards.  Find out proper solution
// for this later!
#define MAGIC_LEFT_SHIFT 15

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
    gMouseDoodad->Transform =
        math::Matrix::CreateFromQuaternion(gMouseDoodad->Rotation) *
        math::Matrix::CreateScalingMatrix(gMouseDoodad->Scale) *
        math::Matrix::CreateTranslationMatrix(gMouseDoodad->Position);
}

void MoveMouseDoodad(math::Vertex const& position)
{
    gMouseDoodad->Position = position;
    UpdateMouseDoodadTransform();

    auto vertices = gMouseDoodad->Model->m_aabbTree.Vertices();
    for (auto& vertex : vertices)
        vertex = math::Vector3::Transform(vertex, gMouseDoodad->Transform);

    gRenderer->ClearGameObjects();
    gRenderer->AddGameObject(vertices,
                             gMouseDoodad->Model->m_aabbTree.Indices());
}

void duDebugDrawNavMeshPolysWithoutFlags(struct duDebugDraw* dd,
                                         const dtNavMesh& mesh,
                                         const unsigned short polyFlags,
                                         const unsigned int col)
{
    if (!dd)
        return;

    for (int i = 0; i < mesh.getMaxTiles(); ++i)
    {
        const dtMeshTile* tile = mesh.getTile(i);
        if (!tile->header)
            continue;
        dtPolyRef base = mesh.getPolyRefBase(tile);

        for (int j = 0; j < tile->header->polyCount; ++j)
        {
            const dtPoly* p = &tile->polys[j];
            if ((p->flags & polyFlags) != 0)
                continue;
            duDebugDrawNavMeshPoly(dd, mesh, base | (dtPolyRef)j, col);
        }
    }
}

int gMovingUp = 0;
int gMovingVertical = 0;
int gMovingRight = 0;
int gMovingForward = 0;

bool gHasStart;
math::Vertex gStart;

// this is the main message handler for the program
LRESULT CALLBACK GuiWindowProc(HWND hWnd, UINT message, WPARAM wParam,
                               LPARAM lParam)
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
            const RECT* rect = (const RECT*)lParam;

            MoveWindow(gControlWindow, rect->right - MAGIC_LEFT_SHIFT,
                       rect->top, CONTROL_WIDTH, CONTROL_HEIGHT, FALSE);

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
                if (gRenderer->HitTest(
                        GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam),
                        Renderer::CollidableGeometryFlag, hit, param))
                {
                    std::stringstream ss;
                    ss << "Hit terrain at (" << hit.X << ", " << hit.Y << ", "
                       << hit.Z << ")\n";

                    int adtX, adtY, chunkX, chunkY;
                    math::Convert::WorldToAdt(hit, adtX, adtY, chunkX, chunkY);

                    ss << "ADT: (" << adtX << ", " << adtY << ") Chunk: ("
                       << chunkX << ", " << chunkY << ")\n";

                    unsigned int zone, area;
                    if (gNavMesh->ZoneAndArea(hit, zone, area))
                        ss << "From mesh.  Zone: " << zone << " Area: " << area
                           << "\n";

                    std::vector<float> heights;
                    if (gNavMesh->FindHeights(hit.X, hit.Y, heights))
                    {
                        ss << "Found " << heights.size() << " height values:\n";
                        for (auto const& h : heights)
                            ss << "    " << h << "\n";
                    }
                    else
                        ss << "Found no heights\n";

                    OutputDebugStringA(ss.str().c_str());
                }
            }
            else
            {
                if (!!gMouseDoodad)
                {
                    gNavMesh->AddGameObject(
                        gMouseDoodad->Guid, gMouseDoodad->DisplayId,
                        gMouseDoodad->Position, gMouseDoodad->Rotation);
                    gMouseDoodad.reset();

                    DetourDebugDraw dd(gRenderer.get());
                    dd.Steep(true);
                    duDebugDrawNavMeshPolysWithFlags(
                        &dd, gNavMesh->GetNavMesh(), PolyFlags::Steep, 0);
                    dd.Steep(false);
                    duDebugDrawNavMeshPolysWithoutFlags(
                        &dd, gNavMesh->GetNavMesh(), PolyFlags::Steep, 0);

                    return TRUE;
                }

                if (gRenderer->HitTest(
                        GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam),
                        Renderer::NavMeshGeometryFlag, hit, param))
                {
                    gRenderer->ClearSprites();

                    if (gHasStart)
                    {
                        std::vector<math::Vertex> path;

                        gHasStart = false;

                        if (gNavMesh->FindPath(gStart, hit, path, false))
                            gRenderer->AddPath(path);
                        else
                            MessageBox(nullptr, "FindPath failed", "Path Find",
                                       0);

                        auto const& prev_hop = path[path.size() - 2];
                        float height;
                        if (gNavMesh->FindHeight(prev_hop, hit.X, hit.Y, height))
                        {
                            std::stringstream ss;
                            ss << "Destination: " << hit.X << ", " << hit.Y
                               << ", " << hit.Z
                               << " FindHeight returns: " << height << "\n";

                            std::vector<float> heights;
                            if (gNavMesh->FindHeights(hit.X, hit.Y, heights))
                                for (auto const& h : heights)
                                    ss << "Possible correct values: " << h << "\n";

                            OutputDebugStringA(ss.str().c_str());
                        }
                        else
                            OutputDebugStringA("No height found");
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

                // only if there is movement to avoid an infinite loop of window
                // messages
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

                if (gRenderer->HitTest(
                        GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam),
                        Renderer::CollidableGeometryFlag, hit, param))
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

void InitializeWindows(HINSTANCE hInstance, HWND& guiWindow,
                       HWND& controlWindow)
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
    wc.hIconSm =
        (HICON)LoadImage(GetModuleHandle(nullptr), MAKEINTRESOURCE(IDI_APPICON),
                         IMAGE_ICON, 16, 16, 0);
    wc.hIcon =
        (HICON)LoadImage(GetModuleHandle(nullptr), MAKEINTRESOURCE(IDI_APPICON),
                         IMAGE_ICON, 32, 32, 0);
    // wc.lpszMenuName = MAKEINTRESOURCE(IDR_MENU1);

    RegisterClassEx(&wc);

    RECT wr = {START_X, START_Y, START_X + START_WIDTH, START_Y + START_HEIGHT};
    AdjustWindowRect(&wr, WS_OVERLAPPEDWINDOW, true);

    guiWindow = CreateWindowEx(
        WS_EX_RIGHTSCROLLBAR, "DXWindow", "namigator testing interface",
        WS_OVERLAPPEDWINDOW, wr.left, wr.top, wr.right - wr.left,
        wr.bottom - wr.top, HWND_DESKTOP, nullptr, hInstance, nullptr);

    ZeroMemory(&wc, sizeof(WNDCLASSEX));

    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = DefWindowProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)COLOR_WINDOW;
    wc.lpszClassName = "ControlWindow";
    wc.hIconSm =
        (HICON)LoadImage(GetModuleHandle(nullptr), MAKEINTRESOURCE(IDI_APPICON),
                         IMAGE_ICON, 16, 16, 0);
    wc.hIcon =
        (HICON)LoadImage(GetModuleHandle(nullptr), MAKEINTRESOURCE(IDI_APPICON),
                         IMAGE_ICON, 32, 32, 0);

    RegisterClassEx(&wc);

    controlWindow = CreateWindowEx(
        WS_EX_RIGHTSCROLLBAR, "ControlWindow", "Control",
        (WS_BORDER | WS_CAPTION) & (~WS_ICONIC), wr.right - MAGIC_LEFT_SHIFT,
        wr.top, CONTROL_WIDTH, CONTROL_HEIGHT, HWND_DESKTOP, nullptr, hInstance,
        nullptr);
}

void GetMapName(const std::string& inName, std::string& outName)
{
    outName = inName.substr(inName.find(' ') + 1);
    outName = outName.substr(0, outName.find('('));
    outName.erase(std::remove(outName.begin(), outName.end(), ' '));
}

void LoadAdt(const parser::Adt* adt)
{
    for (int chunkX = 0; chunkX < MeshSettings::ChunksPerAdt; ++chunkX)
        for (int chunkY = 0; chunkY < MeshSettings::ChunksPerAdt; ++chunkY)
        {
            auto const chunk = adt->GetChunk(chunkX, chunkY);

            gRenderer->AddTerrain(chunk->m_terrainVertices,
                                  chunk->m_terrainIndices, chunk->m_areaId);
            gRenderer->AddLiquid(chunk->m_liquidVertices,
                                 chunk->m_liquidIndices);

            for (auto& d : chunk->m_doodadInstances)
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

            for (auto& w : chunk->m_wmoInstances)
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
        dd.Steep(true);
        duDebugDrawNavMeshPolysWithFlags(&dd, gNavMesh->GetNavMesh(),
                                         PolyFlags::Steep, 0);
        dd.Steep(false);
        duDebugDrawNavMeshPolysWithoutFlags(&dd, gNavMesh->GetNavMesh(),
                                            PolyFlags::Steep, 0);
    }
}

void ChangeMap(const std::string& cn)
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
    catch (utility::exception const& e)
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

                    const float cx =
                        (adt->Bounds.MaxCorner.X + adt->Bounds.MinCorner.X) /
                        2.f;
                    const float cy =
                        (adt->Bounds.MaxCorner.Y + adt->Bounds.MinCorner.Y) /
                        2.f;
                    const float cz =
                        (adt->Bounds.MaxCorner.Z + adt->Bounds.MinCorner.Z) /
                        2.f;

                    avgX += cx;
                    avgY += cy;
                    avgZ += cz;

                    if (!looked)
                    {
                        gRenderer->m_camera.Move(cx + 300.f, cy + 300.f,
                                                 cz + 300.f);
                        gRenderer->m_camera.LookAt(cx, cy, cz);
                        looked = true;
                    }
                }

        avgX /= count;
        avgY /= count;
        avgZ /= count;

        //        gRenderer->m_camera.Move(avgX + 300.f, avgY + 300.f, avgZ +
        //        300.f); gRenderer->m_camera.LookAt(avgX, avgY, avgZ);
    }

    // if the loaded map has no ADTs, but instead a global WMO, load it now,
    // including all mesh tiles
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

        auto const cx =
            (wmo->Bounds.MaxCorner.X + wmo->Bounds.MinCorner.X) / 2.f;
        auto const cy =
            (wmo->Bounds.MaxCorner.Y + wmo->Bounds.MinCorner.Y) / 2.f;
        auto const cz =
            (wmo->Bounds.MaxCorner.Z + wmo->Bounds.MinCorner.Z) / 2.f;

        gRenderer->m_camera.Move(cx + 300.f, cy + 300.f, cz + 300.f);
        gRenderer->m_camera.LookAt(cx, cy, cz);

        gControls->Enable(Controls::PositionX, false);
        gControls->Enable(Controls::PositionY, false);
        gControls->Enable(Controls::Load, false);

        if (gNavMesh)
        {
            DetourDebugDraw dd(gRenderer.get());
            dd.Steep(true);
            duDebugDrawNavMeshPolysWithFlags(&dd, gNavMesh->GetNavMesh(),
                                             PolyFlags::Steep, 0);
            dd.Steep(false);
            duDebugDrawNavMeshPolysWithoutFlags(&dd, gNavMesh->GetNavMesh(),
                                                PolyFlags::Steep, 0);
        }
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
    if (gControls->GetText(Controls::PositionX).find('.') !=
            std::string::npos ||
        gControls->GetText(Controls::PositionY).find('.') != std::string::npos)
    {
        auto const posX = std::stof(gControls->GetText(Controls::PositionX));
        auto const posY = std::stof(gControls->GetText(Controls::PositionY));

        math::Convert::WorldToAdt({posX, posY, 0.f}, x, y);
    }
    else
    {
        auto const intX = std::stoi(gControls->GetText(Controls::PositionX));
        auto const intY = std::stoi(gControls->GetText(Controls::PositionY));

        // if either is negative, it is a world coordinate, or
        // if either is greater than or equal to 64, it is a world coordinate
        if (intX < 0 || intY < 0 || intX >= MeshSettings::Adts ||
            intY >= MeshSettings::Adts)
            math::Convert::WorldToAdt(
                {static_cast<float>(intX), static_cast<float>(intY), 0.f}, x,
                y);
        // otherwise, it is an adt coordinate
        else
        {
            x = intX;
            y = intY;
        }
    }

    // if both the x and y values are integers less than 64, assume the
    // coordinates are ADT coordinates
    if (x < MeshSettings::Adts && y < MeshSettings::Adts && x >= 0 && y >= 0)
    {
        if (!gMap->HasAdt(x, y))
        {
            // this ADT does not exist.  search for one that does, and offer it
            // instead
            int closestX = 0, closestY = 0, closestDist = 64 * 64 + 1;
            for (auto newY = 0; newY < MeshSettings::Adts; ++newY)
                for (auto newX = 0; newX < MeshSettings::Adts; ++newX)
                {
                    if (!gMap->HasAdt(newX, newY))
                        continue;

                    auto const dist = (newX - closestX) * (newX - closestX) +
                                      (newY - closestY) * (newY - closestY);
                    if (closestX < 0 || closestY < 0 || dist < closestDist)
                    {
                        closestX = newX;
                        closestY = newY;
                        closestDist = dist;
                    }
                }

            std::stringstream str;
            str << "Map does not have ADT (" << x << ", " << y
                << ").  Nearest available is (" << closestX << ", " << closestY
                << ").  Load instead?";
            auto const result = MessageBox(nullptr, str.str().c_str(), "Error",
                                           MB_YESNO | MB_ICONEXCLAMATION);
            if (result != IDYES)
                return;

            x = closestX;
            y = closestY;
        }

        try
        {
            auto const adt = gMap->GetAdt(x, y);

            LoadAdt(adt);

            const float cx =
                (adt->Bounds.MaxCorner.X + adt->Bounds.MinCorner.X) / 2.f;
            const float cy =
                (adt->Bounds.MaxCorner.Y + adt->Bounds.MinCorner.Y) / 2.f;
            const float cz =
                (adt->Bounds.MaxCorner.Z + adt->Bounds.MinCorner.Z) / 2.f;

            gRenderer->m_camera.Move(cx + 300.f, cy + 300.f, cz + 300.f);
            gRenderer->m_camera.LookAt(cx, cy, cz);
        }
        catch (const std::exception& e)
        {
            MessageBox(nullptr, e.what(), "Error loading ADT",
                       MB_OK | MB_ICONERROR);
        }
    }
}

void SearchZValues()
{
    auto const posX = std::stof(gControls->GetText(Controls::PositionX));
    auto const posY = std::stof(gControls->GetText(Controls::PositionY));

    std::vector<float> output;
    if (!gNavMesh->FindHeights(posX, posY, output))
    {
        MessageBoxA(nullptr, "FindHeights failed", "Z Search Results", 0);
        return;
    }

    gRenderer->ClearSprites();

    std::stringstream result;

    result << "Heights at (" << posX << ", " << posY << "):\n";
    for (auto const h : output)
    {
        result << h << "\n";
        gRenderer->AddSphere({posX, posY, h}, 0.25f);
    }

    MessageBoxA(nullptr, result.str().c_str(), "Z Search Results", 0);
}

void SpawnGOFromGUI()
{
    if (!gNavMesh)
        return;

    auto const displayId =
        std::stoi(gControls->GetText(Controls::SpawnDoodadEdit));
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
} // namespace

// the entry point for any Windows program
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/,
                   LPSTR lpCmdLine, int nCmdShow)
{
    int argc;
    auto argv = ::CommandLineToArgvW(GetCommandLineW(), &argc);

    if (!argv || argc != 3)
    {
        MessageBox(nullptr, "Usage: <nav data> <wow data>", "ERROR",
                   MB_ICONERROR);
        return EXIT_FAILURE;
    }

    gNavData = fs::path(argv[1]);

    if (!fs::is_directory(gNavData))
    {
        MessageBox(nullptr, "Navigation data directory not found", "ERROR",
                   MB_ICONERROR);
        return EXIT_FAILURE;
    }

    const fs::path wow_path(argv[2]);

    if (!fs::is_directory(wow_path))
    {
        MessageBox(nullptr, "Wow data directory not found", "ERROR",
                   MB_ICONERROR);
        return EXIT_FAILURE;
    }

    gHasStart = false;

    parser::sMpqManager.Initialize(wow_path.string());

    InitializeWindows(hInstance, gGuiWindow, gControlWindow);

    ShowWindow(gGuiWindow, nCmdShow);
    ShowWindow(gControlWindow, nCmdShow);

    // set up and initialize Direct3D
    gRenderer = std::make_unique<Renderer>(gGuiWindow);

    // set up and initialize our Windows common control API for the control
    // window
    gControls = std::make_unique<CommonControl>(gControlWindow);

    gControls->AddLabel("Select Map:", 10, 12);

    std::vector<std::string> maps;
    maps.emplace_back("000 Azeroth");
    maps.emplace_back("001 Kalimdor");
    maps.emplace_back("013 Test");
    maps.emplace_back("025 Scott Test");
    maps.emplace_back("029 Test");
    maps.emplace_back("030 PVPZone01 (Alterac Valley)");
    maps.emplace_back("033 Shadowfang");
    maps.emplace_back("034 StormwindJail (Stockades)");
    maps.emplace_back("036 DeadminesInstance");
    maps.emplace_back("037 PVPZone02 (Azshara Crater)");
    maps.emplace_back("043 WailingCaverns");
    maps.emplace_back("090 GnomeragonInstance");
    maps.emplace_back("229 BlackrockSpire");
    maps.emplace_back("429 DireMaul");
    maps.emplace_back("451 Development");
    maps.emplace_back("489 PVPzone03 (Warsong Gulch)");
    maps.emplace_back("529 PVPzone04 (Arathi Basin)");
    maps.emplace_back("530 Expansion01 (Outland)");
    maps.emplace_back("562 bladesedgearena");
    maps.emplace_back("571 Northrend");
    maps.emplace_back("603 UlduarRaid");

    gControls->AddComboBox(Controls::MapsCombo, maps, 115, 10, ChangeMap);

    gControls->AddLabel("X:", 10, 35);
    gControls->AddTextBox(Controls::PositionX, "-8925", 25, 35, 75, 20);

    gControls->AddLabel("Y:", 10, 60);
    gControls->AddTextBox(Controls::PositionY, "-120", 25, 60, 75, 20);

    gControls->AddButton(Controls::Load, "Load", 115, 57, 75, 25,
                         LoadPositionFromGUI);
    gControls->AddButton(Controls::ZSearch, "Z Search", 200, 57, 75, 25,
                         SearchZValues);

    gControls->Enable(Controls::PositionX, false);
    gControls->Enable(Controls::PositionY, false);
    gControls->Enable(Controls::Load, false);

    gControls->AddCheckBox(Controls::Wireframe, "Wireframe", 10, 85, false,
                           [](bool checked)
                           { gRenderer->SetWireframe(checked); });
    gControls->AddCheckBox(Controls::RenderADT, "Render ADT", 10, 110, true,
                           [](bool checked)
                           { gRenderer->SetRenderADT(checked); });
    gControls->AddCheckBox(
        Controls::RenderLiquid, "Render Liquid", 10, 135, true,
        [](bool checked) { gRenderer->SetRenderLiquid(checked); });
    gControls->AddCheckBox(Controls::RenderWMO, "Render WMO", 10, 160, true,
                           [](bool checked)
                           { gRenderer->SetRenderWMO(checked); });
    gControls->AddCheckBox(
        Controls::RenderDoodad, "Render Doodad", 10, 185, true,
        [](bool checked) { gRenderer->SetRenderDoodad(checked); });
    gControls->AddCheckBox(Controls::RenderMesh, "Render Mesh", 10, 210, true,
                           [](bool checked)
                           { gRenderer->SetRenderMesh(checked); });

    gControls->AddTextBox(Controls::SpawnDoodadEdit, "Display ID", 10, 245, 90,
                          20);
    gControls->AddButton(Controls::SpawnDoodadButton, "Spawn GO", 115, 242, 100,
                         25, SpawnGOFromGUI);

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