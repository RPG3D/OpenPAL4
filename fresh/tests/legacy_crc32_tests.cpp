#include "pal4/legacy_crc32.h"

#include <array>
#include <cstdint>
#include <iostream>
#include <stdexcept>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

}  // namespace

int main() {
    try {
        pal4::LegacyCrc32 crc;
        Check(!crc.Initialized() && crc.LastResult() == 0,
              "CRC table initializes only on first call");
        constexpr std::array<std::uint8_t, 9> digits{
            '1', '2', '3', '4', '5', '6', '7', '8', '9'};
        Check(crc.ComputeAndStore(digits.data(), digits.size()) ==
                  0x340bc6d9u &&
                  crc.Initialized() &&
                  crc.LastResult() == 0x340bc6d9u,
              "CRC matches the reflected polynomial without final XOR");
        Check(crc.ComputeAndStore(nullptr, digits.size()) ==
                  0xffffffffu &&
                  crc.LastResult() == 0xffffffffu,
              "null input skips byte processing regardless of length");
        Check(crc.ComputeAndStore(digits.data(), 0) == 0xffffffffu &&
                  crc.ComputeAndStore(digits.data(), digits.size()) ==
                      0x340bc6d9u,
              "table is reused and zero-length input keeps the seed");

        pal4::LegacyCrc32 second_crc;
        constexpr std::array<std::uint8_t, 1> zero_byte{0};
        Check(!second_crc.Initialized() && second_crc.LastResult() == 0,
              "second IDA CRC routine has independent initial state");
        Check(second_crc.ComputeAndStore(zero_byte.data(), zero_byte.size()) ==
                  0x2dfd1072u &&
                  second_crc.Initialized() &&
                  second_crc.LastResult() == 0x2dfd1072u,
              "second CRC routine hashes binary data without final XOR");
        Check(crc.LastResult() == 0x340bc6d9u &&
                  second_crc.ComputeAndStore(nullptr, 0) == 0xffffffffu &&
                  crc.LastResult() == 0x340bc6d9u,
              "the two CRC global result slots remain independent");

        for (std::int32_t state : {-1, 0, 1, 3, 4}) {
            std::uint32_t row_hash = 0;
            const bool accepted = pal4::GameDataRowHashGate::Update(
                row_hash, digits.data(), digits.size(), state, crc);
            Check(accepted == (state >= 0 && state < 4) &&
                      row_hash == 0x340bc6d9u,
                  "row hash is written before the state gate");
        }
        std::uint32_t empty_hash = 0;
        Check(pal4::GameDataRowHashGate::Update(
                  empty_hash, nullptr, 100, 0, crc) &&
                  empty_hash == 0xffffffffu,
              "row gate accepts null data with a seeded hash");
        for (std::uint32_t state : {0u, 1u, 2u, 3u, 0xffffffffu})
            Check(pal4::GameDataRowHashGate::IsStateAtMostTwo(state) ==
                      (state <= 2u),
                  "shop state gate uses an unsigned upper bound");
        std::cout << "legacy_crc32_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
