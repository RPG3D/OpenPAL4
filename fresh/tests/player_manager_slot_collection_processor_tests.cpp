#include "pal4/player_manager_slot_collection_processor.h"

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

struct Runtime final : pal4::PlayerManagerSlotCollectionRuntimeStub {
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> events;
    std::string throw_at;
    bool append_during_fade{true};
    bool append_initial_values{true};
    pal4::PlayerManagerResultArray* current{};

    void Event(const std::string& event) {
        events.push_back(event);
        if (event == throw_at) throw std::runtime_error(event);
    }
    std::uint32_t Allocate(std::uint32_t bytes) override {
        Check(bytes == 0x80u, "temporary result allocation has 32 slots");
        Event("allocate");
        return 0x9000u;
    }
    std::uint32_t Free(std::uint32_t pointer) override {
        Check(pointer == 0u || pointer == 0x9000u,
              "only old zero and active temporary buffer are freed");
        Event(pointer == 0u ? "free-zero" : "free-result");
        return pointer == 0u ? 0u : 0xfeedu;
    }
    std::uint32_t ReadDword(std::uint32_t address) override {
        events.push_back("read:" + std::to_string(address));
        return words[address];
    }
    void CheckArray(const pal4::PlayerManagerResultArray& result) {
        Check(result.incoming_byte == 0x5au &&
                  result.begin == 0x9000u &&
                  result.capacity_end == 0x9080u &&
                  result.end >= result.begin,
              "result array preserves incoming byte and allocation bounds");
    }
    void ProcessInventory(std::uint32_t slots,
                          pal4::PlayerManagerResultArray& result) override {
        Check(slots == 0x5920u, "inventory member address");
        CheckArray(result);
        Check(result.end == 0x9000u, "new result array starts empty");
        current = &result;
        Event("inventory");
        if (append_initial_values) {
            words[result.end] = 101u;
            result.end += 4u;
        }
    }
    void ProcessIndexedValues(
        std::uint32_t collection,
        pal4::PlayerManagerResultArray& result) override {
        Check(collection == 0x5938u, "indexed collection address");
        CheckArray(result);
        Event("indexed");
        if (append_initial_values) {
            words[result.end] = 102u;
            result.end += 4u;
        }
    }
    void ProcessFixedValues(
        std::uint32_t collection,
        pal4::PlayerManagerResultArray& result) override {
        Check(collection == 0x5938u, "fixed collection address");
        CheckArray(result);
        Event("fixed");
        if (append_initial_values) {
            words[result.end] = 103u;
            result.end += 4u;
        }
    }
    void HandleFade(std::uint32_t script, std::uint32_t value,
                    std::uint32_t marker) override {
        Check(script == 0x5654u && marker == 0x5654u,
              "fade script and non-null marker");
        Event("fade:" + std::to_string(value));
        if (append_during_fade && value == 101u) {
            words[current->end] = 104u;
            current->end += 4u;
        }
    }
    void FinalizeScript(std::uint32_t script) override {
        Check(script == 0x5654u, "script virtual +0x114 target");
        Event("finalize");
    }
};

void OrderedCollectionAndGrowingLoop() {
    Runtime runtime;
    Check(pal4::PlayerManagerSlotCollectionProcessor::Process(
              0x5000u, 0x5au, runtime) == 0xfeedu,
          "collector forwards final free return value");
    Check(runtime.events == std::vector<std::string>{
              "allocate", "free-zero", "inventory", "indexed", "fixed",
              "read:36864", "fade:101", "read:36868", "fade:102",
              "read:36872", "fade:103", "read:36876", "fade:104",
              "finalize", "free-result"},
          "collection stages precede dynamic fade traversal and final cleanup");
}

void EmptyResultStillFinalizes() {
    Runtime runtime;
    runtime.append_initial_values = false;
    Check(pal4::PlayerManagerSlotCollectionProcessor::Process(
              0x5000u, 0x5au, runtime) == 0xfeedu &&
              runtime.events == std::vector<std::string>{
                  "allocate", "free-zero", "inventory", "indexed",
                  "fixed", "finalize", "free-result"},
          "empty result skips fades but still finalizes and frees");
}

void ExceptionCleanup() {
    struct Case {
        const char* failure;
        std::vector<std::string> tail;
    };
    const std::vector<Case> cases{
        {"allocate", {"allocate", "free-zero"}},
        {"inventory", {"inventory", "free-result"}},
        {"indexed", {"indexed", "free-result"}},
        {"fixed", {"fixed", "free-result"}},
        {"fade:101", {"fade:101", "free-result"}},
        {"finalize", {"finalize", "free-result"}},
    };
    for (const auto& scenario : cases) {
        Runtime runtime;
        runtime.throw_at = scenario.failure;
        try {
            static_cast<void>(
                pal4::PlayerManagerSlotCollectionProcessor::Process(
                    0x5000u, 0x5au, runtime));
            Check(false, "injected collection failure must propagate");
        } catch (const std::runtime_error&) {
            Check(runtime.events.size() >= scenario.tail.size(),
                  "failure produced expected cleanup events");
            const auto begin = runtime.events.end() -
                static_cast<std::ptrdiff_t>(scenario.tail.size());
            Check(std::vector<std::string>(begin, runtime.events.end()) ==
                      scenario.tail,
                  "exception cleanup follows the active array pointer");
        }
    }
}

}  // namespace

int main() {
    OrderedCollectionAndGrowingLoop();
    EmptyResultStillFinalizes();
    ExceptionCleanup();
    std::cout << "3 player-manager collection groups passed\n";
}
