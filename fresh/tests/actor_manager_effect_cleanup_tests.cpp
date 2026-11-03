#include "pal4/actor_manager_effect_cleanup.h"

#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

constexpr std::uint32_t actor = 0x1000u;
constexpr std::uint32_t tree = actor + 836u;
constexpr std::uint32_t head = 0x4000u;
constexpr std::uint32_t first = 0x5000u;
constexpr std::uint32_t second = 0x5100u;
constexpr std::uint32_t nil = 0x6000u;

struct Runtime final : pal4::IActorManagerEffectCleanupRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint32_t> next;
    std::vector<std::string> trace;

    Runtime() {
        words[tree + 4u] = head;
        words[head] = head;
        words[head + 4u] = nil;
        words[head + 8u] = head;
        words[tree + 12u] = 0u;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words.at(address);
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
        trace.push_back("write:" + std::to_string(address) + ":" +
                        std::to_string(value));
    }
    std::uint32_t NilNode() override {
        trace.emplace_back("nil");
        return nil;
    }
    std::uint32_t AdvanceTreeCursor(std::uint32_t cursor) override {
        trace.push_back("next:" + std::to_string(cursor));
        return next.at(cursor);
    }
    void ResetDynamicEffect(std::uint32_t address) override {
        trace.push_back("reset:" + std::to_string(address));
    }
    void ClearTreeRange(std::uint32_t address,
                        std::uint32_t root) override {
        Check(address == tree && root == first,
              "bulk clear tree and root");
        trace.emplace_back("clear-tree");
    }
    void EraseTreeNode(std::uint32_t address,
                       std::uint32_t node) override {
        Check(address == tree && (node == first || node == second),
              "individual tree erase arguments");
        trace.push_back("erase:" + std::to_string(node));
    }
    void UpdateEffect(std::uint32_t address) override {
        Check(address == actor + 868u || address == actor + 876u ||
              address == actor + 884u,
              "fixed effect offset");
        trace.push_back("update:" + std::to_string(address - actor));
    }
    std::uint32_t InvokeActorSlot(std::uint32_t subject,
                                  std::uint32_t slot,
                                  std::uint32_t argument) override {
        Check(subject == actor &&
              ((slot == 576u && argument == 0u) ||
               (slot == 24u && argument == 6u)),
              "final virtual calls");
        trace.push_back("slot:" + std::to_string(slot));
        return slot == 24u ? 0x99u : 0x55u;
    }
};

void SetTwoNodes(Runtime& runtime) {
    runtime.words[head] = first;
    runtime.words[head + 4u] = first;
    runtime.next[first] = second;
    runtime.next[second] = head;
}

void EmptyTree() {
    Runtime runtime;
    Check(pal4::ActorManagerEffectCleanup::Release(actor, runtime) == 0x99u &&
          runtime.trace == std::vector<std::string>{
              "update:868", "update:876", "update:884",
              "write:4928:0", "slot:576", "slot:24"},
          "empty tree updates fixed handles then clears counter");
}

void BulkClear() {
    Runtime runtime;
    SetTwoNodes(runtime);
    runtime.words[tree + 12u] = 2u;
    Check(pal4::ActorManagerEffectCleanup::Release(actor, runtime) == 0x99u &&
          runtime.trace == std::vector<std::string>{
              "reset:20496", "next:20480", "reset:20752",
              "next:20736", "clear-tree", "nil", "write:16388:24576",
              "write:4944:0", "write:16384:16384",
              "write:16392:16384", "update:868", "update:876",
              "update:884", "write:4928:0", "slot:576", "slot:24"} &&
          runtime.words[tree + 12u] == 0u &&
          runtime.words[head] == head &&
          runtime.words[head + 4u] == nil &&
          runtime.words[head + 8u] == head,
          "nonempty tree resets dynamic effects before bulk clear");
}

void IndividualErase() {
    Runtime runtime;
    SetTwoNodes(runtime);
    Check(pal4::ActorManagerEffectCleanup::Release(actor, runtime) == 0x99u &&
          runtime.trace == std::vector<std::string>{
              "reset:20496", "next:20480", "reset:20752",
              "next:20736", "next:20480", "erase:20480",
              "next:20736", "erase:20736", "update:868",
              "update:876", "update:884", "write:4928:0",
              "slot:576", "slot:24"},
          "zero-count nonempty tree erases nodes after effect pass");
}

}  // namespace

int main() {
    try {
        EmptyTree();
        BulkClear();
        IndividualErase();
        std::cout << "actor_manager_effect_cleanup_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
