#pragma once

#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace Locales {
    inline const char* languages[] = {"Русский", "English"};
    inline const json locale = {
        {"MODS_LIST_TITLE", {
            {"ru", "Настройте приоритет загрузки модов"},
            {"en", "Configure mod load priority"}
        }},
        {"RELOAD_LIST", {
            {"ru", "Перезагрузить список\nМожет вызвать зависание"},
            {"en", "Reload list\nMay cause freeze"}
        }},
        {"OPEN_FOLDER", {
            {"ru", "Открыть папку с модами"},
            {"en", "Open mods folder"}
        }},
        {"SELECTED_LANG", {
            {"ru", "Русский"},
            {"en", "English"}
        }},
        {"MODS_LIST_TITLE_TOOLTIP", {
            {"ru", "Потяните за мод, чтобы изменить его позицию в списке\nИзменения вступят в силу после загрузки игрой ресурсов"},
            {"en", "Drag a mod to change its position in the list\nChanges will take effect after the game loads resources"}
        }},
        {"NOTF_ML_LOADED", {
            {"ru", "Добро пожаловать!"},
            {"en", "Welcome!"}
        }},
        {"NOTF_MODLIST_REFRESHED", {
            {"ru", "Список модификаций обновлён"},
            {"en", "Mod list refreshed"}
        }},
        {"NOTF_MODS_ADDED", {
            {"ru", "Добавлено модов: %d"},
            {"en", "Added mods: %d"}
        }},
        {"NOTF_MODS_REMOVED", {
            {"ru", "Удалено модов: %d"},
            {"en", "Removed mods: %d"}
        }},
        {"NOTF_MODS_MISSED", {
            {"ru", "Пропущено модов: %d"},
            {"en", "Mods skipped: %d"}
        }}
    };
}