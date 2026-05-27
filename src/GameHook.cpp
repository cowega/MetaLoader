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
    auto findFunctionStartByXref = [](const std::string& str) -> uintptr_t {
        uintptr_t xrefAddr = Utils::FindXRef(str);
        if (!xrefAddr) {
            spdlog::error("Failed to find XREF for string: {}", str);
            return 0;
        }

        unsigned char* ptr = (unsigned char*)xrefAddr;
        for (int i = 0; i < 300; i++, ptr--) {
            unsigned char prev = *(ptr - 1);
            if (prev == 0xCC || prev == 0xC3) {
                return (uintptr_t)ptr;
            }
        }

        spdlog::error("Failed to find function start for string: {}", str);
        return 0;
    };

    uintptr_t CompressedCreateAddr = findFunctionStartByXref("File::CompressedCreate");
    uintptr_t StatAddr = findFunctionStartByXref("tb::LowLevelFileApi::IsRegularFile");

    if (!CompressedCreateAddr || !StatAddr) return 0;

    MH_STATUS status;
    status = MH_CreateHook((LPVOID)CompressedCreateAddr, &GameHook::hkCompressedCreate, (LPVOID*)&fpCompressedCreate);
    if (status != MH_OK) {
        spdlog::error("Failed to create CompressedCreate hook: {}", MH_StatusToString(status));
        return 0;
    }

    status = MH_CreateHook((LPVOID)StatAddr, &GameHook::hkStat, (LPVOID*)&fnStat);
    if (status != MH_OK) {
        spdlog::error("Failed to create Stat hook: {}", MH_StatusToString(status));
        return 0;
    }

    status = MH_EnableHook(MH_ALL_HOOKS);
    if (status != MH_OK) {
        spdlog::error("Failed to enable hooks: {}", MH_StatusToString(status));
        return 0;
    }

    spdlog::info("Stat hook active at 0x{:X}", StatAddr);
    spdlog::info("CompressedCreate hook active at 0x{:X}\n", CompressedCreateAddr);
    return 1;
}

__int64 __fastcall GameHook::hkStat(void* pathObj) {
    if (!pathObj || !GameHook::mlo) return fnStat(pathObj);

    const char* originalPathStr = reinterpret_cast<const char*>(pathObj);
    const fs::path* modPath = GameHook::mlo->GetFile(Utils::Hook::CutRawGamePath(originalPathStr).string());

    if (!modPath) return fnStat(pathObj);

    std::string modPathStr = modPath->string();
    spdlog::info("Stat: {} -> {}", originalPathStr, modPathStr);

    if (modPathStr.size() >= 511) {
        spdlog::error("Path too long in Stat! ({}/511)", modPathStr.size());
        return fnStat(pathObj);
    }

    alignas(16) char fakeObject[1024];
    memset(fakeObject, 0, sizeof(fakeObject));
    strcpy_s(fakeObject, 512, modPathStr.c_str());
    *(size_t*)(fakeObject + 512) = modPathStr.length();

    return fnStat(fakeObject);
}

__int64 __fastcall GameHook::hkCompressedCreate(const char* originalPathObj, void** a2, unsigned int a3) {
    if (!originalPathObj || !GameHook::mlo) return fpCompressedCreate(originalPathObj, a2, a3);

    const fs::path* modPath = GameHook::mlo->GetFile(Utils::Hook::CutRawGamePath(originalPathObj).string());
    if (!modPath) return fpCompressedCreate(originalPathObj, a2, a3);

    std::string modPathStr = modPath->string();
    spdlog::info("CompressedCreate: {} -> {}", originalPathObj, modPathStr);

    if (modPathStr.size() >= 511) {
        spdlog::error("Path too long! ({}/511)", modPathStr.size());
        return fpCompressedCreate(originalPathObj, a2, a3);
    }

    alignas(16) char fakeObject[1024];
    memset(fakeObject, 0, sizeof(fakeObject));
    strcpy_s(fakeObject, 512, modPathStr.c_str());
    *(size_t*)(fakeObject + 512) = modPathStr.length();

    return fpCompressedCreate(fakeObject, a2, a3);
}
