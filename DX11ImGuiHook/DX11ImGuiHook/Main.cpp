#include <windows.h>
#include <d3d11.h>
#include "MinHook/MinHook.h"
#include "ImGui/imgui.h"
#include "ImGui/imgui_impl_win32.h"
#include "ImGui/imgui_impl_dx11.h"

#pragma comment(lib, "d3d11.lib")
#if _WIN64
#pragma comment(lib, "MinHook/libMinHook.x64.lib")
#else
#pragma comment(lib, "MinHook/libMinHook.x86.lib")
#endif

// Needed or Compiler Cries :(
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace Overlay
{
    HWND windowHandle = nullptr;
    WNDPROC originalWndProc = nullptr;

    ID3D11Device* device = nullptr;
    ID3D11DeviceContext* context = nullptr;
    ID3D11RenderTargetView* rtv = nullptr;

    typedef long(__stdcall* PresentFn)(IDXGISwapChain*, UINT, UINT);
    PresentFn originalPresent = nullptr;

    bool initialized = false;

    // Custom WndProc
    LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wParam, lParam))
            return 0; // handled

        return CallWindowProc(originalWndProc, hwnd, msg, wParam, lParam);
    }

    // Setup render target
    void SetupRenderTarget(IDXGISwapChain* swapChain)
    {
        ID3D11Texture2D* backBuffer = nullptr;
        swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&backBuffer));
        device->CreateRenderTargetView(backBuffer, nullptr, &rtv);
        backBuffer->Release();
    }

    // Our hooked Present function
    long __stdcall HookedPresent(IDXGISwapChain* swapChain, UINT interval, UINT flags)
    {
        if (!initialized)
        {
            if (SUCCEEDED(swapChain->GetDevice(__uuidof(ID3D11Device), reinterpret_cast<void**>(&device))))
            {
                device->GetImmediateContext(&context);

                DXGI_SWAP_CHAIN_DESC desc;
                swapChain->GetDesc(&desc);
                windowHandle = desc.OutputWindow;

                SetupRenderTarget(swapChain);

                originalWndProc = (WNDPROC)SetWindowLongPtr(windowHandle, GWLP_WNDPROC, (LONG_PTR)WindowProc);

                IMGUI_CHECKVERSION();
                ImGui::CreateContext();
                ImGuiIO& io = ImGui::GetIO();
                io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;

                ImGui_ImplWin32_Init(windowHandle);
                ImGui_ImplDX11_Init(device, context);

                initialized = true;
            }
        }

        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        // Custom overlay window
        ImGui::Begin("Custom Overlay");
        ImGui::Text("This is a unique overlay!");
        ImGui::End();

        ImGui::Render();
        context->OMSetRenderTargets(1, &rtv, nullptr);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        return originalPresent(swapChain, interval, flags);
    }

    // Retrieve Present pointer in a different way
    PresentFn GetPresentFunction()
    {
        D3D_FEATURE_LEVEL levels[] = { D3D_FEATURE_LEVEL_11_0 };
        DXGI_SWAP_CHAIN_DESC desc = {};
        desc.BufferCount = 1;
        desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        desc.OutputWindow = GetDesktopWindow();
        desc.SampleDesc.Count = 1;
        desc.Windowed = TRUE;
        desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

        ID3D11Device* tempDevice = nullptr;
        IDXGISwapChain* tempSwap = nullptr;

        if (D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0,
            levels, 1, D3D11_SDK_VERSION, &desc, &tempSwap, &tempDevice, nullptr, nullptr) == S_OK)
        {
            void** vtable = *reinterpret_cast<void***>(tempSwap);
            PresentFn addr = reinterpret_cast<PresentFn>(vtable[8]);
            tempSwap->Release();
            tempDevice->Release();
            return addr;
        }

        return nullptr;
    }

    // Main initialization thread
    DWORD WINAPI InitThread(LPVOID)
    {
        MH_Initialize();
        PresentFn presentAddr = GetPresentFunction();
        if (presentAddr)
        {
            MH_CreateHook(presentAddr, &HookedPresent, reinterpret_cast<void**>(&originalPresent));
            MH_EnableHook(presentAddr);
        }

        // Wait until VK_END is pressed
        while (!(GetAsyncKeyState(VK_END) & 0x8000))
        {
            Sleep(50);
        }

        // Cleanup
        MH_DisableHook(presentAddr);
        MH_Uninitialize();

        ImGui_ImplDX11_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();

        if (rtv) { rtv->Release(); rtv = nullptr; }
        if (context) { context->Release(); context = nullptr; }
        if (device) { device->Release(); device = nullptr; }
        SetWindowLongPtr(windowHandle, GWLP_WNDPROC, (LONG_PTR)originalWndProc);

        FreeLibraryAndExitThread((HINSTANCE)GetModuleHandle(nullptr), 0);
        return 0;
    }
}

// DLL entry
BOOL WINAPI DllMain(HINSTANCE hModule, DWORD reason, LPVOID)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        DisableThreadLibraryCalls(hModule);
        CreateThread(nullptr, 0, Overlay::InitThread, nullptr, 0, nullptr);
    }
    return TRUE;
}
