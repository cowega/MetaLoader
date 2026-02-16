#pragma once

#include <atomic>

#include "GameHook.hpp"
#include "LoaderUI.hpp"
#include "Settings.hpp"
#include "Version.hpp"

#define VERSION "v2.2.0"
#define BUILD_DATE __DATE__
#define BUILD_TIME __TIME__

class Loader {
public:
    static inline std::atomic<bool> g_isRun = true;
    static inline std::atomic<bool> g_isLoggerReady = false;
    static inline Settings* settings;

public:
    Loader();
    ~Loader();

private:
    GameHook* hook = nullptr;
    LoaderUI* render = nullptr;
    Version* version = nullptr;

private:
    void InitLogger();

};