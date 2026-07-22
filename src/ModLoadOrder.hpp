#pragma once

#include <unordered_map>
#include <filesystem>
#include <nlohmann/json.hpp>

namespace fs = std::filesystem;
using json = nlohmann::json;

struct ModConfig {
    std::string name;
    bool enabled;
};

struct StringHash {
    using is_transparent = void;

    size_t operator()(std::string_view sv) const {
        return std::hash<std::string_view>{}(sv);
    }
    size_t operator()(const std::string& s) const {
        return std::hash<std::string>{}(s);
    }
};

class ModLoadOrder {
public:
    std::vector<ModConfig> mods;
    std::unordered_map<std::string, std::string, StringHash, std::equal_to<>> vfs;

    ModLoadOrder();
    ~ModLoadOrder();

    std::vector<ModConfig>& GetModsForUI();
    const std::string_view GetFile(std::string_view virtualPath);
    void ProcessAllZipFiles();
    void ApplyChanges();
    void Refresh();

private:
    void SaveConfig();
    void LoadConfig();
    void RebuildVFS();

};