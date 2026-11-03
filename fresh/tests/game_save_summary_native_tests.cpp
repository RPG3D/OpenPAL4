#include "pal4/native_game_save_summary.h"

#include <windows.h>

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

void WriteFixture(const std::filesystem::path& path,
                  std::int32_t marker_length, bool write_header,
                  std::string marker = "TEST") {
    std::ofstream file(path, std::ios::binary);
    file.write(reinterpret_cast<const char*>(&marker_length),
               sizeof(marker_length));
    file.write(marker.data(), static_cast<std::streamsize>(marker.size()));
    if (write_header) {
        pal4::GameSaveHeader header;
        header.game_key = 0x11223344u;
        header.arena_record_id = 7u;
        file.write(reinterpret_cast<const char*>(&header), sizeof(header));
    }
    Check(static_cast<bool>(file), "write save fixture");
}

}  // namespace

int main(int argc, char** argv) {
    const auto root = std::filesystem::temp_directory_path() /
        ("pal4_save_summary_" + std::to_string(GetCurrentProcessId()));
    const auto save_dir = root / "save";
    std::filesystem::create_directories(save_dir);
    pal4::NativeGameSaveSummaryReader reader;
    Check(!reader.ReadSlot(root, 3).has_value(), "missing save file");

    WriteFixture(save_dir / "3.dat", 4, true);
    const auto summary = reader.ReadSlot(root, 3);
    Check(summary && summary->marker == "TEST" &&
              summary->header.game_key == 0x11223344u &&
              summary->header.arena_record_id == 7u,
          "quick-load marker and 52-byte header");

    const std::string long_marker(512, 'A');
    WriteFixture(save_dir / "3.dat", 512, true, long_marker);
    const auto long_summary = reader.ReadSlot(root, 3);
    Check(long_summary && long_summary->marker == long_marker &&
              long_summary->header.game_key == 0x11223344u,
          "long archive marker branch");

    WriteFixture(save_dir / "3.dat", -1, true);
    Check(!reader.ReadSlot(root, 3).has_value() &&
              reader.LastError() == "invalid save marker length",
          "reject negative marker length");
    WriteFixture(save_dir / "3.dat", 4, false);
    Check(!reader.ReadSlot(root, 3).has_value() &&
              reader.LastError() == "invalid save file size",
          "reject truncated header");
    std::filesystem::remove_all(root);

    if (argc > 1) {
        const std::filesystem::path original(argv[1]);
        for (std::int32_t slot = 0; slot < 10; ++slot) {
            const auto loaded = reader.ReadSlot(original, slot);
            Check(loaded && loaded->header.version == 1u,
                  "original first-page save summary");
        }
        const auto first = reader.ReadSlot(original, 0);
        Check(first && first->marker.size() == 6u &&
                  first->header.game_key == 907u &&
                  first->header.arena_record_id == 1u,
              "original slot zero metadata");
    }
    std::cout << "PASS: native save summaries\n";
}
