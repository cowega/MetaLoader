#include "Localization.hpp"
#include "Locales.hpp"

std::string Localization::Get(const char* key) {
    std::string lang = (this->langIndex == 0) ? "ru" : "en";
    return Locales::locale[key][lang].get<std::string>();
}

void Localization::SetLang(int index) {
    this->langIndex = index;
}

int Localization::GetLang() {
    return this->langIndex;
}