#pragma once

#include <string>
#include <vector>

struct mz_zip_archive_tag;
typedef struct mz_zip_archive_tag mz_zip_archive;

class ZipManager {
public:
    ZipManager();
    ~ZipManager();

    ZipManager(const ZipManager&) = delete;
    ZipManager& operator=(const ZipManager&) = delete;

    ZipManager(ZipManager&& other) noexcept;
    ZipManager& operator=(ZipManager&& other) noexcept;

    bool openArchive(const std::string& filename);
    void close();
    bool isOpen() const;

    bool extractSmart(const std::string& archiveName);

private:
    mz_zip_archive* m_archive = nullptr;
    bool m_is_open = false;

    std::string findDataPrefix();

};