#include "LoaderUI.hpp"
#include "fonts.hpp"
#include "Loader.hpp"
#include "Locales.hpp"

#include "spdlog/spdlog.h"
#include "utils.hpp"

LoaderUI::LoaderUI(ModLoadOrder* mlo) {
    bool areHooksInit = false;
    do {
        if (kiero::init(kiero::RenderType::D3D11) == kiero::Status::Success) {
            kiero::bind(8, (void**)&oPresent, this->PresentHook);
            kiero::bind(13, (void**)&LoaderUI::oResizeBuffers, ResizeBuffersHook);
            
            areHooksInit = true;
            spdlog::info("Hooks initialized");
        }
    } while (!areHooksInit);

    if (!this->mlo) this->mlo = mlo;
    if (!this->loc) this->loc = new Localization();
}

LoaderUI::~LoaderUI() {
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    
    if (this->mainRenderTargetView) { 
        this->mainRenderTargetView->Release();
    }

    delete this->mlo;
    delete this->loc;
    delete this->fonts;
    delete this->mainRenderTargetView;
    delete this->pContext;
    delete this->pDevice;
    
    kiero::shutdown();
}

LRESULT __stdcall LoaderUI::WndProc(const HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    if (uMsg == WM_KEYDOWN && (wParam == VK_INSERT || (LoaderUI::renderMenu && wParam == VK_ESCAPE))) {
        LoaderUI::renderMenu = !LoaderUI::renderMenu;
        return 0;
    }

    if (LoaderUI::renderMenu) {
        ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam);
        
        ImGuiIO& io = ImGui::GetIO();
        
        if (io.WantCaptureMouse) {
            switch (uMsg) {
                case WM_LBUTTONDOWN:
                case WM_LBUTTONUP:
                case WM_RBUTTONDOWN:
                case WM_RBUTTONUP:
                case WM_MBUTTONDOWN:
                case WM_MBUTTONUP:
                case WM_MOUSEWHEEL:
                    return 1;
            }
        }
    }

    return CallWindowProc(oWndProc, hWnd, uMsg, wParam, lParam);
}

HRESULT __stdcall LoaderUI::ResizeBuffersHook(IDXGISwapChain* pSwapChain, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags) {
    if (LoaderUI::mainRenderTargetView) {
        LoaderUI::pContext->OMSetRenderTargets(0, 0, 0);
        LoaderUI::mainRenderTargetView->Release();
        LoaderUI::mainRenderTargetView = NULL;
    }

    return LoaderUI::oResizeBuffers(pSwapChain, BufferCount, Width, Height, NewFormat, SwapChainFlags);
}

HRESULT __stdcall LoaderUI::PresentHook(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags) {
    if (!LoaderUI::isUIInit) {
        if (SUCCEEDED(pSwapChain->GetDevice(__uuidof(ID3D11Device), (void**)&LoaderUI::pDevice))) {
            LoaderUI::pDevice->GetImmediateContext(&LoaderUI::pContext);
            DXGI_SWAP_CHAIN_DESC sd;
            pSwapChain->GetDesc(&sd);
            LoaderUI::window = sd.OutputWindow;
            LoaderUI::oWndProc = (WNDPROC)SetWindowLongPtr(LoaderUI::window, GWLP_WNDPROC, (LONG_PTR)LoaderUI::WndProc);
            
            ImGui::CreateContext();
            ImGuiIO& io = ImGui::GetIO();
            io.ConfigFlags = ImGuiConfigFlags_NoMouseCursorChange;
            ImGui_ImplWin32_Init(window);
            io.Fonts->Clear();

            LoaderUI::fonts->fontMain = io.Fonts->AddFontFromMemoryTTF(
                (void*)Inter_Medium,
                sizeof(Inter_Medium),
                18.0,
                nullptr,
                io.Fonts->GetGlyphRangesCyrillic()
            );

            ImFontConfig config;
            config.MergeMode = true;
            config.PixelSnapH = true;
            config.FontDataOwnedByAtlas = false; 

            static const ImWchar icons_ranges[] = { 0xf000, 0xf8ff, 0 };

            io.Fonts->AddFontFromMemoryTTF(
                (void*)Font_Awesome,
                sizeof(Font_Awesome),
                14.0,
                &config,
                icons_ranges
            );

            LoaderUI::fonts->fontTitle = io.Fonts->AddFontFromMemoryTTF(
                (void*)Inter_Bold,
                sizeof(Inter_Bold),
                20.0,
                nullptr,
                io.Fonts->GetGlyphRangesCyrillic()
            );

            LoaderUI::fonts->fontSmall = io.Fonts->AddFontFromMemoryTTF(
                (void*)Inter_Regular,
                sizeof(Inter_Regular),
                14.0,
                nullptr,
                io.Fonts->GetGlyphRangesCyrillic()
            );

            if (LoaderUI::fonts->fontMain) {
                io.FontDefault = LoaderUI::fonts->fontMain;
            } else {
                io.Fonts->AddFontDefault();
            }

            io.Fonts->Build();
            ImGui_ImplDX11_Init(pDevice, pContext);
            ImGui_ImplDX11_InvalidateDeviceObjects();
            ImGui_ImplDX11_CreateDeviceObjects();

            LoaderUI::InitStyles();
            LoaderUI::isUIInit = true;
        }
        else {
            return LoaderUI::oPresent(pSwapChain, SyncInterval, Flags);
        }
    }

    if (!LoaderUI::mainRenderTargetView) {
        ID3D11Texture2D* pBackBuffer;
        pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
        LoaderUI::pDevice->CreateRenderTargetView(pBackBuffer, NULL, &LoaderUI::mainRenderTargetView);
        pBackBuffer->Release();
    }

    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    if (LoaderUI::renderMenu) LoaderUI::RenderMenu();
    ImGui::Render();
    pContext->OMSetRenderTargets(1, &mainRenderTargetView, NULL);
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

    return LoaderUI::oPresent(pSwapChain, SyncInterval, Flags);
}

void LoaderUI::RenderMenu() {
    auto DrawTitle = []() {
        float y = ImGui::GetCursorPosY();
        if (ImGui::Button(reinterpret_cast<const char*>(u8"\uf01e"))) mlo->Refresh();
        Utils::UI::Hint(LOC("RELOAD_LIST"));
        ImGui::SameLine();
        if (ImGui::Button(reinterpret_cast<const char*>(u8"\uf07b"))) Utils::openModFolder();
        Utils::UI::Hint(LOC("OPEN_FOLDER"));
        
        ImGui::PushItemWidth(ImGui::CalcTextSize(LOC("SELECTED_LANG")).x + 40);
        ImGui::SameLine();
        static int selectedLang = loc->GetLang();
        if (ImGui::Combo("##lang", &selectedLang, Locales::languages, IM_ARRAYSIZE(Locales::languages))) loc->SetLang(selectedLang);
        ImGui::PopItemWidth();

        ImGui::SameLine(ImGui::GetWindowWidth() * 0.5 - ImGui::CalcTextSize("MetaLoader").x * 0.5, 0);
        ImGui::PushFont(LoaderUI::fonts->fontTitle);
        ImGui::SetCursorPosY(y - 2);
        ImGui::TextColored(ImVec4(0.56, 0.76, 0.84, 1.00), "Meta");
        ImGui::SameLine(0, 0);
        ImGui::Text("Loader");
        ImGui::PopFont();
        ImGui::SameLine(ImGui::GetWindowWidth() - ImGui::GetStyle().WindowPadding.x - 26.0, 0);
        ImGui::SetCursorPosY(y);
        if (ImGui::Button(reinterpret_cast<const char*>(u8"\uf00d"), ImVec2(26, 0))) LoaderUI::renderMenu = 0;
        ImGui::Separator();
    };

    auto DrawFooter = []() {
        ImGui::Separator();
        ImGui::PushFont(LoaderUI::fonts->fontSmall);
        ImGui::TextDisabled("MetaLoader %s", VERSION);
        ImGui::PopFont();
    };

    ImGuiIO& io = ImGui::GetIO();
    ImVec2 center(io.DisplaySize.x * 0.5, io.DisplaySize.y * 0.5);
    ImGui::SetNextWindowPos(center, ImGuiCond_FirstUseEver, ImVec2(0.5, 0.5));
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoTitleBar;
    ImGui::Begin("MetaLoader", &LoaderUI::renderMenu, flags);
    ImGui::SetWindowSize(ImVec2(600, 0));
    auto& mods = mlo->GetModsForUI();

    DrawTitle();
    ImGui::Text(LOC("MODS_LIST_TITLE"));
    ImGui::SameLine(0, 0);
    Utils::UI::HelpMarker(LOC("MODS_LIST_TITLE_TOOLTIP"));
    
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(ImGui::GetStyle().ItemSpacing.x, 0));
    if (ImGui::BeginChild("ModsList", ImVec2(-1, 300))) {
        ImGui::Separator();
        static int draggingFrom = -1;
        static int draggingAt = -1;
        
        float numColWidth = ImGui::CalcTextSize(std::string(std::to_string(mods.size()) + ".").c_str()).x;
        float startX = 5.0;
        for (int i = 0; i < mods.size(); i++) {
            auto& mod = mods[i];
            
            ImGui::PushID(mod.name.c_str());
            float startY = ImGui::GetCursorPosY();
            std::string numStr;
            if (draggingAt == i) {
                numStr = "+";
            } else if (draggingFrom == i) {
                numStr = "-";
            } else {
                numStr = std::to_string(i + 1) + ".";
            }

            float currentTextWidth = ImGui::CalcTextSize(numStr.c_str()).x;
            ImGui::SetCursorPosX(startX + numColWidth - currentTextWidth);
            
            float textOffset = (22 - ImGui::GetTextLineHeight()) / 2;
            ImGui::SetCursorPosY(startY + textOffset);

            ImGui::TextDisabled(numStr.c_str());
            
            ImGui::SameLine();
            float toggleX = startX + numColWidth + ImGui::GetStyle().ItemSpacing.x;
            ImGui::SetCursorPosX(toggleX);
            
            ImGui::SetCursorPosY(startY + 3.5);
            
            if (Utils::UI::ToggleButton(mod.name.c_str(), mod.enabled)) mlo->ApplyChanges();
            ImGui::SameLine();
            
            ImGui::SetCursorPosY(startY);
            if (!mod.enabled) ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyle().Colors[ImGuiCol_TextDisabled]);
            ImGui::Selectable(mod.name.c_str(), false, 0, ImVec2(0, 22));
            if (!mod.enabled) ImGui::PopStyleColor();

            if (ImGui::IsItemActive() && !ImGui::IsItemHovered()) {
                float dragDeltaY = fabsf(ImGui::GetMouseDragDelta(0).y);

                if (dragDeltaY > 10.0) {
                    if (draggingFrom == -1) {
                        draggingFrom = i;
                    }

                    int direction = (ImGui::GetMouseDragDelta(0).y < 0.0) ? -1 : 1;
                    int nextMod = i + direction;

                    if (nextMod >= 0 && nextMod < mods.size()) {
                        std::swap(mods[i], mods[nextMod]);
                        draggingAt = nextMod;
                        ImGui::ResetMouseDragDelta();
                    }
                }
            }

            if (ImGui::IsItemDeactivated() && draggingFrom != -1) {
                mlo->ApplyChanges();
                draggingFrom = -1;
                draggingAt = -1;
            }
            ImVec2 pMax = ImGui::GetItemRectMax();
            ImVec2 pMin = ImGui::GetItemRectMin();
            ImGui::GetWindowDrawList()->AddLine(
                ImVec2(pMin.x, pMax.y - 1.0),
                ImVec2(pMax.x, pMax.y - 1.0),
                ImGui::GetColorU32(ImVec4(1.0, 1.0, 1.0, 0.08))
            );
            ImGui::PopID();
        }
    }
    ImGui::EndChild();
    ImGui::PopStyleVar();
    DrawFooter();
    ImGui::End();
}

void LoaderUI::InitStyles() {
    ImGuiStyle& style = ImGui::GetStyle();
    
    style.WindowPadding     = ImVec2(8, 6);
    style.FramePadding      = ImVec2(8, 4);
    style.ItemSpacing       = ImVec2(8, 6);
    style.ItemInnerSpacing  = ImVec2(6, 6);
    style.IndentSpacing     = 20.0f;
    
    style.ScrollbarSize     = 8;
    style.GrabMinSize       = 10;

    style.WindowBorderSize  = 0;
    style.ChildBorderSize   = 0;
    style.PopupBorderSize   = 1;
    style.FrameBorderSize   = 0; 

    style.WindowRounding    = 8.0f;
    style.ChildRounding     = 6.0f;
    style.FrameRounding     = 6.0f; 
    style.PopupRounding     = 6.0f;
    style.ScrollbarRounding = 8.0f; 
    style.GrabRounding      = 6.0f;
    style.TabRounding       = 6.0f;

    style.WindowTitleAlign      = ImVec2(0.5f, 0.5f);
    style.ButtonTextAlign       = ImVec2(0.5f, 0.5f);
    style.SelectableTextAlign   = ImVec2(0.0f, 0.5f);

    ImVec4* colors = ImGui::GetStyle().Colors;

    ImVec4 accentColor = ImVec4(0.56, 0.76, 0.84, 1.00);

    colors[ImGuiCol_Text]                   = ImVec4(0.95f, 0.96f, 0.98f, 1.00f);
    colors[ImGuiCol_TextDisabled]           = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);

    colors[ImGuiCol_WindowBg]               = ImVec4(0.10f, 0.10f, 0.12f, 0.98f);
    colors[ImGuiCol_ChildBg]                = ImVec4(0.10f, 0.10f, 0.12f, 0.00f);
    colors[ImGuiCol_PopupBg]                = ImVec4(0.12f, 0.12f, 0.14f, 0.98f);

    colors[ImGuiCol_Border]                 = ImVec4(0.20f, 0.20f, 0.22f, 0.50f);
    colors[ImGuiCol_BorderShadow]           = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);

    colors[ImGuiCol_FrameBg]                = ImVec4(0.18f, 0.18f, 0.20f, 1.00f);
    colors[ImGuiCol_FrameBgHovered]         = ImVec4(0.22f, 0.22f, 0.24f, 1.00f);
    colors[ImGuiCol_FrameBgActive]          = ImVec4(0.25f, 0.25f, 0.27f, 1.00f);

    colors[ImGuiCol_TitleBg]                = ImVec4(0.10f, 0.10f, 0.12f, 1.00f);
    colors[ImGuiCol_TitleBgActive]          = ImVec4(0.10f, 0.10f, 0.12f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed]       = ImVec4(0.10f, 0.10f, 0.12f, 1.00f);

    colors[ImGuiCol_ScrollbarBg]            = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_ScrollbarGrab]          = ImVec4(0.30f, 0.30f, 0.32f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered]   = ImVec4(0.40f, 0.40f, 0.42f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive]    = accentColor;

    colors[ImGuiCol_CheckMark]              = accentColor;
    colors[ImGuiCol_SliderGrab]             = accentColor;
    colors[ImGuiCol_SliderGrabActive]       = ImVec4(accentColor.x, accentColor.y, accentColor.z, 0.80f);

    colors[ImGuiCol_Button]                 = ImVec4(0.18f, 0.18f, 0.20f, 1.00f);
    colors[ImGuiCol_ButtonHovered]          = ImVec4(0.24f, 0.24f, 0.26f, 1.00f);
    colors[ImGuiCol_ButtonActive]           = ImVec4(0.20f, 0.20f, 0.22f, 1.00f);

    colors[ImGuiCol_Header]                 = ImVec4(0.18f, 0.18f, 0.20f, 0.00f); 
    colors[ImGuiCol_HeaderHovered]          = ImVec4(0.18f, 0.18f, 0.20f, 1.00f);
    colors[ImGuiCol_HeaderActive]           = ImVec4(0.22f, 0.22f, 0.24f, 1.00f);
    
    colors[ImGuiCol_Separator]              = ImVec4(0.40f, 0.40f, 0.42f, 0.20f);
    colors[ImGuiCol_SeparatorHovered]       = accentColor;
    colors[ImGuiCol_SeparatorActive]        = accentColor;
    
    colors[ImGuiCol_ResizeGrip]             = ImVec4(accentColor.x, accentColor.y, accentColor.z, 0.25f);
    colors[ImGuiCol_ResizeGripHovered]      = accentColor;
    colors[ImGuiCol_ResizeGripActive]       = accentColor;
}