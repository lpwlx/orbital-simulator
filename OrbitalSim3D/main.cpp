#include <windows.h>
#include <d3d11.h>
#include <dxgi.h>
#include <d3dcompiler.h>

// Глобальные переменные для DirectX
IDXGISwapChain* g_SwapChain = nullptr;
ID3D11Device* g_Device = nullptr;
ID3D11DeviceContext* g_DeviceContext = nullptr;
ID3D11RenderTargetView* g_RenderTargetView = nullptr;

LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message)
    {
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hWnd, message, wParam, lParam);
}

bool InitDirect3D11(HWND hWnd) {
    DXGI_SWAP_CHAIN_DESC scd = {};
    scd.BufferCount = 1;
    scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    scd.OutputWindow = hWnd;
    scd.SampleDesc.Count = 1; // Без мультисэмплинга
    scd.Windowed = TRUE;

    HRESULT hr = D3D11CreateDeviceAndSwapChain(
        nullptr,                    // Адаптер по умолчанию
        D3D_DRIVER_TYPE_HARDWARE,   // Используем аппаратный рендеринг
        nullptr,
        0,                          // Флаги создания устройства
        nullptr, 0,                 // Используем стандартный набор фич
        D3D11_SDK_VERSION,
        &scd,
        &g_SwapChain,
        &g_Device,
        nullptr,
        &g_DeviceContext
    );

    if (FAILED(hr)) {
        return false;
    }

    // Получаем бэк-буфер и создаём Render Target View
    ID3D11Texture2D* pBackBuffer = nullptr;
    hr = g_SwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
    if (FAILED(hr)) {
        return false;
    }

    hr = g_Device->CreateRenderTargetView(pBackBuffer, nullptr, &g_RenderTargetView);
    pBackBuffer->Release();
    if (FAILED(hr)) {
        return false;
    }

    // Устанавливаем Render Target
    g_DeviceContext->OMSetRenderTargets(1, &g_RenderTargetView, nullptr);

    // Устанавливаем вьюпорт
    D3D11_VIEWPORT viewport = {};
    viewport.TopLeftX = 0;
    viewport.TopLeftY = 0;
    viewport.Width = 800;   // Ширина окна
    viewport.Height = 600;  // Высота окна
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    g_DeviceContext->RSSetViewports(1, &viewport);

    return true;
}

void CleanD3D() {
    if (g_RenderTargetView) g_RenderTargetView->Release();
    if (g_SwapChain) g_SwapChain->Release();
    if (g_DeviceContext) g_DeviceContext->Release();
    if (g_Device) g_Device->Release();
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow) {
    // Регистрация класса окна
    const wchar_t CLASS_NAME[] = L"DirectX11WindowClass";
    WNDCLASS wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    RegisterClass(&wc);

    // Создание окна
    HWND hWnd = CreateWindowEx(
        0,
        CLASS_NAME,
        L"DirectX 11 Application",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 800, 600,
        nullptr,
        nullptr,
        hInstance,
        nullptr
    );

    if (hWnd == nullptr) {
        return 0;
    }

    ShowWindow(hWnd, nCmdShow);

    // Инициализация DirectX 11
    if (!InitDirect3D11(hWnd)) {
        MessageBox(hWnd, L"DirectX 11 Initialization Failed", L"Error", MB_OK);
        return 0;
    }

    // Основной цикл сообщений
    MSG msg = {};
    while (msg.message != WM_QUIT) {
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else {
            // Очистка экрана (например, синим цветом)
            float clearColor[4] = { 0.0f, 0.2f, 0.4f, 1.0f };
            g_DeviceContext->ClearRenderTargetView(g_RenderTargetView, clearColor);

            // Выводим кадр
            g_SwapChain->Present(1, 0);
        }
    }

    CleanD3D();

    return 0;
}
