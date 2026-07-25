#pragma once

#include <string>
#include <unordered_map>

struct TranslationEntry {
    std::string ru;
    std::string en;
};

class Localization {
private:
    int langIndex;
    static inline std::unordered_map<std::string, TranslationEntry> translations;
    void LoadLocalizedText();

public:
    Localization();
    ~Localization();
    
    const std::string& Get(const char* key);
    void SetLang(int index);
    int GetLang();

};
