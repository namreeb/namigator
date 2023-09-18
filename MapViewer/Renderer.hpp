#pragma once

#include "Camera.hpp"
#include "utility/Vector.hpp"

#include <Windows.h>
#include <atlbase.h>
#include <d3d11.h>
#include <dxgi1_3.h>
#include <memory>
#include <unordered_set>
#include <vector>

class Renderer
{
public:
    enum Geometry
    {
        TerrainGeometry,
        LiquidGeometry,
        WmoGeometry,
        DoodadGeometry,
        NavMeshGeometry,
        SphereGeometry,
        LineGeometry,
        ArrowGeometry,
        GameObjectGeometry,
        NumGeometryBuffers,
    };

    enum GeometryFlags
    {
        TerrainGeometryFlag = 1 << TerrainGeometry,
        LiquidGeometryFlag = 1 << LiquidGeometry,
        WmoGeometryFlag = 1 << WmoGeometry,
        DoodadGeometryFlag = 1 << DoodadGeometry,
        NavMeshGeometryFlag = 1 << NavMeshGeometry,
        SphereGeometryFlag = 1 << SphereGeometry,
        LineGeometryFlag = 1 << LineGeometry,
        ArrowGeometryFlag = 1 << ArrowGeometry,
        GameObjectGeometryFlag = 1 << GameObjectGeometry,

        CollidableGeometryFlag =
            TerrainGeometryFlag | WmoGeometryFlag | DoodadGeometryFlag,
    };

private:
    static constexpr float LiquidColor[] = {0.25f, 0.28f, 0.9f, 0.5f};
    static constexpr float WmoColor[] = {1.f, 0.95f, 0.f, 1.f};
    static constexpr float DoodadColor[] = {1.f, 0.f, 0.f, 1.f};
    static constexpr float BackgroundColor[] = {0.6f, 0.55f, 0.55f, 1.f};
    static constexpr float SphereColor[] = {1.f, 0.5f, 0.25f, 0.75f};
    static constexpr float LineColor[] = {0.5f, 0.25f, 0.0f, 1.f};
    static constexpr float ArrowColor[] = {0.5f, 0.25f, 0.0f, 1.f};
    static constexpr float GameObjectColor[] = {0.8f, 0.5f, 0.1f, 1.f};
    static constexpr float MeshColor[] = {1.f, 1.f, 1.f, 0.75f};
    static constexpr float MeshSteepColor[] = {0.3f, 0.3f, 0.3f, 0.75f};

    struct ColoredVertex
    {
        math::Vertex vertex;
        float nx, ny, nz;
        float color[4];
    };

    struct GeometryBuffer
    {
        std::uint32_t UserParameter;

        std::vector<ColoredVertex> VertexBufferCpu;
        std::vector<int> IndexBufferCpu;

        CComPtr<ID3D11Buffer> VertexBufferGpu;
        CComPtr<ID3D11Buffer> IndexBufferGpu;
    };

    const HWND m_window;

    CComPtr<IDXGIFactory2> m_dxgiFactory;
    CComPtr<IDXGISwapChain1>
        m_swapChain; // the pointer to the swap chain interface
    CComPtr<ID3D11Device>
        m_device; // the pointer to our Direct3D device interface
    CComPtr<ID3D11DeviceContext>
        m_deviceContext; // the pointer to our Direct3D device context
    CComPtr<ID3D11RenderTargetView>
        m_backBuffer; // the pointer to our back buffer
    CComPtr<ID3D11InputLayout> m_inputLayout;
    CComPtr<ID3D11VertexShader> m_vertexShader;
    CComPtr<ID3D11PixelShader> m_pixelShader;
    CComPtr<ID3D11Buffer> m_cbPerObjectBuffer;
    CComPtr<ID3D11RasterizerState> m_rasterizerState;
    CComPtr<ID3D11RasterizerState> m_rasterizerStateNoCull;
    CComPtr<ID3D11RasterizerState> m_rasterizerStateWireframe;
    CComPtr<ID3D11DepthStencilView> m_depthStencilView;
    CComPtr<ID3D11DepthStencilState> m_depthStencilState;
    CComPtr<ID3D11Texture2D> m_depthStencilBuffer;
    CComPtr<ID3D11BlendState> m_liquidBlendState;
    CComPtr<ID3D11BlendState> m_opaqueBlendState;

    std::vector<GeometryBuffer> m_buffers[NumGeometryBuffers];

    std::unordered_set<unsigned int> m_wmos;
    std::unordered_set<unsigned int> m_doodads;

    /// TODO: Replace with geometry flags
    bool m_renderADT;
    bool m_renderLiquid;
    bool m_renderWMO;
    bool m_renderDoodad;
    bool m_renderMesh;
    bool m_renderPathfind;

    bool m_wireframeEnabled;

    void InsertBuffer(std::vector<GeometryBuffer>& buffer, const float* color,
                      const std::vector<math::Vertex>& vertices,
                      const std::vector<int>& indices,
                      std::uint32_t userParam = 0, bool genNormals = true);

public:
    Renderer(HWND window);

    void ClearBuffers(Geometry type);
    void ClearBuffers();
    void ClearSprites();
    void ClearGameObjects();

    void AddTerrain(const std::vector<math::Vertex>& vertices,
                    const std::vector<int>& indices, std::uint32_t areaId);
    void AddLiquid(const std::vector<math::Vertex>& vertices,
                   const std::vector<int>& indices);
    void AddWmo(unsigned int id, const std::vector<math::Vertex>& vertices,
                const std::vector<int>& indices);
    void AddDoodad(unsigned int id, const std::vector<math::Vertex>& vertices,
                   const std::vector<int>& indices);
    void AddMesh(const std::vector<math::Vertex>& vertices,
                 const std::vector<int>& indices, bool steep);
    void AddLines(const std::vector<math::Vertex>& vertices,
                  const std::vector<int>& indices);
    void AddSphere(const math::Vertex& position, float size,
                   int recursionLevel = 2);
    void AddArrows(const math::Vertex& start, const math::Vertex& end,
                   float step);
    void AddPath(const std::vector<math::Vertex>& path);
    void AddGameObject(const std::vector<math::Vertex>& vertices,
                       const std::vector<int>& indices);

    bool HasWmo(unsigned int id) const;
    bool HasDoodad(unsigned int id) const;

    void Render();

    void SetWireframe(bool enabled);
    void SetRenderADT(bool enabled) { m_renderADT = enabled; }
    void SetRenderLiquid(bool enabled) { m_renderLiquid = enabled; }
    void SetRenderWMO(bool enabled) { m_renderWMO = enabled; }
    void SetRenderDoodad(bool enabled) { m_renderDoodad = enabled; }
    void SetRenderMesh(bool enabled) { m_renderMesh = enabled; }

    bool HitTest(int x, int y, std::uint32_t geometryFlags, math::Vertex& out,
                 std::uint32_t& param) const;

    Camera m_camera;
};