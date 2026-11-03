#include "pal4/combat_random_selection.h"

#include <bit>
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

struct Runtime final : pal4::ICombatRandomSelectionRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> calls;
    std::vector<std::int32_t> released;
    std::uint32_t randoms[2]{0u, 5u};
    std::uint32_t random_index = 0u;
    bool throw_on_apply = false;

    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.push_back("read " + std::to_string(address));
        return words[address];
    }
    std::uint32_t RandomNumber() override {
        calls.push_back("random");
        return randoms[random_index++];
    }
    std::int32_t FloatToInt(float value) override {
        calls.push_back("convert " + std::to_string(value));
        return static_cast<std::int32_t>(value);
    }
    void ApplySelected(std::uint32_t target,
                       std::int32_t selected,
                       std::uint32_t context) override {
        calls.push_back("apply " + std::to_string(target) + " " +
                        std::to_string(selected) + " " +
                        std::to_string(context));
        if (throw_on_apply) throw std::runtime_error("apply failed");
    }
    void ApplySelectedActorEvent(std::uint32_t target,
                                 std::int32_t selected,
                                 std::uint32_t context) override {
        calls.push_back("apply-actor " + std::to_string(target) + " " +
                        std::to_string(selected) + " " +
                        std::to_string(context));
        if (throw_on_apply) throw std::runtime_error("apply failed");
    }
    std::int32_t ReleaseCollection(
        const std::vector<std::int32_t>& values) override {
        released = values;
        calls.push_back("release");
        return 81;
    }
};

void SetFloat(Runtime& runtime, std::uint32_t offset, float value) {
    runtime.words[1000u + offset] = std::bit_cast<std::uint32_t>(value);
}

void Prepare(Runtime& runtime) {
    SetFloat(runtime, 20u, 0.0f);
    const float ids[8]{3.9f, 0.0f, -2.9f, 5.0f,
                       0.0f, 1.0f, 0.0f, 0.0f};
    for (std::uint32_t i = 0u; i < 8u; ++i)
        SetFloat(runtime, 24u + i * 4u, ids[i]);
}

void GateAndSelection() {
    Runtime rejected;
    Prepare(rejected);
    rejected.randoms[0] = 1u;
    Check(pal4::CombatRandomSelection::Process(
              1000u, 3000u, 4000u, rejected) == 0 &&
              rejected.calls == std::vector<std::string>{
                  "random", "read 1020"} && rejected.released.empty(),
          "failed gate skips collection construction");

    Runtime runtime;
    Prepare(runtime);
    Check(pal4::CombatRandomSelection::Process(
              1000u, 3000u, 4000u, runtime) == 81 &&
              runtime.released ==
                  std::vector<std::int32_t>{3, -2, 5, 1} &&
              runtime.calls == std::vector<std::string>{
                  "random", "read 1020",
                  "read 1024", "convert 3.900000",
                  "read 1028", "convert 0.000000",
                  "read 1032", "convert -2.900000",
                  "read 1036", "convert 5.000000",
                  "read 1040", "convert 0.000000",
                  "read 1044", "convert 1.000000",
                  "read 1048", "convert 0.000000",
                  "read 1052", "convert 0.000000",
                  "random", "apply 4000 -2 3000", "release"},
          "eight ids preserve order and second random selects modulo count");
}

void EmptyAndExceptionalCleanup() {
    Runtime empty;
    SetFloat(empty, 20u, 0.0f);
    bool caught_empty = false;
    try {
        static_cast<void>(pal4::CombatRandomSelection::Process(
            1000u, 3000u, 4000u, empty));
    } catch (const std::domain_error&) {
        caught_empty = true;
    }
    Check(caught_empty && empty.random_index == 2u &&
              empty.calls.back() == "release" && empty.released.empty(),
          "empty accepted collection mirrors native divide-by-zero as exception");

    Runtime throws;
    Prepare(throws);
    throws.throw_on_apply = true;
    bool caught_apply = false;
    try {
        static_cast<void>(pal4::CombatRandomSelection::Process(
            1000u, 3000u, 4000u, throws));
    } catch (const std::runtime_error&) {
        caught_apply = true;
    }
    Check(caught_apply && throws.calls.back() == "release" &&
              throws.released ==
                  std::vector<std::int32_t>{3, -2, 5, 1},
          "selection collection is released after dispatch failure");
}

void ActorEventSelection() {
    Runtime rejected;
    Prepare(rejected);
    rejected.randoms[0] = 1u;
    Check(pal4::CombatRandomSelection::ProcessActorEvent(
              1000u, 4000u, 3000u, rejected) == 0 &&
              rejected.calls == std::vector<std::string>{
                  "random", "read 1020"},
          "actor event rejects failed random gate");

    Runtime selected;
    Prepare(selected);
    Check(pal4::CombatRandomSelection::ProcessActorEvent(
              1000u, 4000u, 3000u, selected) == 81 &&
              selected.released ==
                  std::vector<std::int32_t>{3, -2, 5, 1} &&
              selected.calls[selected.calls.size() - 2u] ==
                  "apply-actor 4000 -2 3000" &&
              selected.calls.back() == "release",
          "actor event uses virtual +0x108 after ordered selection");

    Runtime empty;
    SetFloat(empty, 20u, 0.0f);
    bool caught_empty = false;
    try {
        static_cast<void>(pal4::CombatRandomSelection::ProcessActorEvent(
            1000u, 4000u, 3000u, empty));
    } catch (const std::domain_error&) {
        caught_empty = true;
    }
    Check(caught_empty && empty.random_index == 2u &&
              empty.calls.back() == "release",
          "actor event empty collection faults after second random");

    Runtime throws;
    Prepare(throws);
    throws.throw_on_apply = true;
    bool caught_apply = false;
    try {
        static_cast<void>(pal4::CombatRandomSelection::ProcessActorEvent(
            1000u, 4000u, 3000u, throws));
    } catch (const std::runtime_error&) {
        caught_apply = true;
    }
    Check(caught_apply && throws.calls.back() == "release",
          "actor event releases temporary collection after dispatch failure");
}

}  // namespace

int main() {
    try {
        GateAndSelection();
        EmptyAndExceptionalCleanup();
        ActorEventSelection();
        std::cout << "combat random selection tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
