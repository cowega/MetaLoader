#pragma once

#include <string>

class Localization {
private:
    int langIndex = 0;

public:
    std::string Get(const char* key);
    void SetLang(int index);
    int GetLang();
};
