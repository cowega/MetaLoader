#include "Localization.hpp"
#include "Locales.hpp"
#include "Loader.hpp"

Localization::Localization() {
    this->langIndex = this->GetLang();
}

Localization::~Localization() { }

std::string Localization::Get(const char* key) {
    std::string lang;
    switch (this->langIndex) {
    case 0:
        lang = "ru";
        break;
    case 1:
        lang = "en";
        break;

    default:
        lang = "en";
        break;
    }

    return Locales::locale[key][lang].get<std::string>();
}

void Localization::SetLang(int index) {
    this->langIndex = index;
    Loader::settings->SetLanguageIndex(index);
}

int Localization::GetLang() {
    return Loader::settings->GetLanguageIndex();
}