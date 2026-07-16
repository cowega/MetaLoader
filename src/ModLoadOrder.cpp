#include "ModLoadOrder.hpp"
#include "Loader.hpp"
#include "ZipManager.hpp"

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

const fs::path* ModLoadOrder::GetFile(std::string_view virtualPath) {
    thread_local std::string key;
    key.assign(virtualPath);

    for (char& c : key) {
        if (c == '\\') c = '/';
        if (c >= 'A' && c <= 'Z') c = char(c + ('a' - 'A'));
    }

    auto it = vfs.find(key);
    if (it != vfs.end()) return &it->second;
    return 0;
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
                for (char& c : key) {
                    if (c == '\\') c = '/';
                    if (c >= 'A' && c <= 'Z') c = char(c + ('a' - 'A'));
                }

                this->vfs[key] = fs::absolute(entry.path());
            }
        }
    }
}