#include <windows.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
using namespace DirectX;

// Линки
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "D3DCompiler.lib")

// Глобальные переменные
IDXGISwapChain* g_SwapChain = nullptr;
ID3D11Device* g_Device = nullptr;
ID3D11DeviceContext* g_DeviceContext = nullptr;
ID3D11RenderTargetView* g_RenderTargetView = nullptr;

ID3D11VertexShader* g_VertexShader = nullptr;
ID3D11PixelShader* g_PixelShader = nullptr;
ID3D11InputLayout* g_InputLayout = nullptr;
ID3D11Buffer* g_VertexBuffer = nullptr;
ID3D11Buffer* g_IndexBuffer = nullptr;
ID3D11Buffer* g_MatrixBuffer = nullptr;

XMMATRIX g_WorldMatrix, g_ViewMatrix, g_ProjectionMatrix;

struct Vertex {
    XMFLOAT3 position;
    XMFLOAT4 color;
};

struct MatrixBufferType {
    XMMATRIX world;
    XMMATRIX view;
    XMMATRIX projection;
};

LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    if (message == WM_DESTROY) {
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hWnd, message, wParam, lParam);
}

bool LoadShaders() {
    ID3DBlob* vsBlob = nullptr;
    ID3DBlob* psBlob = nullptr;
    ID3DBlob* errorBlob = nullptr;

    HRESULT hr = D3DCompileFromFile(L"C:\\Programming\\current\\OrbitalSim3D\\OrbitalSim3D\\shader.hlsl", nullptr, nullptr, "VSMain", "vs_5_0", 0, 0, &vsBlob, &errorBlob);
    if (FAILED(hr)) {
        if (errorBlob) {
            OutputDebugStringA((char*)errorBlob->GetBufferPointer());
            errorBlob->Release();
        }
        return false;
    }
    
    hr = D3DCompileFromFile(L"C:\\Programming\\current\\OrbitalSim3D\\OrbitalSim3D\\shader.hlsl", nullptr, nullptr, "PSMain", "ps_5_0", 0, 0, &psBlob, &errorBlob);
    if (FAILED(hr)) {
        if (errorBlob) {
            OutputDebugStringA((char*)errorBlob->GetBufferPointer());
            errorBlob->Release();
        }
        vsBlob->Release();
        return false;
    }
    
    if (FAILED(hr)) {
        if (errorBlob) {
            OutputDebugStringA((char*)errorBlob->GetBufferPointer());
            errorBlob->Release();
        }
        else {
            MessageBoxA(nullptr, "Shader file not found or could not be compiled", "Error", MB_OK);
        }
        return false;
    }


    g_Device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &g_VertexShader);
    g_Device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &g_PixelShader);

    D3D11_INPUT_ELEMENT_DESC layout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,   D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 }
    };

    g_Device->CreateInputLayout(layout, 2, vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &g_InputLayout);

    vsBlob->Release();
    psBlob->Release();
    return true;
}

bool InitDirect3D11(HWND hWnd) {
    DXGI_SWAP_CHAIN_DESC scd = {};
    scd.BufferCount = 1;
    scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    scd.OutputWindow = hWnd;
    scd.SampleDesc.Count = 1;
    scd.Windowed = TRUE;

    HRESULT hr = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0,
        nullptr, 0, D3D11_SDK_VERSION, &scd, &g_SwapChain, &g_Device, nullptr, &g_DeviceContext);
    if (FAILED(hr)) return false;

    ID3D11Texture2D* backBuffer = nullptr;
    g_SwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&backBuffer);
    g_Device->CreateRenderTargetView(backBuffer, nullptr, &g_RenderTargetView);
    backBuffer->Release();
    g_DeviceContext->OMSetRenderTargets(1, &g_RenderTargetView, nullptr);

    D3D11_VIEWPORT vp = { 0, 0, 800, 600, 0.0f, 1.0f };
    g_DeviceContext->RSSetViewports(1, &vp);

    // Шейдеры
    if (!LoadShaders()) return false;

    // Вершины куба
    Vertex vertices[] = {
        {{-1,-1,-1}, {1,0,0,1}}, {{-1,+1,-1}, {0,1,0,1}}, {{+1,+1,-1}, {0,0,1,1}}, {{+1,-1,-1}, {1,1,0,1}},
        {{-1,-1,+1}, {1,0,1,1}}, {{-1,+1,+1}, {0,1,1,1}}, {{+1,+1,+1}, {1,1,1,1}}, {{+1,-1,+1}, {0,0,0,1}},
    };

    WORD indices[] = {
        0,1,2, 0,2,3,  4,6,5, 4,7,6,
        4,5,1, 4,1,0,  3,2,6, 3,6,7,
        1,5,6, 1,6,2,  4,0,3, 4,3,7
    };

    // Vertex buffer
    D3D11_BUFFER_DESC vbDesc = { sizeof(vertices), D3D11_USAGE_DEFAULT, D3D11_BIND_VERTEX_BUFFER };
    D3D11_SUBRESOURCE_DATA vbData = { vertices };
    g_Device->CreateBuffer(&vbDesc, &vbData, &g_VertexBuffer);

    // Index buffer
    D3D11_BUFFER_DESC ibDesc = { sizeof(indices), D3D11_USAGE_DEFAULT, D3D11_BIND_INDEX_BUFFER };
    D3D11_SUBRESOURCE_DATA ibData = { indices };
    g_Device->CreateBuffer(&ibDesc, &ibData, &g_IndexBuffer);

    // Constant buffer
    D3D11_BUFFER_DESC cbDesc = { sizeof(MatrixBufferType), D3D11_USAGE_DEFAULT, D3D11_BIND_CONSTANT_BUFFER };
    g_Device->CreateBuffer(&cbDesc, nullptr, &g_MatrixBuffer);

    // Проекция
    g_ProjectionMatrix = XMMatrixPerspectiveFovLH(XM_PIDIV4, 800.0f / 600.0f, 0.1f, 100.0f);

    return true;
}

void CleanD3D() {
    if (g_VertexBuffer) g_VertexBuffer->Release();
    if (g_IndexBuffer) g_IndexBuffer->Release();
    if (g_MatrixBuffer) g_MatrixBuffer->Release();
    if (g_VertexShader) g_VertexShader->Release();
    if (g_PixelShader) g_PixelShader->Release();
    if (g_InputLayout) g_InputLayout->Release();
    if (g_RenderTargetView) g_RenderTargetView->Release();
    if (g_SwapChain) g_SwapChain->Release();
    if (g_DeviceContext) g_DeviceContext->Release();
    if (g_Device) g_Device->Release();
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow) {
    WNDCLASS wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"DXWindow";
    RegisterClass(&wc);

    HWND hWnd = CreateWindowEx(0, L"DXWindow", L"DirectX11 Cube", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 800, 600, nullptr, nullptr, hInstance, nullptr);
    if (!hWnd) return 0;

    ShowWindow(hWnd, nCmdShow);
    if (!InitDirect3D11(hWnd)) {
        MessageBox(hWnd, L"Failed to init D3D11", L"Error", MB_OK);
        return 0;
    }

    MSG msg = {};
    while (msg.message != WM_QUIT) {
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else {
            float t = GetTickCount64() / 1000.0f;
            float radius = 5.0f;

            XMVECTOR eye = XMVectorSet(sinf(t) * radius, 2.0f, cosf(t) * radius, 1.0f);
            XMVECTOR at = XMVectorZero();
            XMVECTOR up = XMVectorSet(0, 1, 0, 0);

            g_ViewMatrix = XMMatrixLookAtLH(eye, at, up);
            g_WorldMatrix = XMMatrixRotationY(t);

            MatrixBufferType mb;
            mb.world = XMMatrixTranspose(g_WorldMatrix);
            mb.view = XMMatrixTranspose(g_ViewMatrix);
            mb.projection = XMMatrixTranspose(g_ProjectionMatrix);
            g_DeviceContext->UpdateSubresource(g_MatrixBuffer, 0, nullptr, &mb, 0, 0);

            float clearColor[4] = { 0.1f, 0.2f, 0.3f, 1.0f };
            g_DeviceContext->ClearRenderTargetView(g_RenderTargetView, clearColor);

            UINT stride = sizeof(Vertex), offset = 0;
            g_DeviceContext->IASetVertexBuffers(0, 1, &g_VertexBuffer, &stride, &offset);
            g_DeviceContext->IASetIndexBuffer(g_IndexBuffer, DXGI_FORMAT_R16_UINT, 0);
            g_DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            g_DeviceContext->IASetInputLayout(g_InputLayout);

            g_DeviceContext->VSSetShader(g_VertexShader, nullptr, 0);
            g_DeviceContext->VSSetConstantBuffers(0, 1, &g_MatrixBuffer);
            g_DeviceContext->PSSetShader(g_PixelShader, nullptr, 0);

            g_DeviceContext->DrawIndexed(36, 0, 0);

            g_SwapChain->Present(1, 0);
        }
    }

    CleanD3D();
    return 0;
}
