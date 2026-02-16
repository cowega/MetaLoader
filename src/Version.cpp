#pragma comment(lib, "winhttp.lib")

#include <windows.h>
#include <winhttp.h>
#include <cstdio>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

#include "Version.hpp"
#include "Loader.hpp"

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
            GetActiveWindow(),
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
    HINTERNET session = WinHttpOpen(L"metaloader/1.0",
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS, 0);

    if (!session) return {};

    HINTERNET connect = WinHttpConnect(session,
        L"api.github.com",
        INTERNET_DEFAULT_HTTPS_PORT, 0);

    if (!connect) return {};

    HINTERNET request = WinHttpOpenRequest(connect,
        L"GET",
        L"/repos/cowega/metaloader/releases/latest",
        nullptr,
        WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES,
        WINHTTP_FLAG_SECURE);

    if (!request) return {};

    WinHttpAddRequestHeaders(request,
        L"User-Agent: metaloader\r\nAccept: application/vnd.github+json\r\n",
        -1, WINHTTP_ADDREQ_FLAG_ADD);

    if (!WinHttpSendRequest(request, 0, 0, 0, 0, 0, 0))
        return {};

    if (!WinHttpReceiveResponse(request, nullptr))
        return {};

    std::string response;
    DWORD size = 0;

    do {
        WinHttpQueryDataAvailable(request, &size);
        if (!size) break;

        std::vector<char> buffer(size);
        DWORD downloaded = 0;

        WinHttpReadData(request, buffer.data(), size, &downloaded);
        response.append(buffer.data(), downloaded);

    } while (size > 0);

    WinHttpCloseHandle(request);
    WinHttpCloseHandle(connect);
    WinHttpCloseHandle(session);

    try {
        auto json = nlohmann::json::parse(response);
        return json["tag_name"].get<std::string>();
    } catch (...) {
        return {};
    }
}

bool Version::ParseVersion(std::string tag, Version_& version) {
    return std::sscanf(tag.c_str(), "v%d.%d.%d", &version.major, &version.minor, &version.patch) == 3;
}

bool Version::GetLatestVersion(Version_& version) {
    std::string tag = Version::GetLatestTag();
    
    if (tag.empty())
        return 0;
    
    return this->ParseVersion(tag, version);
}

bool Version::IsNewer(Version_ githubVersion, Version_ currentVersion) {
    if (githubVersion.major != currentVersion.major)
        return githubVersion.major > currentVersion.major;

    if (githubVersion.minor != currentVersion.minor)
        return githubVersion.minor > currentVersion.minor;

    return githubVersion.patch > currentVersion.patch;
}