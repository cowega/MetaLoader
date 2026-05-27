#pragma once
#pragma comment(lib, "psapi.lib")

#include <Windows.h>
#include <psapi.h>
#include <atomic>
#include <mutex>

#include "SigSearch.hpp"
using namespace SigSearch::literals;

#include "ModLoadOrder.hpp"

typedef __int64 (__fastcall *tStat)(void* pathObj);
typedef __int64(__fastcall* tCompressedCreate)(const char* originalPathObj, void** a2, unsigned int a3);

inline tStat fnStat = 0;
inline tCompressedCreate fpCompressedCreate = 0; 

class GameHook {
public:
    inline static ModLoadOrder* mlo;
    GameHook(std::atomic_bool& successInit);
    ~GameHook();
    bool CreateHook();
    
private:
    inline static __int64 __fastcall hkStat(void* pathObj);
    inline static __int64 __fastcall hkCompressedCreate(const char* originalPathObj, void** a2, unsigned int a3);

};