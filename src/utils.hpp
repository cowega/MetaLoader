#pragma once

#include <filesystem>
#include <string>

namespace fs = std::filesystem;

namespace Utils {
    std::string GetCurrentProcessName();

    fs::path CutRawGamePath(const fs::path& fullPath);

    std::vector<fs::path> GetDirectories(const fs::path& path);

    void openModFolder();

    namespace UI {
        void HelpMarker(const char* desc);

        void Hint(const char* desc);

        bool ToggleButton(const char* unique_id, bool& state);
    }
}