#include "pal4/package_tea.h"
#include "pal4/package_archive_index.h"

#include <array>
#include <cstdlib>
#include <fstream>
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

std::uint32_t Word(const std::uint8_t* bytes) {
    return std::uint32_t(bytes[0]) | (std::uint32_t(bytes[1]) << 8u) |
           (std::uint32_t(bytes[2]) << 16u) |
           (std::uint32_t(bytes[3]) << 24u);
}

void CheckAsset(const char* path, const char* expected_directory) {
    std::ifstream file(path, std::ios::binary);
    Check(file.good(), "open original CPK asset");
    std::array<std::uint8_t, 128> header{};
    file.read(reinterpret_cast<char*>(header.data()), header.size());
    Check(file.gcount() == static_cast<std::streamsize>(header.size()) &&
          Word(header.data()) == 0x1a545352u, "CPK header magic");
    const std::uint32_t table_records = Word(header.data() + 16u);
    const std::uint32_t count = Word(header.data() + 32u);
    Check(table_records == 0x100000u / 32u &&
          count > 0u && count <= table_records,
          "CPK table and active index counts fit archive state");
    std::vector<std::uint8_t> table(0x100000u);
    file.read(reinterpret_cast<char*>(table.data()),
              static_cast<std::streamsize>(table_records * 32u));
    Check(file.gcount() == static_cast<std::streamsize>(table_records * 32u),
          "read complete CPK index");
    Check(pal4::PackageTea::DecryptArchiveTable(table),
          "decrypt one-megabyte package index");
    std::uint32_t active = 0;
    std::uint32_t sorted = 0;
    for (std::uint32_t i = 0; i < count; ++i) {
        const std::uint8_t* record = table.data() + i * 32u;
        active += (Word(record + 4u) & 1u) != 0u;
        if (i != 0u && Word(table.data() + (i - 1u) * 32u) <= Word(record))
            ++sorted;
    }
    Check(active == count && sorted == count - 1u,
          "decrypted CPK records are active and hash sorted");
    if (expected_directory != nullptr) {
        pal4::PackageArchiveState archive;
        archive.WriteWord(40u, count);
        for (std::uint32_t i = 0; i < count * 32u; ++i)
            archive.WriteByte(136u + i, table[i]);
        pal4::PackageNameCrc crc;
        crc.InitializeTable();
        Check(pal4::PackageArchiveIndex::FindByName(
                  archive, expected_directory, crc) >= 0,
              "decrypted index resolves expected directory");
    }
    std::cout << "CPK index: " << count << " records, " << active
              << " active, " << sorted << " sorted transitions; first="
              << std::hex << Word(table.data()) << "/"
              << Word(table.data() + 4u) << "/"
              << Word(table.data() + 8u) << std::dec << '\n';
}

}  // namespace

int main(int argc, char** argv) {
    const auto key = pal4::PackageTea::PackageKeyWords();
    Check(key == std::array<std::uint32_t, 4>{
          0x706d6156u, 0x2e657269u, 0x204a2e43u, 0x53207461u},
          "first sixteen package key bytes are little-endian words");
    std::array<std::uint32_t, 1> short_block{123u};
    Check(pal4::PackageTea::DecryptWords(short_block, key) == -1 &&
          short_block[0] == 123u, "one-word input is rejected without mutation");
    std::vector<std::uint8_t> short_table(8192u, 0xa5u);
    Check(!pal4::PackageTea::DecryptArchiveTable(short_table) &&
          short_table.front() == 0xa5u && short_table.back() == 0xa5u,
          "8 KiB archive table is unchanged");
    std::vector<std::uint8_t> first_valid(8193u, 0u);
    first_valid.back() = 0x5au;
    Check(pal4::PackageTea::DecryptArchiveTable(first_valid) &&
          first_valid.back() == 0x5au &&
          first_valid.front() != 0u && first_valid[4096u] != 0u,
          "two 4 KiB regions change while final byte is untouched");
    if (argc > 1) CheckAsset(argv[1], argc > 2 ? argv[2] : nullptr);
    std::cout << "4 package TEA boundary cases passed\n";
}
