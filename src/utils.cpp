#include <filesystem>
#include <windows.h>
#include <unordered_map>
#include <shellapi.h>
#include <filesystem>
#include <string>
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

    namespace Hook {
        fs::path CutRawGamePath(const fs::path& fullPath) {
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
}
