#pragma once
#pragma comment(lib, "psapi.lib")

#include <Windows.h>
#include <psapi.h>
#include <atomic>
#include <mutex>

#include "SigSearch.hpp"
using namespace SigSearch::literals;

#include "ModLoadOrder.hpp"

inline const auto StatSig = "48 8B 05 ?? ?? ?? ?? 48 8D 4C 24 ?? 48 83 7C 24 ?? 07 48 0F 47 4C 24 ?? 48 8D 54 24 ?? FF D0 85 C0 75 0B 0F B7 5C 24 ?? 66 C1 EB 0F"_sig;
inline const auto CompressedCreateAnchorSig = "41 B8 25 00 00 00 48 8D 15 ?? ?? ?? ?? 48 8D 4D C0"_sig;

typedef __int64 (__fastcall *tStat)(void* pathPtr, void* statBuf);
typedef __int64(__fastcall* tCompressedCreate)(const char* originalPathObj, void** a2, unsigned int a3);

inline tStat fnStat = 0;
inline uintptr_t* ptrOrigStat = 0;
inline tCompressedCreate fpCompressedCreate = 0; 

class GameHook {
public:
    inline static ModLoadOrder* mlo;
    GameHook(std::atomic_bool& successInit);
    ~GameHook();
    bool CreateHook();
    
private:
    inline static __int64 __fastcall hkStat(void* pathPtr, void* statBuf);
    inline static __int64 __fastcall hkCompressedCreate(const char* originalPathObj, void** a2, unsigned int a3);

};