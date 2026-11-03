#include "pal4/pal_event_volumn_manager.h"

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(bool good, const char* message) {
    if (!good) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IPalEventVolumnNameRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::string> names;
    std::vector<std::string> calls;
    std::string query_name;
    std::uint32_t next_head = 0x8000u;

    void InitializeEmptyString(pal4::SceneEvfRecordState&,
                               std::uint32_t offset) override {
        calls.push_back("init-string:" + std::to_string(offset));
    }
    std::uint32_t Allocate(std::uint32_t bytes) override {
        Check(bytes == 12u, "temporary EVF allocates 12-byte list head");
        const auto result = next_head;
        next_head += 0x100u;
        return result;
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
    }
    std::uint32_t SetString(pal4::SceneEvfRecordState&,
                            std::uint32_t offset,
                            std::string_view value) override {
        Check(offset == 104u, "query name uses EVF name field");
        query_name = value;
        calls.push_back("set-name");
        return 0u;
    }
    std::uint32_t ReadListWord(std::uint32_t address) override {
        return words.at(address);
    }
    void WriteListWord(std::uint32_t address,
                       std::uint32_t value) override {
        words[address] = value;
    }
    void FreeListNode(std::uint32_t) override {
        Check(false, "temporary list is empty");
    }
    void ClearBufferRange(std::uint32_t begin,
                          std::uint32_t end) override {
        Check(begin == 0u && end == 0u, "temporary property range empty");
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words.at(address);
    }
    pal4::EffectPropertyState& PropertyAt(std::uint32_t) override {
        Check(false, "temporary property array empty");
        static pal4::EffectPropertyState unused;
        return unused;
    }
    void Free(std::uint32_t address) override {
        calls.push_back("free:" + std::to_string(address));
    }
    void DestroyFrame(std::uint32_t, std::uint32_t) override {
        Check(false, "temporary frame absent");
    }
    std::uint32_t ClumpWorld(std::uint32_t) override { return 0u; }
    void RemoveClump(std::uint32_t, std::uint32_t) override {}
    void DestroyClump(std::uint32_t) override {}
    void RemoveListElement(std::uint32_t,
                           std::uint32_t) override {}
    int CleanupString(pal4::SceneEvfRecordState&,
                      std::uint32_t offset) override {
        calls.push_back("cleanup-string:" + std::to_string(offset));
        return 0;
    }
    std::uint32_t ReadRecordPointer(std::uint32_t slot) override {
        calls.push_back("slot:" + std::to_string(slot));
        return words.at(slot);
    }
    bool CompareRecordName(std::uint32_t record,
                           std::string_view name) override {
        Check(name == query_name, "temporary name is used for comparison");
        calls.push_back("compare:" + std::to_string(record));
        return names.at(record) == name;
    }
};

}  // namespace

int main() {
    pal4::PalEventVolumnState state;
    state.words[1u] = 0x4000u;
    state.words[2u] = 0x400cu;
    state.words[4u] = 0xdeadu;
    Runtime runtime;
    runtime.words[0x4000u] = 0x5000u;
    runtime.words[0x4004u] = 0x6000u;
    runtime.words[0x4008u] = 0x7000u;
    runtime.names = {{0x5000u, "first"},
                     {0x6000u, "target"},
                     {0x7000u, "last"}};
    Check(pal4::PalEventVolumnManager::FindByName(
              state, 0x3000u, 0x9000u, "target", runtime) == 0x6000u &&
              state.words[4u] == 0x6000u &&
              runtime.query_name == "target" &&
              runtime.calls[5u] == "set-name" &&
              runtime.calls[6u] == "slot:16384" &&
              runtime.calls[7u] == "compare:20480" &&
              runtime.calls[8u] == "slot:16388" &&
              runtime.calls[9u] == "compare:24576" &&
              runtime.calls.back() == "cleanup-string:8",
          "first matching record selected and temporary record cleaned");

    runtime.calls.clear();
    Check(pal4::PalEventVolumnManager::FindByName(
              state, 0x3000u, 0x9000u, "missing", runtime) == 0u &&
              state.words[4u] == 0x6000u &&
              runtime.calls[10u] == "slot:16392" &&
              runtime.calls[11u] == "compare:28672",
          "missing name scans all records and preserves selection");

    state.words[2u] = state.words[1u];
    runtime.calls.clear();
    Check(pal4::PalEventVolumnManager::FindByName(
              state, 0x3000u, 0x9000u, "target", runtime) == 0u &&
              state.words[4u] == 0x6000u &&
              runtime.calls[6u] == "free:33280",
          "empty manager still constructs and cleans temporary EVF record");
}
