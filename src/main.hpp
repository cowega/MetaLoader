#pragma once

#include <Windows.h>
#include <string>
#include <vector>
#include <filesystem>

#include <MinHook.h>
#include <spdlog/spdlog.h>
#include <spdlog/async.h>
#include <spdlog/sinks/basic_file_sink.h>

#define VERSION "1.0.0"
#define BUILD_DATE __DATE__
#define BUILD_TIME __TIME__

inline std::atomic<bool> g_isRun = true;
inline std::atomic<bool> g_isLoggerReady = false;
inline const uintptr_t CompressedCreateAddr = 0x1AED4A0;

typedef __int64(__fastcall* tCompressedCreate)(const char* pathObj, void** allocator, unsigned int a3);
inline tCompressedCreate CompressedCreate = NULL;