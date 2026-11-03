#include "pal4/combat_event_outcome.h"

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

struct Runtime final : pal4::ICombatEventOutcomeRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> calls;
    std::vector<std::uint32_t> randoms{0u, 0u};
    std::uint32_t random_index = 0u;
    bool matches[2]{true, true};
    bool throw_on_candidate = false;
    std::uint32_t enumeration_count = 0u;

    std::uint32_t RandomNumber() override {
        calls.push_back("random");
        return randoms.at(random_index++);
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.push_back("read " + std::to_string(address));
        return words[address];
    }
    std::int32_t FloatToInt(float value) override {
        calls.push_back("convert " + std::to_string(value));
        return static_cast<std::int32_t>(value);
    }
    void SetEventData(std::uint32_t slot, std::uint32_t target,
                      std::int32_t first, float scale,
                      std::int32_t second) override {
        calls.push_back("set " + std::to_string(slot) + " " +
                        std::to_string(target) + " " +
                        std::to_string(first) + " " +
                        std::to_string(scale) + " " +
                        std::to_string(second));
    }
    void ProcessObject(std::uint32_t slot,
                       std::uint32_t target) override {
        calls.push_back("process " + std::to_string(slot) + " " +
                        std::to_string(target));
        if (throw_on_candidate && target == 111u)
            throw std::runtime_error("candidate processing failed");
    }
    bool Matches(std::uint32_t slot, std::uint32_t kind) override {
        calls.push_back("matches " + std::to_string(slot) + " " +
                        std::to_string(kind));
        return matches[slot];
    }
    std::uint32_t EnumerateTargets(std::uint32_t target,
                                    std::uint8_t selector) override {
        calls.push_back("enumerate " + std::to_string(target) + " " +
                        std::to_string(selector));
        return ++enumeration_count;
    }
    std::uint32_t VectorBegin(std::uint32_t vector) override {
        calls.push_back("begin " + std::to_string(vector));
        return vector == 1u ? 8000u : 9000u;
    }
    std::uint32_t VectorEnd(std::uint32_t vector) override {
        calls.push_back("end " + std::to_string(vector));
        return vector == 1u ? 8012u : 9008u;
    }
    std::uint32_t CandidateStatus(std::uint32_t candidate) override {
        calls.push_back("status " + std::to_string(candidate));
        return candidate == 222u || candidate == 444u ? 4u : 2u;
    }
    std::uint32_t FreeVector(std::uint32_t vector) override {
        calls.push_back("free " + std::to_string(vector));
        return vector == 1u ? 0x12u : 0x134u;
    }
};

void Prepare(Runtime& runtime) {
    constexpr std::uint32_t record = 0x1234u;
    runtime.words[record + 20u] = std::bit_cast<std::uint32_t>(0.0f);
    runtime.words[record + 24u] = std::bit_cast<std::uint32_t>(1.0f);
    runtime.words[record + 28u] = 7u;
    runtime.words[record + 32u] = std::bit_cast<std::uint32_t>(2.8f);
    runtime.words[record + 36u] = std::bit_cast<std::uint32_t>(0.5f);
    runtime.words[record + 40u] = std::bit_cast<std::uint32_t>(3.9f);
    runtime.words[record + 44u] = std::bit_cast<std::uint32_t>(-4.9f);
    runtime.words[record + 48u] = std::bit_cast<std::uint32_t>(1.5f);
    runtime.words[record + 52u] = std::bit_cast<std::uint32_t>(5.9f);
    runtime.words[8000u] = 111u;
    runtime.words[8004u] = 0u;
    runtime.words[8008u] = 222u;
    runtime.words[9000u] = 333u;
    runtime.words[9004u] = 444u;
}

void FirstGateRejects() {
    Runtime runtime;
    Prepare(runtime);
    runtime.randoms[0] = 1u;
    Check(pal4::CombatEventOutcome::Process(
              0x1234u, 9u, 10u, 7000u, runtime) == 0u &&
              runtime.calls == std::vector<std::string>{
                  "random", "read 4680"},
          "first probability failure avoids event construction");
}

void SecondGateIsStrict() {
    Runtime runtime;
    Prepare(runtime);
    runtime.words[0x1234u + 24u] = 0u;
    Check(pal4::CombatEventOutcome::Process(
              0x1234u, 9u, 10u, 7000u, runtime) == 0u &&
              runtime.random_index == 2u &&
              runtime.calls.back() == "read 4684" &&
              runtime.calls[3] == "convert 3.900000" &&
              runtime.calls[7] == "set 0 7000 2 0.500000 3" &&
              runtime.calls[13] == "set 1 7000 -4 1.500000 5" &&
              runtime.calls[14] == "process 0 7000" &&
              runtime.calls[15] == "process 1 7000",
          "two events process before strict second comparison");
}

void EnumerateBothGroups() {
    Runtime runtime;
    Prepare(runtime);
    Check(pal4::CombatEventOutcome::Process(
              0x1234u, 9u, 10u, 7000u, runtime) == 0x34u &&
              runtime.enumeration_count == 2u,
          "second free return supplies low byte");
    const auto all = runtime.calls;
    const auto find = [&](const std::string& value) {
        for (std::size_t i = 0; i < all.size(); ++i)
            if (all[i] == value) return i;
        return all.size();
    };
    Check(find("enumerate 7000 52") < find("process 0 111") &&
              find("process 0 111") < find("free 1") &&
              find("free 1") < find("matches 1 7") &&
              find("matches 1 7") < find("process 1 333") &&
              find("process 1 333") < find("free 2"),
          "groups enumerate, process and free in native order");
    Check(find("process 0 222") == all.size() &&
              find("process 1 444") == all.size() &&
              find("process 0 0") == all.size(),
          "null and state-four candidates are skipped");
    Check(find("read 4688") < find("matches 0 7") &&
              find("free 1") < find("matches 1 7"),
          "kind field is reread for second group");
}

void MatchFailuresAndCleanup() {
    Runtime no_matches;
    Prepare(no_matches);
    no_matches.matches[0] = false;
    no_matches.matches[1] = false;
    Check(pal4::CombatEventOutcome::Process(
              0x1234u, 0u, 0u, 7000u, no_matches) == 0u &&
              no_matches.enumeration_count == 0u,
          "failed matches avoid target enumeration");

    Runtime first_only;
    Prepare(first_only);
    first_only.matches[1] = false;
    Check(pal4::CombatEventOutcome::Process(
              0x1234u, 0u, 0u, 7000u, first_only) == 0u &&
              first_only.enumeration_count == 1u &&
              first_only.calls.back() == "matches 1 7",
          "failed second match follows first group cleanup");

    Runtime throws;
    Prepare(throws);
    throws.throw_on_candidate = true;
    bool caught = false;
    try {
        static_cast<void>(pal4::CombatEventOutcome::Process(
            0x1234u, 0u, 0u, 7000u, throws));
    } catch (const std::runtime_error&) {
        caught = true;
    }
    Check(caught && throws.calls.back() == "free 1",
          "temporary vector is released when candidate processing throws");
}

}  // namespace

int main() {
    try {
        FirstGateRejects();
        SecondGateIsStrict();
        EnumerateBothGroups();
        MatchFailuresAndCleanup();
        std::cout << "combat event outcome tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
