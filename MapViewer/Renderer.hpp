#pragma once

#include "Camera.hpp"
#include "LinearAlgebra.hpp"

#include <vector>
#include <memory>
#include <Windows.h>
#include <d3d11.h>
#include <set>

class Renderer
{
    private:
        static const float TerrainColor[4];
        static const float LiquidColor[4];
        static const float WmoColor[4];
        static const float DoodadColor[4];
        static const float BackgroundColor[4];

        struct ColoredVertex
        {
            utility::Vertex vertex;
            float nx, ny, nz;
            float color[4];
        };

        struct GeometryBuffer
        {
            ID3D11Buffer *VertexBuffer;
            ID3D11Buffer *IndexBuffer;
            unsigned int IndexCount;
        };

        const HWND m_window;

        IDXGISwapChain *m_swapChain;            // the pointer to the swap chain interface
        ID3D11Device *m_device;                 // the pointer to our Direct3D device interface
        ID3D11DeviceContext *m_deviceContext;   // the pointer to our Direct3D device context
        ID3D11RenderTargetView *m_backBuffer;   // the pointer to our back buffer
        ID3D11InputLayout *m_inputLayout;
        ID3D11VertexShader *m_vertexShader;
        ID3D11PixelShader *m_pixelShader;
        ID3D11Buffer *m_cbPerObjectBuffer;
        ID3D11RasterizerState *m_rasterizerState;
        ID3D11DepthStencilView *m_depthStencilView;
        ID3D11Texture2D *m_depthStencilBuffer;

        std::vector<GeometryBuffer> m_terrainBuffers;
        std::vector<GeometryBuffer> m_liquidBuffers;
        std::vector<GeometryBuffer> m_wmoBuffers;
        std::vector<GeometryBuffer> m_doodadBuffers;

        std::set<unsigned int> m_wmos;
        std::set<unsigned int> m_doodads;

        void InsertBuffer(std::vector<GeometryBuffer> &buffer, const float *color, const std::vector<utility::Vertex> &vertices, const std::vector<int> &indices);

    public:
        Renderer(HWND window);
        ~Renderer();

        void AddTerrain(const std::vector<utility::Vertex> &vertices, const std::vector<int> &indices);
        void AddLiquid(const std::vector<utility::Vertex> &vertices, const std::vector<int> &indices);
        void AddWmo(unsigned int id, const std::vector<utility::Vertex> &vertices, const std::vector<int> &indices);
        void AddDoodad(unsigned int id, const std::vector<utility::Vertex> &vertices, const std::vector<int> &indices);

        void Render() const;

        Camera m_camera;
};