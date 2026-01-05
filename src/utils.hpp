#pragma once

#include <windows.h>
#include <string>
#include <vector>
#include <filesystem>

namespace fs = std::filesystem;

std::string GetCurrentProcessName();

fs::path CutRawGamePath(const fs::path& fullPath);

std::vector<fs::path> GetDirectories(const fs::path& path);

__int64 __fastcall CompressedCreateHook(const char* originalPathObj, void** a2, unsigned int a3);

void init_logger();

void MainThread(HMODULE hModule);