#pragma once

#include "ModLoadOrder.hpp"

#include <nlohmann/json.hpp>

namespace fs = std::filesystem;
using json = nlohmann::json;

class Settings {
public:
    Settings();
    ~Settings();

    void Load();
    void Save();

    std::vector<ModConfig> GetMods();
    void SetMods(const std::vector<ModConfig>& mods);

    int GetLanguageIndex();
    void SetLanguageIndex(const int& index);

private:
    json json;

};