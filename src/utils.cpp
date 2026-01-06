#include "utils.hpp"
#include "main.hpp"
#include "memory.hpp"

std::string GetCurrentProcessName() {
    char procName[MAX_PATH];

    GetModuleFileNameA(NULL, procName, MAX_PATH);
    std::string filename = fs::path(procName).filename().string();

    std::transform(filename.begin(), filename.end(), filename.begin(), ::tolower);
    return filename;
}

fs::path CutRawGamePath(const fs::path& fullPath) {
    fs::path result;
    bool found = false;
    for (const auto& part : fullPath) {
        if (!found && part == "packs") {
            found = true;
            result /= "Data";
            continue;
        }

        if (!found && part == "Data") {
            found = true;
            result /= "Data";
            continue;
        }

        if (found) result /= part;
    }
    return result;
}

std::vector<fs::path> GetDirectories(const fs::path& path) {
    std::vector<fs::path> folders;
    if (!fs::exists(path) || !fs::is_directory(path)) return folders;
    try {
        for (const auto& entry : fs::directory_iterator(path))
            if (entry.is_directory()) folders.push_back(entry.path());
    }
    catch (...) {}
    return folders;
}

__int64 __fastcall CompressedCreateHook(const char* originalPathObj, void** a2, unsigned int a3) {
    if (!originalPathObj) return CompressedCreate(originalPathObj, a2, a3);

    const fs::path currentWorkDirectory = fs::current_path() / "metaloader";
    std::vector<fs::path> folders = GetDirectories(currentWorkDirectory);

    if (!folders.empty()) {
        std::string originalPathStr = originalPathObj;

        for (const fs::path& modFolder : folders) {
            fs::path DVPLPath = modFolder / CutRawGamePath(originalPathStr);
            std::error_code ec;

            if (fs::exists(DVPLPath, ec) && !ec) {
                fs::path relativeModPath = "metaloader" / modFolder.filename() / CutRawGamePath(originalPathStr);
                std::string sRelativeModPath = relativeModPath.generic_string();

                if (g_isLoggerReady) {
                    spdlog::info("Redirect: {} -> {}", originalPathObj, sRelativeModPath);
                }

                alignas(16) char fakeObject[1024];
                memset(fakeObject, 0, 1024);

                if (sRelativeModPath.size() >= 511) {
                    if (g_isLoggerReady) {
                        spdlog::error("Path too long! ({}/511)", sRelativeModPath.size());
                    }
                    continue;
                }

                strcpy_s(fakeObject, 512, sRelativeModPath.c_str());
                *(size_t*)(fakeObject + 512) = sRelativeModPath.length();

                return CompressedCreate(fakeObject, a2, a3);
            }
        }
    }

    return CompressedCreate(originalPathObj, a2, a3);
}

void init_logger() {
    auto logger = spdlog::basic_logger_mt<spdlog::async_factory>(
        "logger",
        "metaloader/metaloader.log.txt",
        true
    );
    spdlog::set_default_logger(logger);

    spdlog::set_pattern("[%H:%M:%S.%e] [%^%l%$] %v");
    spdlog::flush_every(std::chrono::seconds(1));

    g_isLoggerReady = true;
}

void MainThread(HMODULE hModule) {
    init_logger();

    spdlog::info("Metaloader v{}", VERSION);
    spdlog::info("Build: {} {}", BUILD_DATE, BUILD_TIME);
    spdlog::info("Current process: {}", GetCurrentProcessName());

    uintptr_t CompressedCreateAddr = FindCompressedCreate();
    if (!CompressedCreateAddr) return;

    MH_STATUS status = MH_Initialize();
    if (status != MH_OK) {
        spdlog::error("Failed to init MH: {}", MH_StatusToString(status));
        return;
    }

    status = MH_CreateHook((LPVOID)CompressedCreateAddr, &CompressedCreateHook, (LPVOID*)&CompressedCreate);
    if (status != MH_OK) {
        spdlog::error("Failed to create hook: {}", MH_StatusToString(status));
        MH_Uninitialize();
        return;
    }

    status = MH_EnableHook(MH_ALL_HOOKS);
    if (status != MH_OK) {
        spdlog::error("Failed to enable hook: {}", MH_StatusToString(status));
        MH_Uninitialize();
        return;
    }

    spdlog::info("Hook active at 0x{:X}\n", CompressedCreateAddr);

    while (g_isRun) {
        Sleep(100);
    }

    spdlog::info("MainThread exiting");
}