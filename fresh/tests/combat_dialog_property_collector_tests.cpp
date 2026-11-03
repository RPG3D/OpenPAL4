#include "pal4/combat_dialog_property_collector.h"

#include <array>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::ICombatDialogPropertyCollectionRuntime {
    std::vector<std::uint32_t> nodes;
    std::unordered_map<std::uint32_t, std::int32_t> slots;
    std::unordered_map<std::uint32_t, std::int32_t> second_slots;
    std::unordered_map<std::uint32_t, std::uint32_t> slot_reads;
    std::vector<std::string> calls;
    std::size_t position = 0u;
    bool fail_advance = false;

    std::uint32_t CreatePropertyIterator(
        std::uint32_t actor) override {
        Check(actor == 0x2000u, "property owner");
        calls.emplace_back("iterator");
        position = 0u;
        slot_reads.clear();
        return 0x3000u;
    }
    bool IteratorAtEnd(std::uint32_t iterator) override {
        Check(iterator == 0x3000u, "iterator end identity");
        calls.emplace_back("end");
        return position >= nodes.size();
    }
    std::uint32_t IteratorCurrent(
        std::uint32_t iterator) override {
        Check(iterator == 0x3000u && position < nodes.size(),
              "iterator current identity");
        calls.emplace_back("current");
        return nodes[position];
    }
    void IteratorAdvance(std::uint32_t iterator) override {
        Check(iterator == 0x3000u, "iterator advance identity");
        calls.emplace_back("advance");
        if (fail_advance) throw std::runtime_error("advance failed");
        ++position;
    }
    std::uint32_t DestroyIterator(
        std::uint32_t iterator) override {
        Check(iterator == 0x3000u, "iterator destruction identity");
        calls.emplace_back("destroy");
        return 0x77u;
    }
    std::int32_t PropertySlot(std::uint32_t property) override {
        calls.emplace_back("slot:" + std::to_string(property));
        const auto read = slot_reads[property]++;
        if (read != 0u && second_slots.contains(property))
            return second_slots[property];
        return slots[property];
    }
};

void Collection() {
    Runtime runtime;
    std::array<std::uint32_t, 6> properties{
        9u, 9u, 9u, 9u, 9u, 9u};
    Check(pal4::CombatDialogPropertyCollector::Collect(
              properties, 0x2000u, runtime) == 0x77u &&
              properties == std::array<std::uint32_t, 6>{} &&
              runtime.calls == std::vector<std::string>{
                  "iterator", "end", "destroy"},
          "empty collection clears six words and returns destructor value");

    runtime.calls.clear();
    runtime.nodes = {0x4000u, 0x5000u, 0x6000u};
    runtime.slots[0x4000u] = 2;
    runtime.slots[0x5000u] = 6;
    runtime.slots[0x6000u] = 2;
    Check(pal4::CombatDialogPropertyCollector::Collect(
              properties, 0x2000u, runtime) == 0x77u &&
              properties[2] == 0x6000u && properties[0] == 0u &&
              runtime.slot_reads[0x4000u] == 2u &&
              runtime.slot_reads[0x5000u] == 1u &&
              runtime.slot_reads[0x6000u] == 2u &&
              runtime.calls.back() == "destroy",
          "valid slots are read twice, slot six ignored, later duplicate wins");

    runtime.calls.clear();
    runtime.nodes = {0x4000u};
    runtime.second_slots[0x4000u] = 4;
    Check(pal4::CombatDialogPropertyCollector::Collect(
              properties, 0x2000u, runtime) == 0x77u &&
              properties[4] == 0x4000u && properties[2] == 0u,
          "second slot read selects actual write position");
}

void InvalidAndFailurePaths() {
    Runtime runtime;
    std::array<std::uint32_t, 6> properties{};
    runtime.nodes = {0x4000u};
    runtime.slots[0x4000u] = -1;
    bool rejected = false;
    try {
        (void)pal4::CombatDialogPropertyCollector::Collect(
            properties, 0x2000u, runtime);
    } catch (const std::out_of_range&) {
        rejected = true;
    }
    Check(rejected && runtime.calls.back() == "destroy",
          "negative source slot is rejected before unsafe indexing");

    runtime.calls.clear();
    runtime.slots[0x4000u] = 1;
    runtime.second_slots[0x4000u] = 6;
    rejected = false;
    try {
        (void)pal4::CombatDialogPropertyCollector::Collect(
            properties, 0x2000u, runtime);
    } catch (const std::out_of_range&) {
        rejected = true;
    }
    Check(rejected && runtime.calls.back() == "destroy",
          "changed out-of-range slot is rejected before unsafe write");

    runtime.calls.clear();
    runtime.second_slots.clear();
    runtime.fail_advance = true;
    bool failed = false;
    try {
        (void)pal4::CombatDialogPropertyCollector::Collect(
            properties, 0x2000u, runtime);
    } catch (const std::runtime_error&) {
        failed = true;
    }
    Check(failed && runtime.calls.back() == "destroy",
          "iterator is released when advancing fails");
}

}  // namespace

int main() {
    try {
        Collection();
        InvalidAndFailurePaths();
        std::cout << "combat dialog property collector tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
