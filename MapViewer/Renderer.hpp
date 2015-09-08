#pragma once

#include <vector>
#include <Windows.h>
#include <d3d11.h>
#include <d3dx11.h>

struct VERTEX
{
    float x, y, z;
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

        float m_viewProjectionMatrix[16];

        ID3D11Buffer *m_vertexBuffer;
        ID3D11Buffer *m_indexBuffer;
        unsigned int m_vertexCount;
        unsigned int m_indexCount;

        void InitializePipeline();

    public:
        Renderer(HWND window);
        ~Renderer();

        void InitializeVertexBuffer(const std::vector<VERTEX> &vertices, const std::vector<int> &indices);
        void Render();
};