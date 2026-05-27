#pragma comment(lib, "winhttp.lib")

#include <windows.h>
#include <winhttp.h>
#include <cstdio>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

#include "Version.hpp"
#include "Loader.hpp"
#include "utils.hpp"

#ifndef WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_2
#define WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_2 0x00000800
#endif

Version::Version() { }

Version::~Version() { }

void Version::Check() {
    Version_ githubVersion;
    bool isKnowLatestVersion = this->GetLatestVersion(githubVersion);
    if (!isKnowLatestVersion) return;
    
    Version_ currentVersion;
    bool result = this->ParseVersion(VERSION, currentVersion);
    if (!result) return;

    bool isNewer = IsNewer(githubVersion, currentVersion);
    if (isNewer) {
        spdlog::warn("You are using an outdated version of MetaLoader!");

        MessageBoxW(
            Utils::GetMyWindow(),
            L"You are using an outdated version of MetaLoader.\n\n"
            L"A newer version is available at:\n"
            L"github.com/cowega/MetaLoader\n\n"
            L"The game will continue with the current version.",
            L"MetaLoader Update Available",
            MB_OK | MB_ICONWARNING | MB_SETFOREGROUND
        );
    } else {
        spdlog::info("You are using the latest version of MetaLoader!");
    }
}

std::string Version::GetLatestTag() {
    HINTERNET session = WinHttpOpen(L"Mozilla/5.0 (Windows NT 10.0; Win64; x64) MetaLoader/2.3",
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS, 0);

    if (!session) return {};

    WinHttpSetTimeouts(session, 10000, 10000, 10000, 10000);

    DWORD protocols = WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_2;
    WinHttpSetOption(session, WINHTTP_OPTION_SECURE_PROTOCOLS, &protocols, sizeof(protocols));

    HINTERNET connect = WinHttpConnect(session, L"api.github.com", INTERNET_DEFAULT_HTTPS_PORT, 0);
    if (!connect) { WinHttpCloseHandle(session); return {}; }

    HINTERNET request = WinHttpOpenRequest(connect, L"GET", L"/repos/cowega/metaloader/releases/latest",
        nullptr, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);

    if (!request) { WinHttpCloseHandle(connect); WinHttpCloseHandle(session); return {}; }

    WinHttpAddRequestHeaders(request, L"Accept: application/vnd.github.v3+json\r\n", -1, WINHTTP_ADDREQ_FLAG_ADD);

    if (!WinHttpSendRequest(request, 0, 0, 0, 0, 0, 0)) {
        WinHttpCloseHandle(request); WinHttpCloseHandle(connect); WinHttpCloseHandle(session);
        return {};
    }

    if (!WinHttpReceiveResponse(request, nullptr)) {
        WinHttpCloseHandle(request); WinHttpCloseHandle(connect); WinHttpCloseHandle(session);
        return {};
    }

    std::string response;
    char buffer[4096];
    DWORD downloaded = 0;

    while (WinHttpReadData(request, buffer, sizeof(buffer), &downloaded) && downloaded > 0) {
        response.append(buffer, downloaded);
    }

    WinHttpCloseHandle(request);
    WinHttpCloseHandle(connect);
    WinHttpCloseHandle(session);

    if (response.empty()) return {};

    try {
        auto json = nlohmann::json::parse(response);
        return json["tag_name"].get<std::string>();
    } catch (...) {
        return {};
    }
}

bool Version::ParseVersion(std::string tag, Version_& version) {
    const char* ptr = tag.c_str();
    if (*ptr == 'v' || *ptr == 'V') ptr++;
    if (*ptr == '.') ptr++; 

    return std::sscanf(ptr, "%d.%d.%d", &version.major, &version.minor, &version.patch) == 3;
}

bool Version::GetLatestVersion(Version_& version) {
    std::string tag = Version::GetLatestTag();
    
    if (tag.empty()) {
        spdlog::error("Failed to fetch latest version tag from GitHub");
        return 0;
    }

    spdlog::info("Latest version tag on GitHub: {}", tag);
    
    return this->ParseVersion(tag, version);
}

bool Version::IsNewer(Version_ githubVersion, Version_ currentVersion) {
    if (githubVersion.major != currentVersion.major)
        return githubVersion.major > currentVersion.major;

    if (githubVersion.minor != currentVersion.minor)
        return githubVersion.minor > currentVersion.minor;

    return githubVersion.patch > currentVersion.patch;
}