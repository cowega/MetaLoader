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

class ModLoadOrder {
public:
    std::vector<ModConfig> mods;
    std::unordered_map<std::string, fs::path> vfs;

    ModLoadOrder();
    ~ModLoadOrder();

    std::vector<ModConfig>& GetModsForUI();
    std::string GetFile(const std::string& virtualPath);
    void ApplyChanges();
    void Refresh();

private:
    void SaveConfig();
    void LoadConfig();
    void RebuildVFS();

};