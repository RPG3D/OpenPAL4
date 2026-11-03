#include "pal4/combat_conditional_actions.h"

#include <bit>
#include <cstdint>
#include <iostream>
#include <limits>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::ICombatConditionalActionsRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> calls;
    std::map<std::uint32_t, std::vector<std::uint32_t>> vectors;
    std::int32_t stat = 4;
    std::int32_t base = 10;
    std::int32_t counter = 3;
    std::uint32_t random = 5u;
    std::int32_t release_result = 91;
    bool throw_on_apply = false;
    bool throw_on_reset = false;
    std::uint32_t next_vector = 1u;

    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.push_back("read " + std::to_string(address));
        return words[address];
    }
    std::int32_t FloatToInt(float value) override {
        calls.push_back("convert " + std::to_string(value));
        return static_cast<std::int32_t>(value);
    }
    std::int32_t ActorStat(std::uint32_t actor, std::uint32_t slot) override {
        calls.push_back("stat " + std::to_string(actor) + " " +
                        std::to_string(slot));
        return stat;
    }
    std::int32_t ActorBase(std::uint32_t actor, std::uint32_t slot) override {
        calls.push_back("base " + std::to_string(actor) + " " +
                        std::to_string(slot));
        return base;
    }
    std::int32_t ActorCounter(std::uint32_t actor) override {
        calls.push_back("counter " + std::to_string(actor));
        return counter;
    }
    std::uint32_t RandomNumber() override {
        calls.push_back("random");
        return random;
    }
    void ApplySelected(std::uint32_t target, std::int32_t id) override {
        calls.push_back("apply " + std::to_string(target) + " " +
                        std::to_string(id));
        if (throw_on_apply) throw std::runtime_error("apply failed");
    }
    std::int32_t ReleaseIds(const std::vector<std::int32_t>& ids) override {
        std::string entry = "release";
        for (const auto id : ids) entry += " " + std::to_string(id);
        calls.push_back(entry);
        return release_result;
    }
    std::uint32_t EnumerateCandidates(std::uint32_t target,
                                       std::uint8_t selector) override {
        calls.push_back("enumerate " + std::to_string(target) + " " +
                        std::to_string(selector));
        const auto vector = next_vector++;
        const auto start = 8000u + vector * 100u;
        if (vector == 1u) {
            vectors[vector] = {11u, 22u, 33u};
        } else {
            vectors[vector] = {33u, 22u};
        }
        for (std::size_t i = 0; i < vectors[vector].size(); ++i)
            words[start + static_cast<std::uint32_t>(i) * 4u] = vectors[vector][i];
        return vector;
    }
    std::uint32_t VectorBegin(std::uint32_t vector) override {
        calls.push_back("begin " + std::to_string(vector));
        return 8000u + vector * 100u;
    }
    std::uint32_t VectorEnd(std::uint32_t vector) override {
        calls.push_back("end " + std::to_string(vector));
        return 8000u + vector * 100u +
               static_cast<std::uint32_t>(vectors[vector].size()) * 4u;
    }
    std::int32_t CandidateId(std::uint32_t candidate) override {
        calls.push_back("id " + std::to_string(candidate));
        return candidate == 11u || candidate == 33u ? 7 : 9;
    }
    void ResetCandidate(std::uint32_t candidate, std::uint32_t slot,
                        std::int32_t value) override {
        calls.push_back("reset " + std::to_string(candidate) + " " +
                        std::to_string(slot) + " " + std::to_string(value));
        if (throw_on_reset) throw std::runtime_error("reset failed");
    }
    void RefreshCandidate(std::uint32_t candidate) override {
        calls.push_back("refresh " + std::to_string(candidate));
    }
    void FreeVector(std::uint32_t vector) override {
        calls.push_back("free " + std::to_string(vector));
    }
};

void SetFloat(Runtime& runtime, std::uint32_t offset, float value) {
    runtime.words[1000u + offset] = std::bit_cast<std::uint32_t>(value);
}

void Prepare(Runtime& runtime, float mode, float threshold) {
    SetFloat(runtime, 20u, mode);
    SetFloat(runtime, 24u, threshold);
    const float ids[7]{7.9f, 0.0f, -2.8f, 9.0f, 0.0f, 7.0f, 0.0f};
    for (std::uint32_t i = 0u; i < 7u; ++i)
        SetFloat(runtime, 28u + i * 4u, ids[i]);
}

void Gates() {
    Runtime ratio_rejected;
    Prepare(ratio_rejected, 0.0f, 0.3f);
    Check(pal4::CombatConditionalActions::Select(
              1000u, 20u, 30u, ratio_rejected) == 0 &&
              ratio_rejected.calls == std::vector<std::string>{
                  "read 1020", "convert 0.000000", "stat 20 7",
                  "base 20 0", "read 1024"},
          "ratio greater than threshold rejects before collection work");

    Runtime equal;
    Prepare(equal, 0.0f, 0.4f);
    Check(pal4::CombatConditionalActions::Select(1000u, 20u, 30u, equal) ==
              91 && equal.calls.back() == "release 7 -2 9 7",
          "ratio equality enters selection");

    Runtime unordered;
    Prepare(unordered, 0.0f, 0.0f);
    unordered.base = 0;
    unordered.stat = 0;
    Check(pal4::CombatConditionalActions::Select(1000u, 20u, 30u,
                                                 unordered) == 91,
          "unordered ratio enters selection like x87 status mask");

    Runtime counter_rejected;
    Prepare(counter_rejected, 1.0f, 4.9f);
    Check(pal4::CombatConditionalActions::ResetMatching(
              1000u, 20u, 30u, counter_rejected) == 4 &&
              counter_rejected.calls == std::vector<std::string>{
                  "read 1020", "convert 1.000000", "counter 20",
                  "read 1024", "convert 4.900000"},
          "mode one rejects with converted threshold");

    Runtime other;
    Prepare(other, 2.0f, -1.0f);
    Check(pal4::CombatConditionalActions::ResetMatching(
              1000u, 20u, 30u, other) == 0 &&
              other.calls[2] == "read 1024" &&
              other.calls[3] == "counter 20",
          "other mode uses threshold nonzero flag and reads counter");

    Runtime divide_zero;
    Prepare(divide_zero, 2.0f, 0.0f);
    bool caught = false;
    try {
        static_cast<void>(pal4::CombatConditionalActions::Select(
            1000u, 20u, 30u, divide_zero));
    } catch (const std::domain_error&) {
        caught = true;
    }
    Check(caught && divide_zero.calls.back() == "counter 20",
          "zero threshold reaches native integer division fault");
}

void SelectionAndRelease() {
    Runtime selected;
    Prepare(selected, 1.0f, 3.0f);
    Check(pal4::CombatConditionalActions::Select(
              1000u, 20u, 30u, selected) == 91 &&
              selected.calls == std::vector<std::string>{
                  "read 1020", "convert 1.000000", "counter 20",
                  "read 1024", "convert 3.000000",
                  "read 1028", "convert 7.900000",
                  "read 1032", "convert 0.000000",
                  "read 1036", "convert -2.800000",
                  "read 1040", "convert 9.000000",
                  "read 1044", "convert 0.000000",
                  "read 1048", "convert 7.000000",
                  "read 1052", "convert 0.000000",
                  "random", "apply 30 -2", "release 7 -2 9 7"},
          "seven IDs preserve order and random selects modulo nonzero count");

    Runtime empty;
    Prepare(empty, 1.0f, 3.0f);
    for (std::uint32_t i = 0u; i < 7u; ++i)
        SetFloat(empty, 28u + i * 4u, 0.0f);
    bool caught_empty = false;
    try {
        static_cast<void>(pal4::CombatConditionalActions::Select(
            1000u, 20u, 30u, empty));
    } catch (const std::domain_error&) {
        caught_empty = true;
    }
    Check(caught_empty && empty.calls[19] == "random" &&
              empty.calls.back() == "release",
          "empty accepted list faults after random and releases storage");

    Runtime throws;
    Prepare(throws, 1.0f, 3.0f);
    throws.throw_on_apply = true;
    bool caught_apply = false;
    try {
        static_cast<void>(pal4::CombatConditionalActions::Select(
            1000u, 20u, 30u, throws));
    } catch (const std::runtime_error&) {
        caught_apply = true;
    }
    Check(caught_apply && throws.calls.back() == "release 7 -2 9 7",
          "selected ID collection releases after dispatch failure");
}

void ResetMatching() {
    Runtime runtime;
    Prepare(runtime, 1.0f, 3.0f);
    Check(pal4::CombatConditionalActions::ResetMatching(
              1000u, 20u, 30u, runtime) == 0 &&
              runtime.next_vector == 5u,
          "each nonzero ID creates one candidate collection");
    const auto& calls = runtime.calls;
    const auto find = [&](const std::string& entry) {
        for (std::size_t i = 0; i < calls.size(); ++i)
            if (calls[i] == entry) return i;
        return calls.size();
    };
    Check(find("enumerate 30 4") < find("reset 11 7 0") &&
              find("reset 11 7 0") < find("refresh 11") &&
              find("refresh 11") < find("reset 33 7 0") &&
              find("reset 33 7 0") < find("free 1") &&
              find("free 1") < find("enumerate 30 12") &&
              find("free 2") < find("enumerate 30 16") &&
              find("reset 22 7 0") < find("free 3") &&
              find("free 3") < find("enumerate 30 24") &&
              find("free 4") < find("read 1052") &&
              calls.back() == "convert 0.000000",
          "matching IDs reset and refresh in order with per-ID release");

    Runtime throws;
    Prepare(throws, 1.0f, 3.0f);
    throws.throw_on_reset = true;
    bool caught = false;
    try {
        static_cast<void>(pal4::CombatConditionalActions::ResetMatching(
            1000u, 20u, 30u, throws));
    } catch (const std::runtime_error&) {
        caught = true;
    }
    Check(caught && throws.calls.back() == "free 1",
          "candidate vector releases when reset throws");
}

}  // namespace

int main() {
    try {
        Gates();
        SelectionAndRelease();
        ResetMatching();
        std::cout << "combat conditional actions tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
