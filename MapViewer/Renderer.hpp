#pragma once

#include <vector>
#include <memory>
#include <Windows.h>
#include <d3d11.h>

#include "Camera.hpp"

struct ColoredVertex
{
    float x, y, z;
    float nx, ny, nz;
    float color[4];
};

class Renderer
{
    private:
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

        std::vector<ID3D11Buffer *> m_vertexBuffers;
        std::vector<ID3D11Buffer *> m_indexBuffers;
        std::vector<unsigned int> m_indexCounts;

    public:
        Renderer(HWND window);
        ~Renderer();

        void AddGeometry(const std::vector<ColoredVertex> &vertices, const std::vector<int> &indices);
        void Render() const;

        Camera m_camera;
};