#include "pal4/npc_manager_deleter.h"

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

struct Runtime final : pal4::NpcManagerDeleteRuntimeStub {
    std::vector<std::string> events;
    std::uint32_t candidate{0x7100u};
    std::int32_t comparison{};
    std::uint8_t count{};
    std::uint32_t CStringLength(std::uint32_t address) override {
        Check(address == 0x1234u, "name address");
        return 3u;
    }
    void SetErrorFlag(pal4::ModeStringBufferRecord& record,
                      std::uint8_t) override { record.buffer = {}; }
    bool AllocateText(pal4::ModeStringBufferRecord& record,
                      std::uint32_t length, std::uint8_t mode) override {
        Check(length == 3u && mode == 1u, "name allocation");
        record.buffer.data = 0x8001u;
        return true;
    }
    void CopyBytes(std::uint32_t destination,
                   std::uint32_t source, std::uint32_t length) override {
        Check(destination == 0x8001u && source == 0x1234u &&
              length == 3u, "name copy");
    }
    void WriteByte(std::uint32_t address,
                   std::uint8_t value) override {
        Check(address == 0x8004u && value == 0u,
              "trailing NUL");
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        Check(address == 0x8000u, "name reference header");
        return count;
    }
    void FreeTextHeader(std::uint32_t address) override {
        Check(address == 0x8000u, "freed reference header");
        events.emplace_back("free-name");
    }
    std::uint32_t SearchNodeAlternative(std::uint32_t tree,
        const pal4::ModeStringBufferRecord& record) override {
        Check(tree == 0x5028u && record.buffer.data == 0x8001u,
              "NPC name tree");
        events.emplace_back("search");
        return candidate;
    }
    std::int32_t CompareName(const pal4::ModeStringBufferRecord&,
                             std::uint32_t address) override {
        Check(address == 0x710cu, "candidate key");
        events.emplace_back("compare");
        return comparison;
    }
    std::uint32_t ReadDword(std::uint32_t address) override {
        if (address == 0x502cu) return 0x7000u;
        Check(address == 0x711cu, "node payload");
        events.emplace_back("read-npc");
        return 0x9000u;
    }
    void CleanupNpc(std::uint32_t npc) override {
        Check(npc == 0x9000u, "cleanup NPC");
        events.emplace_back("cleanup");
    }
    void DestroyNpc(std::uint32_t npc,
                    std::uint32_t mode) override {
        Check(npc == 0x9000u && mode == 1u,
              "deleting destructor");
        events.emplace_back("destroy");
    }
    void RemoveNode(std::uint32_t tree,
                    std::uint32_t node) override {
        Check(tree == 0x5028u && node == 0x7100u,
              "remove original tree node");
        events.emplace_back("remove");
    }
};

void DeleteCases() {
    Runtime found;
    Check(pal4::NpcManagerDeleter::DeleteByName(
              0x5000u, 0x1234u, found) &&
          found.events == std::vector<std::string>{
              "search", "compare", "read-npc", "cleanup",
              "destroy", "remove", "free-name"},
          "NPC cleanup precedes destructor and node removal");

    Runtime absent;
    absent.candidate = 0x7000u;
    Check(!pal4::NpcManagerDeleter::DeleteByName(
              0x5000u, 0x1234u, absent) &&
          absent.events == std::vector<std::string>{
              "search", "free-name"},
          "empty tree leaves NPCs untouched");

    Runtime negative;
    negative.comparison = -1;
    Check(!pal4::NpcManagerDeleter::DeleteByName(
              0x5000u, 0x1234u, negative) &&
          negative.events == std::vector<std::string>{
              "search", "compare", "free-name"},
          "negative key comparison rejects candidate");
}

}  // namespace

int main() {
    DeleteCases();
    std::cout << "NPC manager deleter cases passed\n";
}
