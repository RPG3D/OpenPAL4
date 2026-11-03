#include "pal4/package_archive_file_entry.h"

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

class Runtime final : public pal4::IPackageFileBufferRuntime {
public:
    std::uint32_t Allocate(std::uint32_t bytes) override {
        events.push_back("alloc:" + std::to_string(bytes));
        return 0x3000u;
    }
    std::uint32_t Read(std::uint32_t handle, std::uint32_t data,
                       std::uint32_t bytes) override {
        events.push_back("read:" + std::to_string(handle) + ":" +
                         std::to_string(data) + ":" +
                         std::to_string(bytes));
        return 1u;
    }
    std::uint32_t Decrypt(std::uint32_t data, std::uint32_t words,
                          const char*) override {
        events.push_back("decrypt:" + std::to_string(data) + ":" +
                         std::to_string(words));
        return 1u;
    }
    std::uint32_t Free(std::uint32_t data) override {
        events.push_back("free:" + std::to_string(data));
        return 1u;
    }
    std::vector<std::string> events;
};

}  // namespace

int main() {
    pal4::PackageArchiveState archive;
    archive.WriteWord(1048748u, 0x2000u);
    pal4::PackageArchiveFileSlot slot;
    slot.index = 1u;
    slot.decoded_bytes = 11u;
    pal4::PackageFileBufferState buffer;
    Runtime runtime;
    pal4::PackageArchiveFileEntry::LoadToBuffer(
        archive, nullptr, &buffer, runtime);
    pal4::PackageArchiveFileEntry::LoadToBuffer(
        archive, &slot, nullptr, runtime);
    slot.index = 0xffffffffu;
    pal4::PackageArchiveFileEntry::LoadToBuffer(
        archive, &slot, &buffer, runtime);
    Check(runtime.events.empty() && buffer.loaded == 0u,
          "null inputs and invalid index have no side effects");
    slot.index = 1u;
    archive.WriteWord(136u + 32u + 28u, 1u);
    pal4::PackageArchiveFileEntry::LoadToBuffer(
        archive, &slot, &buffer, runtime);
    Check(buffer.loaded == 1u && buffer.bytes == 11u &&
          runtime.events == std::vector<std::string>({
              "alloc:11", "read:8192:12288:11", "decrypt:12288:2"}),
          "archive record +28 controls decryption and +36 slot size is used");
    runtime.events.clear();
    archive.WriteWord(136u + 32u + 28u, 0u);
    pal4::PackageArchiveFileEntry::LoadToBuffer(
        archive, &slot, &buffer, runtime);
    Check(runtime.events == std::vector<std::string>({
              "free:12288", "alloc:11", "read:8192:12288:11"}),
          "plain reload releases old buffer and skips decrypt");
    std::cout << "3 package archive load-buffer test cases passed\n";
}
