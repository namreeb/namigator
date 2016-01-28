#include <vector>
#include <cassert>
#include <set>

#include <d3d11.h>
#include <dxgi1_3.h>

#include "Renderer.hpp"
#include "utility/Include/LinearAlgebra.hpp"

// include the Direct3D Library file
#pragma comment (lib, "d3d11.lib")
#pragma comment (lib, "dxgi.lib")

#include "pixelShader.hpp"
#include "vertexShader.hpp"

#define ZERO(x) memset(&x, 0, sizeof(decltype(x)))
#define ThrowIfFail(x) if (FAILED(x)) throw "Renderer initialization error"

const float Renderer::TerrainColor[4]       = { 0.1f, 0.8f, 0.3f, 1.f };
const float Renderer::LiquidColor[4]        = { 0.25f, 0.28f, 0.9f, 0.5f };
const float Renderer::WmoColor[4]           = { 1.f, 0.95f, 0.f, 1.f };
const float Renderer::DoodadColor[4]        = { 1.f, 0.f, 0.f, 1.f };
const float Renderer::MeshColor[4]          = { 1.f, 1.f, 1.f, 0.75f };
const float Renderer::BackgroundColor[4]    = { 0.f, 0.2f, 0.4f, 1.f };

Renderer::Renderer(HWND window) : m_window(window), m_renderADT(true), m_renderLiquid(true), m_renderWMO(true), m_renderDoodad(true), m_renderMesh(true)
{
    RECT wr;
    GetWindowRect(window, &wr);

    // create a struct to hold information about the swap chain
    DXGI_SWAP_CHAIN_DESC1 scd;

    // clear out the struct for use
    ZERO(scd);

    // fill the swap chain description struct
    scd.BufferCount = 1;                                    // one back buffer
    scd.Format = DXGI_FORMAT_R8G8B8A8_UNORM;                // use 32-bit color
    scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;      // how swap chain is to be used
    scd.Width = wr.right - wr.left;
    scd.Height = wr.bottom - wr.top;
    scd.SampleDesc.Count = 1;                               // how many multisamples
    scd.SampleDesc.Quality = 0;                             // multisample quality level
    scd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    scd.Scaling = DXGI_SCALING_STRETCH;

    ThrowIfFail(D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, nullptr, 0, D3D11_SDK_VERSION, &m_device, nullptr, &m_deviceContext));

    UINT quality = 0;
    if (SUCCEEDED(m_device->CheckMultisampleQualityLevels(scd.Format, 8, &quality))) {
        scd.SampleDesc.Count = 8;
        scd.SampleDesc.Quality = quality - 1;
    }

    ThrowIfFail(CreateDXGIFactory2(0, IID_PPV_ARGS(&m_dxgiFactory)));
    ThrowIfFail(m_dxgiFactory->CreateSwapChainForHwnd(m_device, m_window, &scd, NULL, NULL, &m_swapChain));

    // create a device, device context and swap chain using the information in the scd struct
    //ThrowIfFail(D3D11CreateDeviceAndSwapChain(nullptr,
    //    D3D_DRIVER_TYPE_HARDWARE,
    //    nullptr,
    //    0,
    //    nullptr,
    //    0,
    //    D3D11_SDK_VERSION,
    //    &scd,
    //    &m_swapChain,
    //    &m_device,
    //    nullptr,
    //    &m_deviceContext));

    //assert(m_swapChain);

    // get the address of the back buffer
    ID3D11Texture2D *backBuffer;
    ThrowIfFail(m_swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<LPVOID *>(&backBuffer)));

    // use the back buffer address to create the render target
    ThrowIfFail(m_device->CreateRenderTargetView(backBuffer, nullptr, &m_backBuffer));
    backBuffer->Release();

    // Set the viewport
    D3D11_VIEWPORT viewport;
    ZERO(viewport);

    viewport.TopLeftX = 0.f;
    viewport.TopLeftY = 0.f;
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.f;
    viewport.Width = static_cast<float>(wr.right - wr.left);
    viewport.Height = static_cast<float>(wr.bottom - wr.top);

    m_deviceContext->RSSetViewports(1, &viewport);

    ThrowIfFail(m_device->CreateVertexShader(g_VShader, sizeof(g_VShader), nullptr, &m_vertexShader));
    ThrowIfFail(m_device->CreatePixelShader(g_PShader, sizeof(g_PShader), nullptr, &m_pixelShader));

    m_deviceContext->VSSetShader(m_vertexShader, nullptr, 0);
    m_deviceContext->PSSetShader(m_pixelShader, nullptr, 0);

    D3D11_INPUT_ELEMENT_DESC ied[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };

    ThrowIfFail(m_device->CreateInputLayout(ied, _countof(ied), g_VShader, sizeof(g_VShader), &m_inputLayout));
    m_deviceContext->IASetInputLayout(m_inputLayout);

    D3D11_BUFFER_DESC cbbd;
    ZERO(cbbd);

    cbbd.Usage = D3D11_USAGE_DEFAULT;
    cbbd.ByteWidth = 32 * sizeof(float);
    cbbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    cbbd.CPUAccessFlags = 0;
    cbbd.MiscFlags = 0;

    ThrowIfFail(m_device->CreateBuffer(&cbbd, nullptr, &m_cbPerObjectBuffer));

    SetWireframe(false);

    D3D11_TEXTURE2D_DESC dsdtex;
    ZERO(dsdtex);

    dsdtex.Width = wr.right - wr.left;
    dsdtex.Height = wr.bottom - wr.top;
    dsdtex.MipLevels = 1;
    dsdtex.ArraySize = 1;
    dsdtex.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    dsdtex.SampleDesc = scd.SampleDesc;
    dsdtex.Usage = D3D11_USAGE_DEFAULT;
    dsdtex.BindFlags = D3D11_BIND_DEPTH_STENCIL;
    dsdtex.CPUAccessFlags = 0;
    dsdtex.MiscFlags = 0;

    ThrowIfFail(m_device->CreateTexture2D(&dsdtex, nullptr, &m_depthStencilBuffer));
    ThrowIfFail(m_device->CreateDepthStencilView(m_depthStencilBuffer, nullptr, &m_depthStencilView));

    D3D11_DEPTH_STENCIL_DESC dsd;
    ZERO(dsd);

    dsd.DepthEnable = TRUE;
    dsd.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    dsd.DepthFunc = D3D11_COMPARISON_LESS;
    dsd.StencilEnable = FALSE;

    ThrowIfFail(m_device->CreateDepthStencilState(&dsd, &m_depthStencilState));

    m_deviceContext->OMSetDepthStencilState(m_depthStencilState, 0);
    ID3D11RenderTargetView * pBackBuffer[] = { m_backBuffer };
    m_deviceContext->OMSetRenderTargets(1, pBackBuffer, m_depthStencilView);

    D3D11_BLEND_DESC bd;
    ZERO(bd);

    bd.RenderTarget[0].BlendEnable = TRUE;
    bd.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
    bd.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
    bd.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    bd.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
    bd.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    bd.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    bd.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    ThrowIfFail(m_device->CreateBlendState(&bd, &m_liquidBlendState));

    bd.RenderTarget[0].BlendEnable = FALSE;
    ThrowIfFail(m_device->CreateBlendState(&bd, &m_opaqueBlendState));
}

void Renderer::ClearBuffers()
{
    m_terrainBuffers.clear();
    m_liquidBuffers.clear();
    m_wmoBuffers.clear();
    m_doodadBuffers.clear();
    m_meshBuffers.clear();
}

void Renderer::AddTerrain(const std::vector<utility::Vertex> &vertices, const std::vector<int> &indices)
{
    InsertBuffer(m_terrainBuffers, TerrainColor, vertices, indices);
}

void Renderer::AddLiquid(const std::vector<utility::Vertex> &vertices, const std::vector<int> &indices)
{
    InsertBuffer(m_liquidBuffers, LiquidColor, vertices, indices);
}

void Renderer::AddWmo(unsigned int id, const std::vector<utility::Vertex> &vertices, const std::vector<int> &indices)
{
    // if we are already rendering this wmo, do nothing
    if (m_wmos.find(id) != m_wmos.end())
        return;

    m_wmos.insert(id);

    InsertBuffer(m_wmoBuffers, WmoColor, vertices, indices);
}

void Renderer::AddDoodad(unsigned int id, const std::vector<utility::Vertex> &vertices, const std::vector<int> &indices)
{
    // if we are already rendering this doodad, do nothing
    if (m_doodads.find(id) != m_doodads.end())
        return;

    m_doodads.insert(id);

    InsertBuffer(m_doodadBuffers, DoodadColor, vertices, indices);
}

void Renderer::AddMesh(const std::vector<utility::Vertex> &vertices, const std::vector<int> &indices)
{
    InsertBuffer(m_meshBuffers, MeshColor, vertices, indices);
}

bool Renderer::HasWmo(unsigned int id) const
{
    return m_wmos.find(id) != m_wmos.end();
}

bool Renderer::HasDoodad(unsigned int id) const
{
    return m_doodads.find(id) != m_doodads.end();
}

void Renderer::InsertBuffer(std::vector<GeometryBuffer> &buffer, const float *color, const std::vector<utility::Vertex> &vertices, const std::vector<int> &indices)
{
    if (!vertices.size() || !indices.size())
        return;

    D3D11_BUFFER_DESC vertexBufferDesc, indexBufferDesc;
    
    ZERO(vertexBufferDesc);

    vertexBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    vertexBufferDesc.ByteWidth = sizeof(ColoredVertex) * vertices.size();
    vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vertexBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    ID3D11Buffer *vertexBuffer;

    ThrowIfFail(m_device->CreateBuffer(&vertexBufferDesc, nullptr, &vertexBuffer));

    D3D11_MAPPED_SUBRESOURCE ms;
    ThrowIfFail(m_deviceContext->Map(vertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &ms));

    {
        std::vector<utility::Vector3> normals;
        normals.resize(vertices.size());

        for (size_t i = 0; i < indices.size() / 3; ++i)
        {
            auto const& v0 = vertices[indices[i * 3 + 0]];
            auto const& v1 = vertices[indices[i * 3 + 1]];
            auto const& v2 = vertices[indices[i * 3 + 2]];

            auto n = utility::Vector3::CrossProduct(v1 - v0, v2 - v0);
            n = utility::Vector3::Normalize(n);

            normals[indices[i * 3 + 0]] += n;
            normals[indices[i * 3 + 1]] += n;
            normals[indices[i * 3 + 2]] += n;
        }

        std::vector<ColoredVertex> coloredVertices;
        coloredVertices.reserve(vertices.size());

        for (size_t i = 0; i < vertices.size(); ++i)
        {
            ColoredVertex vertex;
            vertex.vertex = vertices[i];

            std::memcpy(vertex.color, color, sizeof(vertex.color));

            auto n = utility::Vector3::Normalize(normals[i]);
            vertex.nx = n.X;
            vertex.ny = n.Y;
            vertex.nz = n.Z;

            coloredVertices.emplace_back(vertex);
        }

        memcpy(ms.pData, &coloredVertices[0], sizeof(ColoredVertex)*coloredVertices.size());
    }

    m_deviceContext->Unmap(vertexBuffer, 0);

    ZERO(indexBufferDesc);

    indexBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    indexBufferDesc.ByteWidth = sizeof(int) * indices.size();
    indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
    indexBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    ID3D11Buffer *indexBuffer;

    ThrowIfFail(m_device->CreateBuffer(&indexBufferDesc, nullptr, &indexBuffer));

    ThrowIfFail(m_deviceContext->Map(indexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &ms));
    memcpy(ms.pData, &indices[0], sizeof(int)*indices.size());
    m_deviceContext->Unmap(indexBuffer, 0);

    buffer.push_back({ vertexBuffer, indexBuffer, indices.size() });
}

void Renderer::Render() const
{
    // clear the back buffer to a deep blue
    m_deviceContext->ClearRenderTargetView(m_backBuffer, BackgroundColor);
    m_deviceContext->ClearDepthStencilView(m_depthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.f, 0);

    D3D11_VIEWPORT viewport;
    UINT viewportCount = 1;

    m_deviceContext->RSGetViewports(&viewportCount, &viewport);

    float aspect = viewport.Width / viewport.Height;
    auto proj = utility::Matrix::CreateProjectionMatrix(PI / 4.f, aspect, 1.f, 10000.f);

    struct VSConstants {
        float viewMatrix[16];
        float projMatrix[16];
    } constants;

    proj.PopulateArray(constants.projMatrix);
    m_camera.GetViewMatrix().PopulateArray(constants.viewMatrix);

    m_deviceContext->UpdateSubresource(m_cbPerObjectBuffer, 0, nullptr, &constants, 0, 0);
    m_deviceContext->VSSetConstantBuffers(0, 1, &m_cbPerObjectBuffer.p);

    const unsigned int stride = sizeof(ColoredVertex);
    const unsigned int offset = 0;

    // draw terrain
    if (m_renderADT)
        for (size_t i = 0; i < m_terrainBuffers.size(); ++i)
        {
            ID3D11Buffer *const pBuffer[] = { m_terrainBuffers[i].VertexBuffer };
            m_deviceContext->IASetVertexBuffers(0, 1, pBuffer, &stride, &offset);
            m_deviceContext->IASetIndexBuffer(m_terrainBuffers[i].IndexBuffer, DXGI_FORMAT_R32_UINT, 0);
            m_deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

            m_deviceContext->DrawIndexed(m_terrainBuffers[i].IndexCount, 0, 0);
        }

    // draw wmos
    if (m_renderWMO)
        for (size_t i = 0; i < m_wmoBuffers.size(); ++i)
        {
            ID3D11Buffer *const pBuffer[] = { m_wmoBuffers[i].VertexBuffer };
            m_deviceContext->IASetVertexBuffers(0, 1, pBuffer, &stride, &offset);
            m_deviceContext->IASetIndexBuffer(m_wmoBuffers[i].IndexBuffer, DXGI_FORMAT_R32_UINT, 0);
            m_deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

            m_deviceContext->DrawIndexed(m_wmoBuffers[i].IndexCount, 0, 0);
        }

    // draw doodads
    if (m_renderDoodad)
        for (size_t i = 0; i < m_doodadBuffers.size(); ++i)
        {
            ID3D11Buffer *const pBuffer[] = { m_doodadBuffers[i].VertexBuffer };
            m_deviceContext->IASetVertexBuffers(0, 1, pBuffer, &stride, &offset);
            m_deviceContext->IASetIndexBuffer(m_doodadBuffers[i].IndexBuffer, DXGI_FORMAT_R32_UINT, 0);
            m_deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

            m_deviceContext->DrawIndexed(m_doodadBuffers[i].IndexCount, 0, 0);
        }

    if (m_renderLiquid || m_renderMesh)
    {
        const static float blendFactor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
        m_deviceContext->OMSetBlendState(m_liquidBlendState, blendFactor, 0xffffffff);

        // draw liquid (with alpha blending)
        if (m_renderLiquid)
            for (size_t i = 0; i < m_liquidBuffers.size(); ++i)
            {
                ID3D11Buffer *const pBuffer[] = { m_liquidBuffers[i].VertexBuffer };
                m_deviceContext->IASetVertexBuffers(0, 1, pBuffer, &stride, &offset);
                m_deviceContext->IASetIndexBuffer(m_liquidBuffers[i].IndexBuffer, DXGI_FORMAT_R32_UINT, 0);
                m_deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

                m_deviceContext->DrawIndexed(m_liquidBuffers[i].IndexCount, 0, 0);
            }

        // draw meshes (also with alpha blending)
        if (m_renderMesh)
            for (size_t i = 0; i < m_meshBuffers.size(); ++i)
            {
                ID3D11Buffer *const pBuffer[] = { m_meshBuffers[i].VertexBuffer };
                m_deviceContext->IASetVertexBuffers(0, 1, pBuffer, &stride, &offset);
                m_deviceContext->IASetIndexBuffer(m_meshBuffers[i].IndexBuffer, DXGI_FORMAT_R32_UINT, 0);
                m_deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

                m_deviceContext->DrawIndexed(m_meshBuffers[i].IndexCount, 0, 0);
            }

        m_deviceContext->OMSetBlendState(m_opaqueBlendState, blendFactor, 0xffffffff);
    }

    // switch the back buffer and the front buffer
    DXGI_PRESENT_PARAMETERS presentParams;
    ZERO(presentParams);

    m_swapChain->Present1(0, 0, &presentParams);
}

void Renderer::SetWireframe(bool enabled)
{
    D3D11_RASTERIZER_DESC rasterizerDesc;
    ZERO(rasterizerDesc);

    rasterizerDesc.FrontCounterClockwise = true;

    if (enabled)
    {
        rasterizerDesc.FillMode = D3D11_FILL_WIREFRAME;
        rasterizerDesc.CullMode = D3D11_CULL_NONE;
    }
    else
    {
        rasterizerDesc.FillMode = D3D11_FILL_SOLID;
        rasterizerDesc.CullMode = D3D11_CULL_BACK;
    }

    m_rasterizerState.Release();
    ThrowIfFail(m_device->CreateRasterizerState(&rasterizerDesc, &m_rasterizerState));
    m_deviceContext->RSSetState(m_rasterizerState);
}
