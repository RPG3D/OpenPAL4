#include "pal4/player_manager_slot_snapshot.h"

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::PlayerManagerSlotSnapshotRuntimeStub {
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> events;
    std::string throw_at;
    std::uint32_t first_limit{10u};
    std::size_t limit_calls{};
    std::uint32_t property7{15u};
    bool pair_copied{};

    Runtime() {
        for (std::uint32_t i = 0u; i < 84u; ++i) {
            words[0x5658u + i * 4u] = 0x1000u + i;
            words[0x57a8u + i * 4u] = 0x2000u + i;
        }
        words[0x565cu] = first_limit;
        words[0x5890u] = property7;
        words[0x5894u] = 31u;
        words[0x5898u] = 27u;
        words[0x5918u] = 0xc1u;
        words[0x591cu] = 0xc2u;
    }
    void Event(const std::string& event) {
        events.push_back(event);
        if (event == throw_at) throw std::runtime_error(event);
    }
    std::uint32_t ReadDword(std::uint32_t address) override {
        if (address == 0x5658u) Event("copy-first-begin");
        if (address == 0x57a8u) Event("copy-second-begin");
        if (pair_copied && address == 0x5898u)
            Event("read-property9");
        if (pair_copied && address == 0x5894u)
            Event("read-property8");
        if (pair_copied && address == 0x5890u)
            Event("read-property7");
        return words[address];
    }
    void WriteDword(std::uint32_t address,
                    std::uint32_t value) override {
        if (address == 0x9000u) Event("write-first");
        words[address] = value;
        if (address == 0x914cu) Event("write-last");
    }
    void CopyTripleOwner(
        pal4::PlayerManagerScriptSnapshot& snapshot,
        std::uint32_t source) override {
        Check(source == 0x58f8u && snapshot.words[0] == 0x843270u &&
                  snapshot.words[1] == 0x1000u &&
                  snapshot.words[85] == 0x2000u,
              "first owner copy sees both initial records and base vtable");
        Event("copy-triple");
        snapshot.words[169] = 1u;
        snapshot.words[170] = 0xa000u;
        snapshot.words[171] = 0xa00cu;
        snapshot.words[172] = 0xa00cu;
    }
    void CopyPairOwner(
        pal4::PlayerManagerScriptSnapshot& snapshot,
        std::uint32_t source) override {
        Check(source == 0x5908u && snapshot.words[170] == 0xa000u,
              "pair copy follows triple copy");
        Event("copy-pair");
        pair_copied = true;
        snapshot.words[173] = 2u;
        snapshot.words[174] = 0xb000u;
        snapshot.words[175] = 0xb008u;
        snapshot.words[176] = 0xb008u;
    }
    void ProcessInventory(
        std::uint32_t entries,
        pal4::PlayerManagerScriptSnapshot& snapshot) override {
        Check(entries == 0x7777u &&
                  snapshot.words[0] == 0x8430e8u &&
                  snapshot.words[177] == 0xc1u &&
                  snapshot.words[178] == 0xc2u &&
                  snapshot.words[85] == 0x1000u &&
                  snapshot.words[1] == 0x1000u,
              "inventory sees cloned script and overwritten data record");
        Event("inventory");
        snapshot.words[85 + 15u] = 0xd15u;
    }
    void ProcessSkills(pal4::PlayerManagerScriptSnapshot& snapshot,
                       std::uint32_t mode) override {
        Check(mode == 1u && snapshot.words[170] == 0xa000u &&
                  snapshot.words[174] == 0xb000u,
              "skill batch receives both owned collections");
        Event("skills");
    }
    std::uint32_t GetElementByIndex(
        const pal4::PlayerManagerScriptSnapshot& snapshot,
        std::uint32_t index) override {
        Check(index == 0u && snapshot.words[86] == first_limit,
              "getter queries index zero of second data record");
        ++limit_calls;
        Event("limit");
        return snapshot.words[86];
    }
    void SetProperty(pal4::PlayerManagerScriptSnapshot& snapshot,
                     std::uint32_t index,
                     std::uint32_t value) override {
        if (index == 9u) {
            Check(value == 27u, "property nine saved before processing");
            Event("property9");
            snapshot.words[85 + 10u] = value;
        } else {
            Check(index == 8u && value == 31u,
                  "property eight saved before processing");
            Event("property8");
            snapshot.words[85 + 9u] = value;
        }
    }
    void Free(std::uint32_t pointer) override {
        Event(pointer == 0xb000u ? "free-pair" : "free-triple");
        Check(pointer == 0xb000u || pointer == 0xa000u,
              "cleanup frees the two cloned owner buffers");
    }
};

std::size_t At(const std::vector<std::string>& events,
               const std::string& wanted) {
    for (std::size_t i = 0u; i < events.size(); ++i)
        if (events[i] == wanted) return i;
    Check(false, "expected snapshot event missing");
    return events.size();
}

void SuccessfulSnapshotAndClamp() {
    Runtime runtime;
    Check(pal4::PlayerManagerSlotSnapshot::Build(
              0x5000u, 0x9000u, 0x7777u, runtime) == 0x9000u &&
              runtime.limit_calls == 2u,
          "snapshot returns destination and re-queries upper bound");
    for (std::uint32_t i = 0u; i < 84u; ++i) {
        const auto expected = i == 58u ? 10u :
            i == 10u ? 27u : i == 9u ? 31u :
            i == 15u ? 0xd15u : i == 1u ? 10u : 0x1000u + i;
        Check(runtime.words[0x9000u + i * 4u] == expected,
              "complete 0x150-byte output record matches processed snapshot");
    }
    const auto& e = runtime.events;
    Check(At(e, "copy-first-begin") < At(e, "copy-second-begin") &&
              At(e, "copy-second-begin") < At(e, "copy-triple") &&
              At(e, "copy-triple") < At(e, "copy-pair") &&
              At(e, "read-property9") < At(e, "read-property8") &&
              At(e, "read-property8") < At(e, "read-property7") &&
              At(e, "inventory") < At(e, "skills") &&
              At(e, "skills") < At(e, "property9") &&
              At(e, "property9") < At(e, "property8") &&
              At(e, "property8") < At(e, "write-first") &&
              At(e, "write-last") < At(e, "free-pair") &&
              At(e, "free-pair") < At(e, "free-triple"),
          "snapshot copies, processes, outputs and frees in IDA order");
}

void NegativeFloorAndInRange() {
    for (const auto value : {-2, 5}) {
        Runtime runtime;
        runtime.property7 = static_cast<std::uint32_t>(value);
        runtime.words[0x5890u] = runtime.property7;
        Check(pal4::PlayerManagerSlotSnapshot::Build(
                  0x5000u, 0x9000u, 0x7777u, runtime) == 0x9000u &&
                  runtime.limit_calls == 1u &&
                  runtime.words[0x9000u + 58u * 4u] ==
                      static_cast<std::uint32_t>(value < 0 ? 0 : value),
              "negative value floors to zero and in-range value stays");
    }
}

void ExceptionUnwind() {
    struct Case {
        const char* failure;
        std::vector<std::string> cleanup;
    };
    const std::vector<Case> cases{
        {"copy-triple", {}},
        {"copy-pair", {"free-triple"}},
        {"inventory", {"free-pair", "free-triple"}},
        {"skills", {"free-pair", "free-triple"}},
        {"property8", {"free-pair", "free-triple"}},
        {"write-first", {"free-pair", "free-triple"}},
    };
    for (const auto& scenario : cases) {
        Runtime runtime;
        runtime.throw_at = scenario.failure;
        try {
            static_cast<void>(pal4::PlayerManagerSlotSnapshot::Build(
                0x5000u, 0x9000u, 0x7777u, runtime));
            Check(false, "injected snapshot failure must propagate");
        } catch (const std::runtime_error&) {
            std::vector<std::string> cleanup;
            for (const auto& event : runtime.events)
                if (event == "free-pair" || event == "free-triple")
                    cleanup.push_back(event);
            Check(cleanup == scenario.cleanup,
                  "exception cleanup follows IDA unwind-map state");
        }
    }
}

}  // namespace

int main() {
    SuccessfulSnapshotAndClamp();
    NegativeFloorAndInRange();
    ExceptionUnwind();
    std::cout << "3 player-manager snapshot groups passed\n";
}
