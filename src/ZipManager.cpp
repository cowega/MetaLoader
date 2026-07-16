#include "ZipManager.hpp"
#include <filesystem>
#include <algorithm>
#include <cstring>
#include <spdlog/spdlog.h>

#define MINIZ_CPP_IMPLEMENTATION
#include "zip_file.hpp"

namespace fs = std::filesystem;

ZipManager::ZipManager() {
    m_archive = new mz_zip_archive();
    std::memset(m_archive, 0, sizeof(*m_archive));
}

ZipManager::~ZipManager() {
    close();
    delete m_archive;
}

ZipManager::ZipManager(ZipManager&& other) noexcept {
    m_archive = other.m_archive;
    m_is_open = other.m_is_open;
    other.m_archive = nullptr;
    other.m_is_open = false;
}

ZipManager& ZipManager::operator=(ZipManager&& other) noexcept {
    if (this != &other) {
        close();
        delete m_archive;
        m_archive = other.m_archive;
        m_is_open = other.m_is_open;
        other.m_archive = nullptr;
        other.m_is_open = false;
    }
    return *this;
}

bool ZipManager::openArchive(const std::string& filename) {
    close();
    if (!m_archive || !mz_zip_reader_init_file(m_archive, filename.c_str(), 0)) {
        return false;
    }
    m_is_open = true;
    return true;
}

void ZipManager::close() {
    if (m_is_open && m_archive) {
        if (m_archive->m_zip_mode == MZ_ZIP_MODE_WRITING) { 
            mz_zip_writer_finalize_archive(m_archive);
            mz_zip_writer_end(m_archive);
        } else if (m_archive->m_zip_mode == MZ_ZIP_MODE_READING) {
            mz_zip_reader_end(m_archive);
        }
        std::memset(m_archive, 0, sizeof(*m_archive));
        m_is_open = false;
    }
}

bool ZipManager::isOpen() const {
    return m_is_open;
}

std::string ZipManager::findDataPrefix() {
    if (!m_is_open || !m_archive) return "";

    mz_uint totalFiles = mz_zip_reader_get_num_files(m_archive);
    
    for (mz_uint i = 0; i < totalFiles; ++i) {
        mz_zip_archive_file_stat fileStat;
        if (!mz_zip_reader_file_stat(m_archive, i, &fileStat)) {
            continue;
        }

        std::string fullPath(fileStat.m_filename);
        
        size_t pos = 0;
        while (pos != std::string::npos) {
            size_t nextSlash = fullPath.find('/', pos);
            std::string segment = (nextSlash == std::string::npos) 
                                  ? fullPath.substr(pos) 
                                  : fullPath.substr(pos, nextSlash - pos);

            if (segment.rfind("Data", 0) == 0) {
                if (nextSlash == std::string::npos) {
                    return fullPath;
                } else {
                    return fullPath.substr(0, nextSlash + 1);
                }
            }

            if (nextSlash == std::string::npos) break;
            pos = nextSlash + 1;
        }
    }
    return "";
}

bool ZipManager::extractSmart(const std::string& archiveName) {
    fs::path rootPath("metaloader");
    fs::path fullArchivePath = archiveName;

    if (!fs::exists(fullArchivePath)) {
        fullArchivePath = rootPath / archiveName;
        if (fullArchivePath.extension() != ".zip") {
            fullArchivePath.replace_extension(".zip");
        }
    }

    if (!openArchive(fullArchivePath.string())) {
        spdlog::warn("Couldn't open the archive: {}", fullArchivePath.string());
        return false;
    }

    std::string folderName = fullArchivePath.stem().string();

    std::string dataPrefix = findDataPrefix();
    if (dataPrefix.empty()) {
        spdlog::warn("No folder starting with 'Data' was found in the archive.");
        close();
        return false;
    }

    std::string cutoffPrefix = "";
    size_t dataPos = dataPrefix.find("Data");
    if (dataPos != std::string::npos && dataPos > 0) {
        cutoffPrefix = dataPrefix.substr(0, dataPos);
    }

    fs::path targetDir = rootPath / folderName;

    mz_uint totalFiles = mz_zip_reader_get_num_files(m_archive);
    
    for (mz_uint i = 0; i < totalFiles; ++i) {
        mz_zip_archive_file_stat fileStat;
        if (!mz_zip_reader_file_stat(m_archive, i, &fileStat)) continue;

        std::string fileInArchive(fileStat.m_filename);

        if (!cutoffPrefix.empty()) {
            if (fileInArchive.rfind(cutoffPrefix, 0) != 0) {
                continue;
            }
            fileInArchive = fileInArchive.substr(cutoffPrefix.size());
        } else {
            if (fileInArchive.rfind("Data", 0) != 0 && fileInArchive.find("/Data") == std::string::npos) {
                continue;
            }
        }

        fs::path finalDiskPath = targetDir / fileInArchive;

        if (mz_zip_reader_is_file_a_directory(m_archive, i)) {
            fs::create_directories(finalDiskPath);
        } else {
            fs::create_directories(finalDiskPath.parent_path());

            if (!mz_zip_reader_extract_to_file(m_archive, i, finalDiskPath.string().c_str(), 0)) {
                spdlog::warn("Couldn't extract the file: {}", fileStat.m_filename);
            }
        }
    }

    close();
    return true;
}