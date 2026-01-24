#include <filesystem>

#include <MinHook.h>
#include <spdlog/spdlog.h>

#include "GameHook.hpp"
#include "Loader.hpp"
#include "utils.hpp"

namespace fs = std::filesystem;

GameHook::GameHook(std::atomic_bool& successInit) {
    MH_STATUS status = MH_Initialize();
    if (status != MH_OK) {
        spdlog::error("Failed to init MH: {}", MH_StatusToString(status));
        successInit = false;
        return;
    }
    if (!this->mlo) this->mlo = new ModLoadOrder();
}

GameHook::~GameHook() {
    MH_DisableHook(MH_ALL_HOOKS);
    MH_Uninitialize();
    delete this->mlo;
}

bool GameHook::CreateHook() {
    auto getAnchorAddressBySig = []() -> uintptr_t {
        HMODULE hMod = GetModuleHandle(nullptr);
        MODULEINFO modInfo;
        GetModuleInformation(GetCurrentProcess(), hMod, &modInfo, sizeof(modInfo));

        uintptr_t startAddr = (uintptr_t)modInfo.lpBaseOfDll;
        uintptr_t endAddr = startAddr + modInfo.SizeOfImage;

        uintptr_t found = SigSearch::FindSignatureInRange(startAddr, endAddr, CompressedCreateAnchorSig);

        return found;
    };

    auto findCompressedCreate = [getAnchorAddressBySig]() -> uintptr_t {
        uintptr_t anchorAddr = getAnchorAddressBySig();
        if (!anchorAddr) {
            spdlog::error("Anchor signature outdated!");
            return 0;
        }

        unsigned char* ptr = (unsigned char*)anchorAddr;
        for (int i = 0; i < 300; i++, ptr--) {
            unsigned char prev = *(ptr - 1);
            if ((prev == 0xCC || prev == 0xC3) && *ptr == 0x48) {
                spdlog::info("Function starts at 0x{:X} (dist: {})", (uintptr_t)ptr, i);
                return (uintptr_t)ptr;
            }
        }

        spdlog::error("Failed to found function start address!");
        return 0;
    };

    uintptr_t CompressedCreateAddr = findCompressedCreate();
    if (!CompressedCreateAddr) return 0;

    MH_STATUS status = MH_CreateHook((LPVOID)CompressedCreateAddr, &GameHook::Hook, (LPVOID*)&CompressedCreate);
    if (status != MH_OK) {
        spdlog::error("Failed to create hook: {}", MH_StatusToString(status));
        MH_Uninitialize();
        return 0;
    }

    status = MH_EnableHook(MH_ALL_HOOKS);
    if (status != MH_OK) {
        spdlog::error("Failed to enable hook: {}", MH_StatusToString(status));
        MH_Uninitialize();
        return 0;
    }

    spdlog::info("Hook active at 0x{:X}\n", CompressedCreateAddr);
    return 1;
}

__int64 __fastcall GameHook::Hook(const char* originalPathObj, void** a2, unsigned int a3) {
    if (!originalPathObj || !GameHook::mlo) return CompressedCreate(originalPathObj, a2, a3);

    auto modPath = GameHook::mlo->GetFile(Utils::CutRawGamePath(originalPathObj).string());
    
    if (modPath.empty()) return CompressedCreate(originalPathObj, a2, a3);
    
    alignas(16) char fakeObject[1024];
    memset(fakeObject, 0, 1024);

    if (modPath.size() >= 511) {
        if (Loader::g_isLoggerReady) {
            spdlog::error("Path too long! ({}/511)", modPath.size());
        }
    }

    strcpy_s(fakeObject, 512, modPath.c_str());
    *(size_t*)(fakeObject + 512) = modPath.length();

    return CompressedCreate(fakeObject, a2, a3);
}
