#pragma once

#include <atomic>

#include "GameHook.hpp"

#define VERSION "2.0.0"
#define BUILD_DATE __DATE__
#define BUILD_TIME __TIME__

class Loader {
public:
    static inline std::atomic<bool> g_isRun = true;
    static inline std::atomic<bool> g_isLoggerReady = false;

public:
    Loader();
    ~Loader();

private:
    GameHook* hook = nullptr;

private:
    void InitLogger();

};