#include "main.hpp"
#include "utils.hpp"

extern "C" __declspec(dllexport) void DummyExport() { return; }

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH:
        if (GetCurrentProcessName().find("blitz") == std::string::npos) return TRUE;

        DisableThreadLibraryCalls(hModule);
        CreateThread(nullptr, 0, (LPTHREAD_START_ROUTINE)MainThread, hModule, 0, nullptr);
        break;

    case DLL_PROCESS_DETACH:
        g_isRun = false;
        Sleep(200);

        if (g_isLoggerReady) {
            spdlog::info("Metaloader unloading");
            spdlog::default_logger()->flush();
            spdlog::shutdown();
        }

        MH_DisableHook(MH_ALL_HOOKS);
        MH_Uninitialize();
        break;
    }
    return TRUE;
}