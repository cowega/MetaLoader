#include "Localization.hpp"
#include "Locales.hpp"
#include "Loader.hpp"

Localization::Localization() {
    this->langIndex = this->GetLang();
    this->LoadLocalizedText();
}

Localization::~Localization() { }

void Localization::LoadLocalizedText() {
    Localization::translations.clear();
    
    for (const auto& [key, value] : Locales::locale.items()) {
        TranslationEntry entry;
        
        if (value.contains("ru")) {
            entry.ru = value["ru"].get<std::string>();
        } else {
            entry.ru = key;
        }

        if (value.contains("en")) {
            entry.en = value["en"].get<std::string>();
        } else {
            entry.en = key;
        }

        Localization::translations[key] = std::move(entry);
    }

}

const std::string& Localization::Get(const char* key) {
    const std::string currentLang = (this->langIndex == 0) ? "ru" : "en";
    
    auto it = Localization::translations.find(key);
    if (it != Localization::translations.end()) {
        return (currentLang == "ru") ? it->second.ru : it->second.en;
    }

    static const std::string fallback = "???";
    return fallback;
}

void Localization::SetLang(int index) {
    this->langIndex = index;
    Loader::settings->SetLanguageIndex(index);
}

int Localization::GetLang() {
    return Loader::settings->GetLanguageIndex();
}