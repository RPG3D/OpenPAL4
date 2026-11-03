#include "pal4/npc_manager_finder.h"

#include <cstdlib>
#include <iostream>
#include <unordered_map>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::MonsterManagerFindRuntimeStub {
    std::uint32_t candidate{0x7100u};
    std::int32_t comparison{};
    bool allocate{true};
    std::unordered_map<std::uint32_t, std::uint8_t> headers;
    std::vector<std::uint32_t> frees;
    std::uint32_t CStringLength(std::uint32_t address) override {
        Check(address == 0x1234u, "name address");
        return 3u;
    }
    void SetErrorFlag(pal4::ModeStringBufferRecord& record,
                      std::uint8_t) override { record.buffer = {}; }
    bool AllocateText(pal4::ModeStringBufferRecord& record,
                      std::uint32_t length, std::uint8_t mode) override {
        Check(length == 3u && mode == 1u, "text allocation");
        if (allocate) {
            record.buffer.data = 0x8001u;
            headers[0x8000u] = 0u;
        }
        return allocate;
    }
    void CopyBytes(std::uint32_t destination,
                   std::uint32_t source,
                   std::uint32_t length) override {
        Check(destination == 0x8001u && source == 0x1234u &&
              length == 3u, "text copy");
    }
    void WriteByte(std::uint32_t address,
                   std::uint8_t value) override {
        if (address == 0x8004u) Check(value == 0u, "trailing NUL");
        else headers[address] = value;
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        return headers.at(address);
    }
    void FreeTextHeader(std::uint32_t address) override {
        frees.push_back(address);
    }
    std::uint32_t SearchNodeAlternative(std::uint32_t tree,
        const pal4::ModeStringBufferRecord& text) override {
        Check(tree == 0x5028u &&
              text.buffer.data == (allocate ? 0x8001u : 0u),
              "NPC tree search");
        return candidate;
    }
    std::int32_t CompareName(const pal4::ModeStringBufferRecord& text,
                             std::uint32_t address) override {
        Check(text.buffer.length == (allocate ? 3u : 0u) &&
              address == 0x710cu, "candidate key compare");
        return comparison;
    }
    std::uint32_t ReadDword(std::uint32_t address) override {
        if (address == 0x502cu) return 0x7000u;
        Check(address == 0x711cu, "NPC payload offset");
        return 0x9000u;
    }
};

void LookupCases() {
    Runtime runtime;
    Check(pal4::NpcManagerFinder::FindByName(
              0x5000u, 0x1234u, runtime) == 0x9000u &&
          runtime.frees == std::vector<std::uint32_t>{0x8000u},
          "matching NPC returns payload and frees name");

    Runtime negative;
    negative.comparison = -1;
    Check(pal4::NpcManagerFinder::FindByName(
              0x5000u, 0x1234u, negative) == 0u,
          "negative key comparison rejects candidate");

    Runtime empty;
    empty.candidate = 0x7000u;
    Check(pal4::NpcManagerFinder::FindByName(
              0x5000u, 0x1234u, empty) == 0u,
          "tree header is not an NPC");

    Runtime no_buffer;
    no_buffer.allocate = false;
    Check(pal4::NpcManagerFinder::FindByName(
              0x5000u, 0x1234u, no_buffer) == 0x9000u &&
          no_buffer.frees.empty(),
          "failed text allocation retains observed search path");
}

}  // namespace

int main() {
    LookupCases();
    std::cout << "NPC manager finder cases passed\n";
}
