#include "Settings.hpp"

#include <fstream>
#include <spdlog/spdlog.h>

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(ModConfig, name, enabled);

Settings::Settings() {
    this->Load();
}

Settings::~Settings() {
    this->Save();
}

void Settings::Load() {
    if (!fs::exists("metaloader")) {
        fs::create_directory("metaloader");
    }

    if (fs::exists("metaloader/settings.json")) {
        std::ifstream f("metaloader/settings.json");
        
        if (!f.is_open()) {
            spdlog::error("Failed to open settings.json");
            return;
        }

        try {
            f >> this->json;
            spdlog::info("Config loaded from settings.json");
        } catch (const nlohmann::json::parse_error& e) {
            spdlog::error("JSON parse error: {}", e.what());
            this->json = nlohmann::json::object();
        }
    } else {
        spdlog::warn("settings.json not found, using default config");
        this->json = nlohmann::json::object();
    }
}

void Settings::Save() {
    if (this->json.empty()) return;

    std::ofstream o("metaloader/settings.json");
    if (o.is_open()) {
        o << this->json.dump(4);
        spdlog::info("Config saved to settings.json");
    } else {
        spdlog::error("Error to save config!");
    }
}

std::vector<ModConfig> Settings::GetMods() {
    if (!this->json.contains("mods")) return { };

    return this->json["mods"].get<std::vector<ModConfig>>();
}

void Settings::SetMods(const std::vector<ModConfig>& mods) {
    this->json["mods"] = mods;
    this->Save();
}

int Settings::GetLanguageIndex() {
    if (!this->json.contains("language")) return 1;

    return this->json["language"].get<int>();
}

void Settings::SetLanguageIndex(const int& index) {
    this->json["language"] = index;
    this->Save();
}