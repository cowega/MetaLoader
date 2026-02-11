#pragma once

#include <filesystem>
#include <string>

namespace fs = std::filesystem;

namespace Utils {
    std::string GetCurrentProcessName();
    std::vector<fs::path> GetDirectories(const fs::path& path);
    void openModFolder();
    HWND GetMyWindow();

    namespace Hook {
        struct MSVCString {
            union { char buf[16]; char* ptr; };
            unsigned __int64 size;
            unsigned __int64 capacity;
        };
        
        #pragma pack(push, 1)
        struct OptionalResolveResult {
            unsigned char value[0x30];
            unsigned char has_value;
            unsigned char pad[7];
        };
        #pragma pack(pop)

        void InitGameAllocator();
        bool IsReadablePtr(const void* p, size_t bytes = 1);
        std::string TryReadMSVCString(void* obj);
        void* GetQword(const unsigned char* p, size_t off);
        void SetQword(unsigned char* p, size_t off, void* v);
        void SetU64(unsigned char* p, size_t off, unsigned long long v);
        fs::path CutRawGamePath(const fs::path& fullPath);
        void CollapseDvpl(std::string& s);
        bool IsDrivePatternAt(const std::string& s, size_t i);
        std::string FixCompressedPath(std::string s);
    }

    namespace UI {
        void HelpMarker(const char* desc);
        void Hint(const char* desc);
        bool ToggleButton(const char* unique_id, bool& state);
    }
}