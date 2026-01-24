#pragma once

#include <Windows.h>
#include <d3d11.h>
#include <dxgi.h>
#include "kiero.h"
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"

#include "ModLoadOrder.hpp"

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

typedef HRESULT(__stdcall* Present) (IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags);
typedef HRESULT(__stdcall* ResizeBuffers) (IDXGISwapChain* pSwapChain, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags);
typedef LRESULT(CALLBACK* WNDPROC)(HWND, UINT, WPARAM, LPARAM);

struct Fonts {
    ImFont* fontMain = nullptr;
    ImFont* fontTitle = nullptr;
    ImFont* fontSmall = nullptr;
    ImFont* fontAwesome = nullptr;
};

class LoaderUI {
public:
    LoaderUI(ModLoadOrder* mlo);
    ~LoaderUI();
    inline static Fonts* fonts = new Fonts();
    
private:
    inline static ModLoadOrder* mlo;

    inline static Present oPresent;
    inline static ResizeBuffers oResizeBuffers;
    inline static WNDPROC oWndProc;

    inline static ID3D11RenderTargetView* mainRenderTargetView = nullptr;
    inline static ID3D11DeviceContext* pContext = nullptr;
    inline static ID3D11Device* pDevice = nullptr;
    
    inline static HWND window = NULL;
    inline static bool isUIInit = false;
    inline static bool renderMenu = false;
    
    static HRESULT __stdcall PresentHook(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags);
    static HRESULT __stdcall ResizeBuffersHook(IDXGISwapChain* pSwapChain, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags);
    static LRESULT __stdcall WndProc(const HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    static void RenderMenu();
    static void RenderNotification();
    static void InitStyles();
};