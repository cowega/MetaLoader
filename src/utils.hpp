#pragma once

#include <filesystem>
#include <string>

namespace fs = std::filesystem;

namespace Utils {
    std::string GetCurrentProcessName();

    fs::path CutRawGamePath(const fs::path& fullPath);

    std::vector<fs::path> GetDirectories(const fs::path& path);
}