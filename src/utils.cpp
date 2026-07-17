#include <windows.h>
#include <unordered_map>
#include <shellapi.h>
#include <filesystem>
#include <string>
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <spdlog/spdlog.h>

#include "utils.hpp"
#include "imgui.h"
#include "LoaderUI.hpp"
#include "GameHook.hpp"

namespace Utils {
    std::string GetCurrentProcessName() {
        char procName[MAX_PATH];

        GetModuleFileNameA(NULL, procName, MAX_PATH);
        std::string filename = fs::path(procName).filename().string();

        std::transform(filename.begin(), filename.end(), filename.begin(), ::tolower);
        return filename;
    }

    std::vector<fs::path> GetDirectories(const fs::path& path) {
        std::vector<fs::path> folders;
        if (!fs::exists(path) || !fs::is_directory(path)) return folders;
        try {
            for (const auto& entry : fs::directory_iterator(path))
                if (entry.is_directory()) folders.push_back(entry.path());
        }
        catch (...) {}
        return folders;
    }

    void openModFolder() {
        std::filesystem::path path = std::filesystem::current_path() / "metaloader";
        std::filesystem::create_directories(path);

        ShellExecuteW(NULL, L"explore", path.c_str(), NULL, NULL, SW_SHOWNORMAL);
    }

    HWND GetMyWindow()  {
        DWORD myPID = GetCurrentProcessId();
        HWND hWnd = GetTopWindow(NULL); 

        while (hWnd) {
            DWORD pid = 0;
            GetWindowThreadProcessId(hWnd, &pid);

            if (pid == myPID && IsWindowVisible(hWnd)) return hWnd;
            hWnd = GetWindow(hWnd, GW_HWNDNEXT);
        }
        return NULL;
    }

    bool HasCyrillic(const std::wstring& text) {
        for (wchar_t wc : text) {
            if (wc >= 0x0400 && wc <= 0x04FF) {
                return true;
            }
        }
        return false;
    }

    namespace Hook {
        fs::path CutRawGamePath_old(const fs::path& fullPath) {
            fs::path result;
            bool found = false;
            for (const auto& part : fullPath) {
                if (!found && part == "packs") {
                    found = true;
                    result /= "Data";
                    continue;
                }

                if (!found && part == "Data") {
                    found = true;
                    result /= "Data";
                    continue;
                }

                if (found) result /= part;
            }
            return result;
        }

        size_t FindDir(std::string_view path, std::string_view dir) {
            size_t pos = 0;
            while ((pos = path.find(dir, pos)) != std::string_view::npos) {
                bool validStart = (pos == 0 || path[pos - 1] == '/' || path[pos - 1] == '\\');
                bool validEnd = (pos + dir.length() == path.length() || path[pos + dir.length()] == '/' || path[pos + dir.length()] == '\\');
                
                if (validStart && validEnd) {
                    return pos;
                }
                pos += dir.length();
            }
            return std::string_view::npos;
        }

        std::string_view CutRawGamePath(std::string_view fullPath, char* outBuffer, size_t bufferSize) {
            size_t posData = FindDir(fullPath, "Data");
            size_t posPacks = FindDir(fullPath, "packs");
            
            size_t firstPos = (posData < posPacks) ? posData : posPacks;

            if (firstPos == std::string_view::npos) {
                return {};
            }

            std::string_view rest = (firstPos == posData) ? fullPath.substr(posData + 4) : fullPath.substr(posPacks + 5);
            size_t requiredSize = 4 + rest.length();
            
            if (requiredSize >= bufferSize) {
                return {};
            }

            memcpy(outBuffer, "Data", 4);
            
            for (size_t i = 0; i < rest.length(); ++i) {
                char c = rest[i];
                outBuffer[4 + i] = (c == '\\') ? '/' : c;
            }

            return std::string_view(outBuffer, requiredSize);
        }
    }

    namespace UI {
        void HelpMarker(const char* desc) {
            ImGui::PushFont(LoaderUI::fonts->fontSmall);
            ImGui::TextDisabled("(?)");
            ImGui::PopFont();
            Hint(desc);
        }
    
        void Hint(const char* desc) {
            if (ImGui::IsItemHovered()) {
                ImGui::BeginTooltip();
                ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0);
                ImGui::TextUnformatted(desc);
                ImGui::PopTextWrapPos();
                ImGui::EndTooltip();
            }
        }

        bool ToggleButton(const char* unique_id, bool& state) {
            static std::unordered_map<std::string, double> LastTime;
            static std::unordered_map<std::string, bool> LastActive;

            ImVec2 p = ImGui::GetCursorScreenPos();
            ImDrawList* dl = ImGui::GetWindowDrawList();

            bool clicked = false;

            float h = 16.0;
            float w = h * 1.75;
            float r = h / 2.0;
            float s = 0.25;

            auto ImSaturate = [](float f) -> float {
                return (f < 0.0) ? 0.0 : ((f > 1.0) ? 1.0 : f);
            };

            float x_begin = state ? 1.0 : 0.0;
            float t_begin = state ? 0.0 : 1.0;

            if (ImGui::InvisibleButton(unique_id, ImVec2(w, h))) {
                state = !state;
                LastTime[std::string(unique_id)] = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::steady_clock::now().time_since_epoch()).count() / 1000.0;
                LastActive[std::string(unique_id)] = true;
                clicked = true;
            }

            if (LastActive[std::string(unique_id)]) {
                auto current_time = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::steady_clock::now().time_since_epoch()).count() / 1000.0;
                double time = current_time - LastTime[std::string(unique_id)];

                if (time <= s) {
                    float anim = ImSaturate(static_cast<float>(time / s));
                    x_begin = state ? anim : 1.0 - anim;
                    t_begin = state ? 1.0 - anim : anim;
                } else {
                    LastActive[std::string(unique_id)] = false;
                }
            }

            ImVec4 bg_color(0.5608, 0.7608, 0.8431, ImGui::IsItemHovered() ? 0.8 : 0.9);
            if (!state) {
                bg_color = ImVec4(0.65, 0.63, 0.60, ImGui::IsItemHovered() ? 0.8 : 0.9);
            }

            dl->AddRectFilled(
                ImVec2(p.x, p.y),
                ImVec2(p.x + w, p.y + h),
                ImGui::GetColorU32(bg_color),
                r
            );

            dl->AddCircleFilled(
                ImVec2(p.x + r + x_begin * (w - r * 2), p.y + r),
                (t_begin < 0.5) ? x_begin * r : t_begin * r,
                ImGui::GetColorU32(ImVec4(0.9, 0.9, 0.9, state ? 1.0 : 0.75)),
                static_cast<int>(r + 5)
            );

            return clicked;
        }
    }

    uintptr_t FindXRef(const std::string& stringTarget) {
        HMODULE hMod = GetModuleHandle(nullptr);
        if (!hMod) return 0;

        uintptr_t base = (uintptr_t)hMod;
        PIMAGE_DOS_HEADER dosHeader = (PIMAGE_DOS_HEADER)base;
        PIMAGE_NT_HEADERS ntHeaders = (PIMAGE_NT_HEADERS)(base + dosHeader->e_lfanew);
        PIMAGE_SECTION_HEADER sectionHeader = IMAGE_FIRST_SECTION(ntHeaders);

        uintptr_t stringAddress = 0;

        for (int i = 0; i < ntHeaders->FileHeader.NumberOfSections; i++) {
            auto& section = sectionHeader[i];
            char name[IMAGE_SIZEOF_SHORT_NAME + 1] = { 0 };
            memcpy(name, section.Name, IMAGE_SIZEOF_SHORT_NAME);

            if (strcmp(name, ".rdata") == 0 || strcmp(name, ".data") == 0) {
                uintptr_t start = base + section.VirtualAddress;
                uintptr_t end = start + section.SizeOfRawData;

                for (uintptr_t addr = start; addr < end - stringTarget.length(); addr++) {
                    if (memcmp((void*)addr, stringTarget.c_str(), stringTarget.length() + 1) == 0) {
                        stringAddress = addr;
                        break;
                    }
                }
            }
            if (stringAddress) break;
        }

        if (!stringAddress) return 0;

        for (int i = 0; i < ntHeaders->FileHeader.NumberOfSections; i++) {
            auto& section = sectionHeader[i];
            char name[IMAGE_SIZEOF_SHORT_NAME + 1] = { 0 };
            memcpy(name, section.Name, IMAGE_SIZEOF_SHORT_NAME);

            if (strcmp(name, ".text") == 0) {
                uintptr_t start = base + section.VirtualAddress;
                uintptr_t end = start + section.SizeOfRawData;

                for (uintptr_t addr = start; addr < end - 7; addr++) {
                    uint8_t* ptr = (uint8_t*)addr;
                    if ((ptr[0] == 0x48 || ptr[0] == 0x4C) && (ptr[1] == 0x8D || ptr[1] == 0x8B)) {
                        if ((ptr[2] & 0xC7) == 0x05) {
                            int32_t offset = *(int32_t*)(addr + 3);
                            uintptr_t target = addr + 7 + offset;
                            if (target == stringAddress) return addr;
                        }
                    }
                }
            }
        }

        return 0;
    }
}
