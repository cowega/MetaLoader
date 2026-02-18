#include <string>
#include <filesystem>
#include <windows.h> 
#include <algorithm>

#include "GameHook.hpp"
#include "Loader.hpp"
#include "utils.hpp"

#include <MinHook.h>
#include <spdlog/spdlog.h>

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
    auto getAnchorAddressBySig = [](auto signature) -> uintptr_t {
        HMODULE hMod = GetModuleHandle(nullptr);
        MODULEINFO modInfo;
        GetModuleInformation(GetCurrentProcess(), hMod, &modInfo, sizeof(modInfo));

        uintptr_t startAddr = (uintptr_t)modInfo.lpBaseOfDll;
        uintptr_t endAddr = startAddr + modInfo.SizeOfImage;

        uintptr_t found = SigSearch::FindSignatureInRange(startAddr, endAddr, signature);

        if (!found) spdlog::error("Anchor signature outdated!");

        return found;
    };

    auto findFunctionStartByAnchor = [getAnchorAddressBySig](auto anchorSig) -> uintptr_t {
        uintptr_t anchorAddr = getAnchorAddressBySig(anchorSig);
        if (!anchorAddr) return 0;
        unsigned char* ptr = (unsigned char*)anchorAddr;
        for (int i = 0; i < 300; i++, ptr--) {
            unsigned char prev = *(ptr - 1);
            if ((prev == 0xCC || prev == 0xC3)) {
                return (uintptr_t)ptr;
            }
        }

        return 0;
    };

    uintptr_t CompressedCreateAddr = findFunctionStartByAnchor(CompressedCreateAnchorSig);
    if (!CompressedCreateAddr) {
        spdlog::error("Failed to found function start CompressedCreate's address!");
        return 0;
    }

    MH_STATUS status;
    status = MH_CreateHook((LPVOID)CompressedCreateAddr, &GameHook::hkCompressedCreate, (LPVOID*)&fpCompressedCreate);
    if (status != MH_OK) {
        spdlog::error("Failed to create CompressedCreate hook: {}", MH_StatusToString(status));
        MH_Uninitialize();
        return 0;
    }

    auto GetQwordSlotFromMov = [](uintptr_t movAddr) -> uintptr_t {
        int32_t disp = *reinterpret_cast<int32_t*>(movAddr + 3);
        return (movAddr + 7) + disp;
    };

    uintptr_t anchor = getAnchorAddressBySig(StatSig);
    if (!anchor) return 0;
    uintptr_t qwordAddr = GetQwordSlotFromMov(anchor);

    ptrOrigStat = reinterpret_cast<uint64_t*>(qwordAddr);
    fnStat = reinterpret_cast<tStat>(*ptrOrigStat);

    DWORD oldProt{};
    VirtualProtect(ptrOrigStat, sizeof(*ptrOrigStat), PAGE_READWRITE, &oldProt);
    *ptrOrigStat = reinterpret_cast<uint64_t>(&GameHook::hkStat);
    VirtualProtect(ptrOrigStat, sizeof(*ptrOrigStat), oldProt, &oldProt);

    status = MH_EnableHook(MH_ALL_HOOKS);
    if (status != MH_OK) {
        spdlog::error("Failed to enable hooks: {}", MH_StatusToString(status));
        MH_Uninitialize();
        return 0;
    }

    spdlog::info("Stat hook active at 0x{:X}", qwordAddr);
    spdlog::info("CompressedCreate hook active at 0x{:X}\n", CompressedCreateAddr);
    return 1;
}

__int64 __fastcall GameHook::hkStat(void* pathPtr, void* statBuf) {
    const wchar_t* wpath = reinterpret_cast<const wchar_t*>(pathPtr);

    if (wpath) {
        std::wstring ws(wpath);
        std::string modKey(ws.begin(), ws.end());

        auto modPath = GameHook::mlo->GetFile(Utils::Hook::CutRawGamePath(modKey).string());
        if (!modPath.empty()) {
            modPath = fs::absolute(modPath).string();
            std::replace(modPath.begin(), modPath.end(), '\\', '/');
            spdlog::info("[hkStat]: {}", modPath);

            std::wstring wmod(modPath.begin(), modPath.end());
            return fnStat((void*)wmod.c_str(), statBuf);
        }
    }

    return fnStat(pathPtr, statBuf);
}

__int64 __fastcall GameHook::hkCompressedCreate(const char* originalPathObj, void** a2, unsigned int a3) {
    if (!originalPathObj || !GameHook::mlo) return fpCompressedCreate(originalPathObj, a2, a3);

    auto modPath = GameHook::mlo->GetFile(Utils::Hook::CutRawGamePath(originalPathObj).string());
    
    if (modPath.empty()) return fpCompressedCreate(originalPathObj, a2, a3);
    
    alignas(16) char fakeObject[1024];
    memset(fakeObject, 0, 1024);

    if (modPath.size() >= 511) {
        if (Loader::g_isLoggerReady) {
            spdlog::error("Path too long! ({}/511)", modPath.size());
        }
    }

    strcpy_s(fakeObject, 512, modPath.c_str());
    *(size_t*)(fakeObject + 512) = modPath.length();

    spdlog::info("[hkCompressedCreate]: {}", modPath);
    return fpCompressedCreate(fakeObject, a2, a3);
}
