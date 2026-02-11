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

    uintptr_t ResolveAddr = findFunctionStartByAnchor(ResolveSig);
    if (!ResolveAddr) {
        spdlog::error("Failed to found function start Resolve's address!");
        return 0;
    }

    uintptr_t CompressedCreateAddr = findFunctionStartByAnchor(CompressedCreateAnchorSig);
    if (!CompressedCreateAddr) {
        spdlog::error("Failed to found function start CompressedCreate's address!");
        return 0;
    }

    MH_STATUS status;
    status = MH_CreateHook((LPVOID)(ResolveAddr), &GameHook::hkPathResolveChain__Resolve, (LPVOID*)&fpPathResolveChain__Resolve);    
    if (status != MH_OK) {
        spdlog::error("Failed to create Resolve hook: {}", MH_StatusToString(status));
        MH_Uninitialize();
        return 0;
    }

    status = MH_CreateHook((LPVOID)CompressedCreateAddr, &GameHook::hkCompressedCreate, (LPVOID*)&fpCompressedCreate);
    if (status != MH_OK) {
        spdlog::error("Failed to create CompressedCreate hook: {}", MH_StatusToString(status));
        MH_Uninitialize();
        return 0;
    }
    
    status = MH_EnableHook(MH_ALL_HOOKS);
    if (status != MH_OK) {
        spdlog::error("Failed to enable hooks: {}", MH_StatusToString(status));
        MH_Uninitialize();
        return 0;
    }

    spdlog::info("Resolve hook active at 0x{:X}", ResolveAddr);
    spdlog::info("CompressedCreate hook active at 0x{:X}\n", CompressedCreateAddr);
    return 1;
}

__int64 __fastcall GameHook::hkPathResolveChain__Resolve(__int64 a1, __int64 a2, __int64 a3, unsigned int a4, __int64 a5) {
    if (!gameMalloc) {
        Utils::Hook::InitGameAllocator();
    }

    __int64 ret = fpPathResolveChain__Resolve(a1, a2, a3, a4, a5);
    if (!a2) return ret;

    auto* opt = (Utils::Hook::OptionalResolveResult*)a2;
    bool hasValue = (opt->has_value != 0);

    if (hasValue) {
        std::lock_guard<std::mutex> lk(Mutex);
        memcpy(TemplateValue, opt->value, 0x30);
        HaveTemplate = true;
    }

    if (!a3 || !GameHook::mlo)
        return ret;

    std::string vPath = Utils::Hook::TryReadMSVCString((void*)a3);
    if (vPath.empty())
        return ret;

    if (vPath.rfind("~res:/", 0) != 0 && vPath.rfind("~doc:/", 0) != 0)
        return ret;

    std::string cleanPath = vPath.substr(6);
    std::string searchKey = "Data/" + cleanPath + ".dvpl";

    size_t pos = searchKey.find(".pvr");
    if (pos != std::string::npos) searchKey.replace(pos, 4, ".dds");

    std::string modPath_ = GameHook::mlo->GetFile(searchKey);
    if (modPath_.empty())
        return ret;

    unsigned char tpl[0x30]; {
        std::lock_guard<std::mutex> lk(Mutex);
        if (!HaveTemplate) return ret;
        memcpy(tpl, TemplateValue, 0x30);
    }

    std::string modPath = fs::absolute(modPath_).string();
    std::replace(modPath.begin(), modPath.end(), '\\', '/');

    if (!gameMalloc)
        return ret;

    const size_t len = modPath.size();
    char* mem = (char*)gameMalloc(len + 1);
    if (!mem)
        return ret;

    memcpy(mem, modPath.c_str(), len);
    mem[len] = '\0';

    Utils::Hook::SetQword(tpl, 0x00, mem);
    Utils::Hook::SetU64(tpl, 0x08, (unsigned long long)len);

    memcpy(opt->value, tpl, 0x30);
    opt->has_value = 1;

    spdlog::info("[Resolve] Redirect: {} -> {}", vPath, modPath);

    return ret;
}

__int64 __fastcall GameHook::hkCompressedCreate(const char* originalPathObj, void** a2, unsigned int a3) {
    if (!originalPathObj || !GameHook::mlo) return fpCompressedCreate(originalPathObj, a2, a3);
    std::string in = originalPathObj;
    if (in.empty()) {
        return fpCompressedCreate(originalPathObj, a2, a3);
    }

    std::string fixed = Utils::Hook::FixCompressedPath(in);

    if (fixed == in) {
        return fpCompressedCreate(originalPathObj, a2, a3);
    }

    alignas(16) char fakeObject[1024];
    memset(fakeObject, 0, sizeof(fakeObject));

    if (fixed.size() >= 511) {
        spdlog::error("[CompressedCreate] Path too long! ({}/511)", fixed.size());
        return fpCompressedCreate(originalPathObj, a2, a3);
    }

    strcpy_s(fakeObject, 512, fixed.c_str());
    *(size_t*)(fakeObject + 512) = fixed.length();

    spdlog::info("[CompressedCreate] Fix: {} -> {}", in, fixed);

    return fpCompressedCreate(fakeObject, a2, a3);
}
