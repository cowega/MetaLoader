#include "ModLoadOrder.hpp"
#include "Loader.hpp"

#include <fstream>
#include <spdlog/spdlog.h>

ModLoadOrder::ModLoadOrder() {
    this->LoadConfig();
}

ModLoadOrder::~ModLoadOrder() { }

std::vector<ModConfig>& ModLoadOrder::GetModsForUI() {
    return this->mods;
}

std::string ModLoadOrder::GetFile(const std::string& virtualPath) {
    std::string path = virtualPath; 
    std::replace(path.begin(), path.end(), '\\', '/');
    std::transform(path.begin(), path.end(), path.begin(), ::tolower);

    auto it = this->vfs.find(path);
    if (it != this->vfs.end()) {
        spdlog::info("Redirect: {} -> {}", virtualPath, it->second.string());
        return it->second.string();
    }
    return "";
}

void ModLoadOrder::ApplyChanges() {
    this->SaveConfig();
    this->RebuildVFS();
}

void ModLoadOrder::Refresh() {
    this->LoadConfig();
}

void ModLoadOrder::SaveConfig() {
    Loader::settings->SetMods(this->mods);
}

void ModLoadOrder::LoadConfig() {
    this->mods.clear();
    bool changed = false;
    this->mods = Loader::settings->GetMods();
    
    auto it = this->mods.begin();
    while (it != this->mods.end()) {
        fs::path modPath = fs::path("metaloader") / it->name;
        if (!fs::exists(modPath)) {
            spdlog::info("Removing non-existent mod: {}", it->name);
            it = this->mods.erase(it);
            changed = true;
        } else {
            it++;
        }
    }

    for (const auto& entry : fs::directory_iterator("metaloader")) {
        if (entry.is_directory()) {
            std::string name = entry.path().filename().string();
            
            auto it = std::find_if(this->mods.begin(), this->mods.end(), 
                [&](const ModConfig& m){ return m.name == name; });

            if (it == this->mods.end()) {
                this->mods.push_back({name, true});
                changed = true;
            }
        }
    }
    if (changed) this->SaveConfig();

    this->RebuildVFS();
}

void ModLoadOrder::RebuildVFS() {
    this->vfs.clear();
    auto modsCopy = this->mods;

    for (auto it = this->mods.rbegin(); it != this->mods.rend(); it++) {
        auto& mod = *it;
        if (!mod.enabled) continue;

        fs::path modPath = fs::path("metaloader") / mod.name;
        if (!fs::exists(modPath)) {
            spdlog::warn("Mod directory not found: {}", mod.name);
            continue;
        }

        for (const auto& entry : fs::recursive_directory_iterator(modPath)) {
            if (entry.is_regular_file()) {
                std::string key = fs::relative(entry.path(), modPath).generic_string();
                std::transform(key.begin(), key.end(), key.begin(), ::tolower);

                this->vfs[key] = entry.path();
            }
        }
    }
}