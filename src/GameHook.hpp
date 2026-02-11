#pragma once
#pragma comment(lib, "psapi.lib")

#include <Windows.h>
#include <psapi.h>
#include <atomic>
#include <mutex>

#include "SigSearch.hpp"
using namespace SigSearch::literals;

#include "ModLoadOrder.hpp"

inline const auto ResolveSig = "48 8D 05 ?? ?? ?? ?? 48 8D 0D ?? ?? ?? ?? C7 44 24 70 BF 00 00 00"_sig;
inline const auto CompressedCreateAnchorSig = "41 B8 25 00 00 00 48 8D 15 ?? ?? ?? ?? 48 8D 4D C0"_sig;

typedef __int64 (__fastcall *tPathResolveChain__Resolve)(__int64 a1, __int64 a2, __int64 a3, unsigned int a4, __int64 a5);
typedef __int64(__fastcall* tCompressedCreate)(const char* originalPathObj, void** a2, unsigned int a3);
typedef void* (__cdecl* tMalloc)(size_t size);

inline tPathResolveChain__Resolve fpPathResolveChain__Resolve = 0;
inline tCompressedCreate fpCompressedCreate = 0; 
inline tMalloc gameMalloc = nullptr;

inline std::mutex Mutex;
inline bool HaveTemplate = false;
inline unsigned char TemplateValue[0x30] = {};

class GameHook {
public:
    inline static ModLoadOrder* mlo;
    GameHook(std::atomic_bool& successInit);
    ~GameHook();
    bool CreateHook();
    
private:
    inline static __int64 __fastcall hkPathResolveChain__Resolve(__int64 a1, __int64 a2, __int64 a3, unsigned int a4, __int64 a5);
    inline static __int64 __fastcall hkCompressedCreate(const char* originalPathObj, void** a2, unsigned int a3);

};