#pragma once

#include <filesystem>
#include <string>

namespace fs = std::filesystem;

namespace Utils {
    std::string GetCurrentProcessName();
    std::vector<fs::path> GetDirectories(const fs::path& path);
    void openModFolder();
    HWND GetMyWindow();
    uintptr_t FindXRef(const std::string& stringTarget);
    bool HasCyrillic(const std::wstring& text);

    namespace Hook {
        fs::path CutRawGamePath_old(const fs::path& fullPath);
        size_t FindDir(std::string_view path, std::string_view dir);
        std::string_view CutRawGamePath(std::string_view fullPath, char* outBuffer, size_t bufferSize);
    }

    namespace UI {
        void HelpMarker(const char* desc);
        void Hint(const char* desc);
        bool ToggleButton(const char* unique_id, bool& state);
    }
}