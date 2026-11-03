#include "pal4/package_disc_file_api.h"

#include <cstdlib>
#include <iostream>
#include <string>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

class Win32Fixture final : public pal4::IPackageDiscWin32Api {
public:
    std::uint32_t AllocationGranularity() override {
        ++calls;
        return 65536u;
    }
    std::uint32_t CreateFile(const char* path, std::uint32_t access,
                             std::uint32_t share, std::uint32_t security,
                             std::uint32_t disposition,
                             std::uint32_t attributes,
                             std::uint32_t template_file) override {
        ++calls;
        last_path = path;
        last_access = access;
        last_share = share;
        last_security = security;
        last_disposition = disposition;
        last_attributes = attributes;
        last_template = template_file;
        return file_result;
    }
    std::uint32_t CreateMapping(std::uint32_t file,
                                std::uint32_t security,
                                std::uint32_t protection,
                                std::uint32_t maximum_high,
                                std::uint32_t maximum_low,
                                const char* name) override {
        ++calls;
        last_file = file;
        last_security = security;
        last_protection = protection;
        last_high = maximum_high;
        last_low = maximum_low;
        last_path = name ? name : "";
        return mapping_result;
    }
    std::uint32_t CloseHandle(std::uint32_t handle) override {
        ++calls;
        last_file = handle;
        return close_result;
    }
    std::uint32_t MapView(std::uint32_t mapping, std::uint32_t access,
                           std::uint32_t offset_high,
                           std::uint32_t offset_low,
                           std::uint32_t bytes) override {
        ++calls;
        last_file = mapping;
        last_access = access;
        last_high = offset_high;
        last_low = offset_low;
        last_bytes = bytes;
        return view_result;
    }
    std::uint32_t UnmapView(std::uint32_t address) override {
        ++calls;
        last_file = address;
        return unmap_result;
    }
    pal4::PackageDiscReadResult ReadFile(std::uint32_t file,
                                         void* destination,
                                         std::uint32_t bytes) override {
        ++calls;
        last_file = file;
        last_destination = destination;
        last_bytes = bytes;
        return read_result;
    }
    std::uint32_t SetFilePointer(std::uint32_t file,
                                 std::int32_t distance,
                                 std::uint32_t method) override {
        ++calls;
        last_file = file;
        last_distance = distance;
        last_method = method;
        return pointer_result;
    }
    std::uint32_t ReportShortRead() override {
        ++reports;
        return 0x12345678u;
    }

    int calls = 0;
    int reports = 0;
    std::string last_path;
    std::uint32_t last_file = 0;
    std::uint32_t last_access = 0;
    std::uint32_t last_share = 0;
    std::uint32_t last_security = 0;
    std::uint32_t last_disposition = 0;
    std::uint32_t last_attributes = 0;
    std::uint32_t last_template = 0;
    std::uint32_t last_protection = 0;
    std::uint32_t last_high = 0;
    std::uint32_t last_low = 0;
    std::uint32_t last_bytes = 0;
    std::int32_t last_distance = 0;
    std::uint32_t last_method = 0;
    void* last_destination = nullptr;
    std::uint32_t file_result = 42u;
    std::uint32_t mapping_result = 88u;
    std::uint32_t view_result = 0xabcd0000u;
    std::uint32_t close_result = 1u;
    std::uint32_t unmap_result = 1u;
    std::uint32_t pointer_result = 123u;
    pal4::PackageDiscReadResult read_result{1u, 4u};
};

}  // namespace

int main() {
    Win32Fixture win32;
    pal4::PackageDiscFileApi api(win32);
    Check(api.AllocationGranularity() == 65536u && win32.calls == 1,
          "GetSystemInfo allocation granularity forwarding");
    Check(api.CreateFile("game.cpk", 1u, 2u, 3u, 4u, 5u, 6u) == 42u &&
          win32.last_path == "game.cpk" && win32.last_access == 1u &&
          win32.last_share == 2u && win32.last_security == 3u &&
          win32.last_disposition == 4u && win32.last_attributes == 5u &&
          win32.last_template == 6u,
          "CreateFile forwards all seven arguments");
    Check(api.CreateMapping(42u, 2u, 3u, 99u, "map") == 88u &&
          win32.last_high == 0u && win32.last_low == 99u &&
          win32.last_path == "map", "mapping inserts zero high size");
    Check(api.MapView(88u, 4u, 65536u, 4096u) == 0xabcd0000u &&
          win32.last_high == 0u && win32.last_low == 65536u &&
          win32.last_bytes == 4096u, "view inserts zero high offset");
    Check(api.CloseFileHandle(42u) == 1u &&
          api.CloseMappingHandle(88u) == 1u &&
          api.UnmapView(0xabcd0000u) == 1u,
          "handle close and unmap forwarding");
    win32.close_result = 2u;
    win32.unmap_result = 2u;
    Check(api.CloseFileHandle(42u) == 2u &&
          api.CloseMappingHandle(88u) == 0u &&
          api.UnmapView(0xabcd0000u) == 0u,
          "first close returns raw BOOL, second close and unmap normalize");
    char destination[4]{};
    Check(api.LastReadStatusByte() == 0xffu &&
          api.ReadFile(42u, destination, 4u) == 1u &&
          api.LastReadStatusByte() == 1u && api.LastBytesRead() == 4u &&
          win32.last_destination == destination,
          "full read updates status and returns success");
    win32.read_result = {0u, 2u};
    Check(api.ReadFile(42u, destination, 4u) == 0x12345600u &&
          api.LastReadStatusByte() == 0u && api.LastBytesRead() == 2u &&
          win32.reports == 1, "short read reports and clears AL");
    win32.read_result = {0u, 4u};
    Check(api.ReadFile(42u, destination, 4u) == 0u &&
          win32.reports == 1, "failed read with full count does not report");
    Check(api.SetFilePointer(42u, -4) == 123u &&
          win32.last_distance == -4 && win32.last_method == 0u,
          "seek uses FILE_BEGIN and signed distance");
    win32.file_result = 0xffffffffu;
    Check(api.FileExists("absent") == 0xffffff00u,
          "failed existence open retains upper EAX bytes");
    win32.file_result = 42u;
    win32.close_result = 0x11223344u;
    Check(api.FileExists("present") == 0x11223301u &&
          win32.last_file == 42u && win32.last_access == 0x80000000u &&
          win32.last_share == 1u && win32.last_disposition == 3u &&
          win32.last_attributes == 0x10000080u,
          "existence uses fixed open flags and preserves close upper bytes");

    const int prior_calls = win32.calls;
    pal4::PackageDiscFileApi disabled(win32, 0u);
    Check(disabled.CreateFile("ignored", 0u, 0u, 0u, 0u, 0u, 0u) == 0u &&
          disabled.CreateMapping(0u, 0u, 0u, 0u, nullptr) == 0u &&
          disabled.CloseFileHandle(0u) == 0u &&
          disabled.MapView(0u, 0u, 0u, 0u) == 0u &&
          disabled.UnmapView(0u, 0xaabbccdd) == 0xaabbcc00u &&
          disabled.ReadFile(0u, nullptr, 0u, 0x11223344u) == 0x11223300u &&
          disabled.CloseMappingHandle(0u, 0x55667788u) == 0x55667700u &&
          disabled.SetFilePointer(0u, 0) == 0u &&
          disabled.FileExists("ignored", 0x99887766u) == 0x99887700u &&
          win32.calls == prior_calls,
          "disabled mode never calls Win32 and preserves incidental high bytes");
    std::cout << "13 package disc file API cases passed\n";
}
