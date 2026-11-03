#include "pal4/package_archive_file_entry.h"

#include <array>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

class Runtime final : public pal4::IPackageArchiveReadRuntime {
public:
    void CopyFromAddress(void* destination, std::uint32_t source,
                         std::uint32_t bytes) override {
        events.push_back("copy:" + std::to_string(source) + ":" +
                         std::to_string(bytes));
        if (bytes != 0u) *static_cast<char*>(destination) = 'M';
    }
    void ReadFile(std::uint32_t file, void* destination,
                  std::uint32_t bytes) override {
        events.push_back("read:" + std::to_string(file) + ":" +
                         std::to_string(bytes));
        if (bytes != 0u) *static_cast<char*>(destination) = 'F';
    }
    std::vector<std::string> events;
};

}  // namespace

int main() {
    pal4::PackageArchiveState archive;
    pal4::PackageArchiveFileSlot slot;
    Runtime runtime;
    std::array<char, 4> destination{};
    archive.WriteWord(4u, 2u);
    slot.data = 0x9000u;
    slot.reserved40 = 0x1ffu;
    Check(pal4::PackageArchiveFileEntry::ReadData(
              archive, slot, destination.data(), 3u, runtime) == 0x201u &&
          slot.reserved40 == 0x202u && destination[0] == 'M' &&
          runtime.events == std::vector<std::string>({"copy:37375:3"}),
          "mapped read uses data plus cursor and returns cursor high bytes");
    archive.WriteWord(4u, 1u);
    archive.WriteWord(1048748u, 0x7000u);
    runtime.events.clear();
    Check(pal4::PackageArchiveFileEntry::ReadData(
              archive, slot, destination.data(), 5u, runtime) == 0x201u &&
          slot.reserved40 == 0x207u && destination[0] == 'F' &&
          runtime.events == std::vector<std::string>({"read:28672:5"}),
          "stream read ignores wrapper status and advances requested bytes");
    std::cout << "2 package archive read test cases passed\n";
}
