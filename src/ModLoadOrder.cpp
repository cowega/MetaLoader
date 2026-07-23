#include "ModLoadOrder.hpp"
#include "Loader.hpp"
#include "ZipManager.hpp"
#include "utils.hpp"

#include <fstream>
#include <spdlog/spdlog.h>
#include <string_view>

ModLoadOrder::ModLoadOrder() {
    this->LoadConfig();
}

ModLoadOrder::~ModLoadOrder() { }

std::vector<ModConfig>& ModLoadOrder::GetModsForUI() {
    return this->mods;
}

const std::string_view ModLoadOrder::GetFile(std::string_view virtualPath) {
    auto it = vfs.find(virtualPath); 
    if (it != vfs.end()) {
        return it->second;
    }
    return {};
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

void ModLoadOrder::ProcessAllZipFiles() {
    std::vector<std::string> zipFiles;

    for (const auto& entry : fs::directory_iterator("metaloader")) {
        if (entry.is_regular_file() && entry.path().extension() == ".zip") {
            zipFiles.push_back(entry.path().filename().string());
        }
    }

    for (const auto& archiveName : zipFiles) {
        bool unpacked = Loader::zip->extractSmart(archiveName);
        if (unpacked) {
            spdlog::info("{} archive successfully unpacked", archiveName);
        } else {
            spdlog::warn("{} archive could not be processed", archiveName);
        }

        fs::path zipPath = fs::path("metaloader") / archiveName;
        std::error_code ec;
        if (fs::remove(zipPath, ec)) {
            spdlog::info("{} archive successfully deleted", archiveName);
        } else {
            spdlog::error("Failed to delete archive {}: {}", archiveName, ec.message());
        }
    }
}

void ModLoadOrder::LoadConfig() {
    this->mods.clear();
    bool changed = false;
    this->mods = Loader::settings->GetMods();
    this->ProcessAllZipFiles();

    auto it = this->mods.begin();
    while (it != this->mods.end()) {
        fs::path modPath = fs::path("metaloader") / it->name;
        if (!fs::exists(modPath) || !fs::is_directory(modPath)) {
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

            if (Utils::HasCyrillic(entry.path().filename().wstring())) {
                spdlog::warn("Mod \"{}\" skipped: Cyrillic in name. Rename folder to English to load.", name);
                continue;
            }
            
            auto it = std::find_if(this->mods.begin(), this->mods.end(), 
                [&](const ModConfig& m){ return m.name == name; });

            if (it == this->mods.end()) {
                spdlog::info("Found new mod folder: {}", name);
                this->mods.push_back({name, {}, true});
                changed = true;
            }
        }
    }

    for (auto& mod : this->mods) {
        std::vector<std::string> newAssets;
        
        fs::path modDataPath = fs::path("metaloader") / mod.name / "data";

        if (fs::exists(modDataPath) && fs::is_directory(modDataPath)) {
            for (const auto& file : fs::recursive_directory_iterator(modDataPath)) {
                if (file.is_regular_file()) {
                    fs::path relativePath = fs::relative(file.path(), fs::path("metaloader") / mod.name);
                    
                    newAssets.push_back(relativePath.generic_string());
                }
            }
        }

        if (mod.assets != newAssets) {
            mod.assets = std::move(newAssets);
            changed = true;
        }
    }

    if (changed) this->SaveConfig();

    this->RebuildVFS();
}

void ModLoadOrder::RebuildVFS() {
    this->vfs.clear();

    for (auto it = this->mods.rbegin(); it != this->mods.rend(); it++) {
        auto& mod = *it;
        if (!mod.enabled) continue;

        fs::path modPath = fs::path("metaloader") / mod.name;
        if (!fs::exists(modPath)) {
            spdlog::warn("Mod directory not found: {}", mod.name);
            continue;
        }

        for (const auto& assetRelativePath : mod.assets) {
            std::string key = assetRelativePath;
            for (char& c : key) {
                if (c >= 'A' && c <= 'Z') c = char(c + ('a' - 'A'));
            }
            
            fs::path filePath = modPath / assetRelativePath;
            this->vfs[key] = fs::absolute(filePath).string();
        }
    }
}
