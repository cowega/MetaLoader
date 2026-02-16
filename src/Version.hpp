#pragma once
#include <string>

class Version {
public:
    Version();
    ~Version();
    void Check();

private:
    struct Version_ {
        int major;
        int minor;
        int patch;
    };

private:
    std::string GetLatestTag();
    bool ParseVersion(std::string tag, Version_& version);
    bool GetLatestVersion(Version_& version);
    bool IsNewer(Version_ githubVersion, Version_ currentVersion);

};