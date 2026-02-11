#include <Windows.h>

#include "utils.hpp"
#include "Loader.hpp"

extern "C" __declspec(dllexport) void DummyExport() { return; }

void MainThread(HMODULE hModule) {
    do {
        Sleep(100);
    } while (Utils::GetMyWindow() == NULL);

    Loader* loader = new Loader();

    while (Loader::g_isRun) {
        Sleep(100);
    }

    delete loader;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH:
        if (Utils::GetCurrentProcessName().find("blitz") == std::string::npos) return TRUE;

        DisableThreadLibraryCalls(hModule);
        CreateThread(nullptr, 0, (LPTHREAD_START_ROUTINE)MainThread, hModule, 0, nullptr);
        break;

    case DLL_PROCESS_DETACH:
        
        break;
    }
    return TRUE;
}