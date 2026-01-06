#include "memory.hpp"
#include "main.hpp"

template<typename T>
uintptr_t GetAddressBySignature(const T& sig) {
    HMODULE hMod = GetModuleHandle(nullptr);
    MODULEINFO modInfo;
    GetModuleInformation(GetCurrentProcess(), hMod, &modInfo, sizeof(modInfo));

    uintptr_t startAddr = (uintptr_t)modInfo.lpBaseOfDll;
    uintptr_t endAddr = startAddr + modInfo.SizeOfImage;

    uintptr_t found = SigSearch::FindSignatureInRange(startAddr, endAddr, sig);

    return found;
}

uintptr_t FindCompressedCreate() {
    uintptr_t anchorAddr = GetAddressBySignature(CompressedCreateAnchorSig);
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
}