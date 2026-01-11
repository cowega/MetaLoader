#include <filesystem>
#include <Windows.h>

#include "utils.hpp"

namespace Utils {
    std::string GetCurrentProcessName() {
        char procName[MAX_PATH];

        GetModuleFileNameA(NULL, procName, MAX_PATH);
        std::string filename = fs::path(procName).filename().string();

        std::transform(filename.begin(), filename.end(), filename.begin(), ::tolower);
        return filename;
    }

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
}
