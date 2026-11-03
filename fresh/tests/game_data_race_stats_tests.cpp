#include "pal4/game_data_race_stats.h"

#include <bit>
#include <cstdint>
#include <iostream>
#include <stdexcept>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

void CheckResetAndWrapper() {
    pal4::GameDataRaceStats stats;
    stats.Data().fill(0xffffffffu);
    Check(&stats.Construct() == &stats && stats.Data()[0] == 7,
          "constructor wrapper returns the same 144-byte object");
    for (std::size_t i = 1; i < stats.Data().size(); ++i)
        Check(stats.Data()[i] == 0, "constructor clears remaining words");
    stats.Data()[35] = 42;
    Check(stats.Reset() == 0 && stats.Data()[35] == 0 &&
              stats.Data()[0] == 7,
          "reset returns zero and clears the final member");
}

void CheckAccumulate() {
    pal4::GameDataRaceStats destination;
    pal4::GameDataRaceStats source;
    (void)destination.Construct();
    (void)source.Construct();
    const auto setup_words = [&](std::size_t first, std::size_t count) {
        for (std::size_t i = first; i < first + count; ++i) {
            destination.Data()[i] = 0xffffffffu;
            source.Data()[i] = 2;
        }
    };
    const auto setup_floats = [&](std::size_t first, std::size_t count) {
        for (std::size_t i = first; i < first + count; ++i) {
            destination.Data()[i] = std::bit_cast<std::uint32_t>(1.25f);
            source.Data()[i] = std::bit_cast<std::uint32_t>(2.5f);
        }
    };
    setup_words(1, 8);
    setup_floats(9, 4);
    setup_words(13, 6);
    setup_floats(19, 6);
    setup_floats(25, 6);
    setup_words(31, 5);

    Check(destination.AddFrom(source, 0x4000, 0x4800) == 0x800 &&
              destination.Data()[0] == 7,
          "accumulator returns the original 32-bit address difference");
    const auto check_words = [&](std::size_t first, std::size_t count) {
        for (std::size_t i = first; i < first + count; ++i)
            Check(destination.Data()[i] == 1,
                  "integer section uses wrapping DWORD addition");
    };
    const auto check_floats = [&](std::size_t first, std::size_t count) {
        for (std::size_t i = first; i < first + count; ++i)
            Check(std::bit_cast<float>(destination.Data()[i]) == 3.75f,
                  "float section adds numeric values");
    };
    check_words(1, 8);
    check_floats(9, 4);
    check_words(13, 6);
    check_floats(19, 6);
    check_floats(25, 6);
    check_words(31, 5);
    Check(destination.AddFrom(source, 0x5000, 0x4000) == 0xfffff000u,
          "negative address delta wraps to the original DWORD return");
}

void CheckDescriptorHeader() {
    pal4::GameDataRaceDescriptorHeader header;
    header.Data().fill(0xdeadbeefu);
    Check(&header.Initialize(0x8ef7a8, 7) == &header &&
              header.Data()[0] == 0x8444ac &&
              header.Data()[1] == 0x8ef7a8 && header.Data()[2] == 7,
          "descriptor constructor writes the vtable, source and kind");
    for (std::size_t i = 3; i < header.Data().size(); ++i)
        Check(header.Data()[i] == 0xdeadbeefu,
              "descriptor constructor preserves later object fields");
}

}  // namespace

int main() {
    try {
        CheckResetAndWrapper();
        CheckAccumulate();
        CheckDescriptorHeader();
        std::cout << "game_data_race_stats_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
