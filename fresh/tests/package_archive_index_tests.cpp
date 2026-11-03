#include "pal4/package_archive_index.h"

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

void SetRecord(pal4::PackageArchiveState& archive, std::uint32_t index,
               std::uint32_t hash, std::uint32_t flags,
               std::uint32_t parent = 0) {
    const std::uint32_t base = 136u + index * 32u;
    archive.WriteWord(base, hash);
    archive.WriteWord(base + 4u, flags);
    archive.WriteWord(base + 8u, parent);
}

class Runtime final : public pal4::IPackageArchiveIndexRuntime {
public:
    void SetFilePointer(std::uint32_t file, std::uint32_t offset) override {
        last_file = file;
        offsets.push_back(offset);
    }
    void ReadFile(std::uint32_t file, void* destination,
                  std::uint32_t bytes) override {
        Check(file == last_file, "read uses archive handle");
        Check(read_count < names.size(), "expected file read");
        Check(bytes == names[read_count].size() + 1u, "record byte count");
        std::memcpy(destination, names[read_count].c_str(), bytes);
        ++read_count;
    }
    std::uint32_t ReportMissingDirectory(const char* path) override {
        missing = path;
        return 0x1234u;
    }
    std::uint32_t last_file = 0;
    std::vector<std::uint32_t> offsets;
    std::vector<std::string> names;
    std::size_t read_count = 0;
    std::string missing;
};

int Visit(const char* name, void* context) {
    auto& seen = *static_cast<std::vector<std::string>*>(context);
    seen.emplace_back(name);
    return name[0] == 's' ? 0 : 1;
}

}  // namespace

int main() {
    pal4::PackageNameCrc crc;
    crc.InitializeTable();
    pal4::PackageArchiveState archive;
    const auto hash = [&crc](const char* name) {
        return crc.Compute(reinterpret_cast<const std::uint8_t*>(name));
    };
    const std::uint32_t directory_hash = hash("fonts");
    const std::uint32_t low_hash = directory_hash - 1u;
    const std::uint32_t high_hash = directory_hash + 1u;
    archive.WriteWord(40u, 3u);
    SetRecord(archive, 0u, low_hash, 0u);
    SetRecord(archive, 1u, directory_hash, 1u);
    SetRecord(archive, 2u, high_hash, 0x11u);
    Check(pal4::PackageArchiveIndex::FindByName(archive, "FONTS", crc) == 1,
          "lowercase name and find active directory");
    Check(pal4::PackageArchiveIndex::FindByHash(archive, low_hash) == -1 &&
          pal4::PackageArchiveIndex::FindByHash(archive, high_hash) == -1,
          "inactive and excluded records do not match");
    Check(pal4::PackageArchiveIndex::FindByName(archive, "missing", crc) == -1,
          "missing name");

    Runtime runtime;
    Check(pal4::PackageArchiveIndex::EnumerateFiles(
              archive, "missing", crc, Visit, nullptr, runtime) == 0x1234u &&
          runtime.missing == "missing", "missing directory reporting");

    // The directory record is sorted by hash; child iteration uses table order.
    archive.WriteWord(40u, 5u);
    SetRecord(archive, 3u, 0xeeee0001u, 1u, directory_hash);
    SetRecord(archive, 4u, 0xeeee0002u, 1u, directory_hash);
    archive.WriteWord(136u + 3u * 32u + 12u, 100u);
    archive.WriteWord(136u + 3u * 32u + 16u, 7u);
    archive.WriteWord(136u + 3u * 32u + 24u, 7u);
    archive.WriteWord(136u + 4u * 32u + 12u, 200u);
    archive.WriteWord(136u + 4u * 32u + 16u, 9u);
    archive.WriteWord(136u + 4u * 32u + 24u, 7u);
    archive.WriteWord(1048748u, 0x9876u);
    runtime.names = {"a.font", "s.font"};
    std::vector<std::string> seen;
    Check(pal4::PackageArchiveIndex::EnumerateFiles(
              archive, "fonts", crc, Visit, &seen, runtime) == 0u &&
          seen == runtime.names && runtime.offsets ==
              std::vector<std::uint32_t>{107u, 209u} &&
          runtime.last_file == 0x9876u,
          "enumeration reads names and stops on callback zero");
    runtime.read_count = 0;
    Check(pal4::PackageArchiveIndex::EnumerateFiles(
              archive, "fonts", crc, nullptr, nullptr, runtime) == 5u,
          "without callback returns record count");
    std::cout << "4 package archive index test cases passed\n";
}
