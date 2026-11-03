#include "pal4/package_archive.h"

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

class Runtime final : public pal4::IPackageArchiveLookupRuntime {
public:
    std::uint32_t FindFileIndex(const char* path) override {
        ++find_calls;
        last_path = path;
        return index;
    }
    std::uint32_t ReportNoPackageLoaded() override {
        ++error_calls;
        return 0xaabbccddu;
    }
    std::uint32_t index = 0u;
    std::string last_path;
    int find_calls = 0;
    int error_calls = 0;
};

}  // namespace

int main() {
    pal4::PackageArchiveState state;
    Runtime runtime;
    Check(pal4::PackageArchive::FileExistsRaw(state, "a.csb", runtime) ==
              0xaabbcc00u && runtime.error_calls == 1 &&
          runtime.find_calls == 0,
          "closed archive reports error and clears only AL");
    state.WriteByte(pal4::PackageArchiveState::kOpenFlag, 1u);
    runtime.index = 0x12345600u;
    Check(pal4::PackageArchive::FileExistsRaw(state, "a.csb", runtime) ==
              0x12345601u && runtime.last_path == "a.csb" &&
          runtime.find_calls == 1,
          "found index sets AL and keeps upper bytes");
    runtime.index = 0xffffffffu;
    Check(pal4::PackageArchive::FileExistsRaw(state, "missing.csb", runtime) ==
              0xffffff00u && runtime.find_calls == 2,
          "missing index clears AL and retains high bytes");
    std::cout << "3 package archive lookup test cases passed\n";
}
