#include <vector>
#include <limits>
#include <sstream>

#define NOMINMAX    // for some reason, these macros interfere with numerical_limits?

#include <d3d11.h>
#include <d3dx11.h>

#include "Renderer.hpp"
#include "LinearAlgebra.hpp"

// include the Direct3D Library file
#pragma comment (lib, "d3d11.lib")
#pragma comment (lib, "d3dx11.lib")

#include "pixelShader.hpp"
#include "vertexShader.hpp"

#define ZERO(x) memset(&x, 0, sizeof(decltype(x)))

#define WIREFRAME

Renderer::Renderer(HWND window) : m_window(window)
{
    // create a struct to hold information about the swap chain
    DXGI_SWAP_CHAIN_DESC scd;

    // clear out the struct for use
    ZERO(scd);

    // fill the swap chain description struct
    scd.BufferCount = 1;                                    // one back buffer
    scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;     // use 32-bit color
    scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;      // how swap chain is to be used
    scd.OutputWindow = window;                              // the window to be used
    scd.SampleDesc.Count = 1;                               // how many multisamples
    scd.SampleDesc.Quality = 0;                             // multisample quality level
    scd.Windowed = TRUE;                                    // windowed/full-screen mode

    // create a device, device context and swap chain using the information in the scd struct
    D3D11CreateDeviceAndSwapChain(NULL,
        D3D_DRIVER_TYPE_HARDWARE,
        NULL,
#ifdef _DEBUG
        D3D11_CREATE_DEVICE_DEBUG,
#else
        NULL,
#endif
        nullptr,
        NULL,
        D3D11_SDK_VERSION,
        &scd,
        &m_swapChain,
        &m_device,
        nullptr,
        &m_deviceContext);


    // get the address of the back buffer
    ID3D11Texture2D *backBuffer;
    m_swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<LPVOID *>(&backBuffer));

    // use the back buffer address to create the render target
    m_device->CreateRenderTargetView(backBuffer, NULL, &m_backBuffer);
    backBuffer->Release();

    // set the render target as the back buffer
    m_deviceContext->OMSetRenderTargets(1, &m_backBuffer, NULL);

    // Set the viewport
    D3D11_VIEWPORT viewport;
    ZERO(viewport);

    RECT wr;
    GetWindowRect(window, &wr);

    viewport.TopLeftX = 0.f;
    viewport.TopLeftY = 0.f;
    viewport.Width = static_cast<float>(wr.right - wr.left);
    viewport.Height = static_cast<float>(wr.bottom - wr.top);

    m_deviceContext->RSSetViewports(1, &viewport);

    InitializePipeline();

    D3D11_BUFFER_DESC cbbd;
    ZERO(cbbd);

    cbbd.Usage = D3D11_USAGE_DEFAULT;
    cbbd.ByteWidth = sizeof(m_viewProjectionMatrix);
    cbbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    cbbd.CPUAccessFlags = 0;
    cbbd.MiscFlags = 0;

    m_device->CreateBuffer(&cbbd, nullptr, &m_cbPerObjectBuffer);

    D3D11_RASTERIZER_DESC rasterizerDesc;
    ZERO(rasterizerDesc);

#ifdef WIREFRAME
    rasterizerDesc.FillMode = D3D11_FILL_WIREFRAME;
#else
    rasterizerDesc.FillMode = D3D11_FILL_SOLID;
#endif

    rasterizerDesc.CullMode = D3D11_CULL_NONE;

    m_device->CreateRasterizerState(&rasterizerDesc, &m_rasterizerState);
    m_deviceContext->RSSetState(m_rasterizerState);
}

Renderer::~Renderer()
{
    m_swapChain->Release();
    m_backBuffer->Release();
    m_device->Release();
    m_deviceContext->Release();
    m_cbPerObjectBuffer->Release();
    m_rasterizerState->Release();
}

void Renderer::InitializePipeline()
{
    m_device->CreateVertexShader(g_VShader, sizeof(g_VShader), nullptr, &m_vertexShader);
    m_device->CreatePixelShader(g_PShader, sizeof(g_PShader), nullptr, &m_pixelShader);

    m_deviceContext->VSSetShader(m_vertexShader, nullptr, 0);
    m_deviceContext->PSSetShader(m_pixelShader, nullptr, 0);

    D3D11_INPUT_ELEMENT_DESC ied[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };

    m_device->CreateInputLayout(ied, 2, g_VShader, sizeof(g_VShader), &m_inputLayout);
    m_deviceContext->IASetInputLayout(m_inputLayout);
}

void Renderer::InitializeVertexBuffer(const std::vector<VERTEX> &vertices, const std::vector<int> &indices)
{
    D3D11_BUFFER_DESC vertexBufferDesc, indexBufferDesc;
    
    ZERO(vertexBufferDesc);

    vertexBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    vertexBufferDesc.ByteWidth = sizeof(VERTEX) * vertices.size();
    vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vertexBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    m_device->CreateBuffer(&vertexBufferDesc, nullptr, &m_vertexBuffer);

    D3D11_MAPPED_SUBRESOURCE ms;
    m_deviceContext->Map(m_vertexBuffer, NULL, D3D11_MAP_WRITE_DISCARD, NULL, &ms);
    memcpy(ms.pData, &vertices[0], sizeof(VERTEX)*vertices.size());
    m_deviceContext->Unmap(m_vertexBuffer, NULL);

    ZERO(indexBufferDesc);

    indexBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    indexBufferDesc.ByteWidth = sizeof(int) * indices.size();
    indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
    indexBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    m_device->CreateBuffer(&indexBufferDesc, nullptr, &m_indexBuffer);

    m_deviceContext->Map(m_indexBuffer, NULL, D3D11_MAP_WRITE_DISCARD, NULL, &ms);
    memcpy(ms.pData, &indices[0], sizeof(int)*indices.size());
    m_deviceContext->Unmap(m_indexBuffer, NULL);

    float minX = std::numeric_limits<float>::max(), maxX = std::numeric_limits<float>::lowest(),
          minY = std::numeric_limits<float>::max(), maxY = std::numeric_limits<float>::lowest(),
          minZ = std::numeric_limits<float>::max(), maxZ = std::numeric_limits<float>::lowest();

    for (unsigned int i = 0; i < vertices.size(); ++i)
    {
        if (vertices[i].x < minX)
            minX = vertices[i].x;
        if (vertices[i].x > maxX)
            maxX = vertices[i].x;

        if (vertices[i].y < minY)
            minY = vertices[i].y;
        if (vertices[i].y > maxY)
            maxY = vertices[i].y;

        if (vertices[i].z < minZ)
            minZ = vertices[i].z;
        if (vertices[i].z > maxZ)
            maxZ = vertices[i].z;
    }

    const float averageX = (minX + maxX) / 2.f;
    const float averageY = (minY + maxY) / 2.f;
    const float aspect = 1200.f / 800.f;

    const utility::Vertex up(0.f, 1.f, 0.f);

    auto view = utility::Matrix<float>::CreateViewMatrix({ averageX, averageY, maxZ + 100.f }, { averageX, averageY, maxZ}, up);
    auto projection = utility::Matrix<float>::CreateProjectionMatrix(PI/4.f, aspect, 0.1f, 10000.f);
    auto viewProjection = view*projection;

    viewProjection.PopulateArray(m_viewProjectionMatrix);

    m_vertexCount = vertices.size();
    m_indexCount = indices.size();
}

void Renderer::Render()
{
    const float blue[4] = { 0.f, 0.2f, 0.4f, 1.f };

    // clear the back buffer to a deep blue
    m_deviceContext->ClearRenderTargetView(m_backBuffer, blue);

    m_deviceContext->UpdateSubresource(m_cbPerObjectBuffer, 0, nullptr, &m_viewProjectionMatrix, 0, 0);
    m_deviceContext->VSSetConstantBuffers(0, 1, &m_cbPerObjectBuffer);

    unsigned int stride = sizeof(VERTEX);
    unsigned int offset = 0;

    m_deviceContext->IASetVertexBuffers(0, 1, &m_vertexBuffer, &stride, &offset);
    m_deviceContext->IASetIndexBuffer(m_indexBuffer, DXGI_FORMAT_R32_UINT, 0);
    m_deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    m_deviceContext->DrawIndexed(m_indexCount, 0, 0);
    //m_deviceContext->Draw(m_vertexCount, 0);

    // switch the back buffer and the front buffer
    m_swapChain->Present(0, 0);
}