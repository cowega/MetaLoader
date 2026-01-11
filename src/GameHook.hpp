#pragma once
#pragma comment(lib, "psapi.lib")

#include <Windows.h>
#include <psapi.h>
#include <atomic>

#include "SigSearch.hpp"
using namespace SigSearch::literals;

inline const auto CompressedCreateAnchorSig = "41 B8 25 00 00 00 48 8D 15 ?? ?? ?? ?? 48 8D 4D C0"_sig;
typedef __int64(__fastcall* tCompressedCreate)(const char* pathObj, void** allocator, unsigned int a3);
inline tCompressedCreate CompressedCreate = NULL;

class GameHook {
public:
    GameHook(std::atomic_bool& successInit);
    ~GameHook();
    bool CreateHook();
    
private:
    static __int64 __fastcall Hook(const char* originalPathObj, void** a2, unsigned int a3);

};