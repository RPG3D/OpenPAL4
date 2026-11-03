#include "pal4/game_arena_record_compactor.h"

#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <utility>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Memory final : pal4::IGameArenaRecordMemory {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::uint32_t> reads;
    std::vector<std::pair<std::uint32_t, std::uint32_t>> writes;

    std::uint32_t ReadWord(std::uint32_t address) override {
        reads.push_back(address);
        return words[address];
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        writes.emplace_back(address, value);
        words[address] = value;
    }
};

void EmptySuffix() {
    Memory memory;
    memory.words[0x5008u] = 0x6024u;
    Check(pal4::GameArenaRecordCompactor::Compact(
              0x5000u, 0x6000u, 0x6024u, memory) == 0x6000u &&
              memory.reads == std::vector<std::uint32_t>{0x5008u} &&
              memory.writes ==
                  (std::vector<std::pair<std::uint32_t, std::uint32_t>>{
                      {0x5008u, 0x6000u}}),
          "empty suffix collapses range end to destination");
}

void TwoRecordShift() {
    Memory memory;
    memory.words = {{0x5008u, 0x6024u},
                    {0x600cu, 11u}, {0x6010u, 12u}, {0x6014u, 13u},
                    {0x6018u, 21u}, {0x601cu, 22u}, {0x6020u, 23u}};
    Check(pal4::GameArenaRecordCompactor::Compact(
              0x5000u, 0x6000u, 0x600cu, memory) == 0x6000u &&
              memory.writes ==
                  (std::vector<std::pair<std::uint32_t, std::uint32_t>>{
                      {0x6000u, 11u}, {0x6004u, 12u}, {0x6008u, 13u},
                      {0x600cu, 21u}, {0x6010u, 22u}, {0x6014u, 23u},
                      {0x5008u, 0x6018u}}),
          "two 12-byte records shift forward by one record");
    Check(memory.words[0x6000u] == 11u &&
              memory.words[0x600cu] == 21u &&
              memory.words[0x5008u] == 0x6018u,
          "record contents and new end reflect in-place compaction");
}

void InvalidRange() {
    Memory memory;
    memory.words[0x5008u] = 0x6024u;
    for (const auto source : {0x6028u, 0x6010u}) {
        bool rejected = false;
        try {
            static_cast<void>(pal4::GameArenaRecordCompactor::Compact(
                0x5000u, 0x6000u, source, memory));
        } catch (const std::invalid_argument&) {
            rejected = true;
        }
        Check(rejected, "malformed range rejected before writes");
    }
    Check(memory.writes.empty(), "invalid range does not mutate memory");

    pal4::GameArenaRecordMemoryStub stub;
    Check(pal4::GameArenaRecordCompactor::Compact(
              0u, 0u, 0u, stub) == 0u,
          "native memory stub links for empty range");
}

}  // namespace

int main() {
    try {
        EmptySuffix();
        TwoRecordShift();
        InvalidRange();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
