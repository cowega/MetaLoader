#pragma once

#include <string>

class Localization {
private:
    int langIndex;

public:
    Localization();
    ~Localization();

    std::string Get(const char* key);
    void SetLang(int index);
    int GetLang();

};
