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
            {"ru", "Перезагрузить список"},
            {"en", "Reload list"}
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
        }}
    };
}