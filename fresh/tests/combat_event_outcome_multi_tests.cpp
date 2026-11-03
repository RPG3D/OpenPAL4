#include "pal4/combat_event_outcome.h"

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

struct Runtime final : pal4::ICombatMultiEffectRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> calls;
    std::uint32_t random = 0u;
    bool throw_during_process = false;

    std::uint32_t RandomNumber() override {
        calls.push_back("random");
        return random;
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
        if (throw_during_process)
            throw std::runtime_error("process failure");
    }
    bool Matches(std::uint32_t, std::uint32_t) override {
        throw std::runtime_error("unexpected Matches call");
    }
    std::uint32_t EnumerateTargets(std::uint32_t target,
                                    std::uint8_t selector) override {
        calls.push_back("enumerate " + std::to_string(target) + " " +
                        std::to_string(selector));
        return 1u;
    }
    std::uint32_t VectorBegin(std::uint32_t vector) override {
        Check(vector == 1u, "vector handle");
        calls.push_back("begin");
        return 8000u;
    }
    std::uint32_t VectorEnd(std::uint32_t vector) override {
        Check(vector == 1u, "vector handle");
        calls.push_back("end");
        return 8012u;
    }
    std::uint32_t CandidateStatus(std::uint32_t) override {
        throw std::runtime_error("unexpected CandidateStatus call");
    }
    std::uint32_t FreeVector(std::uint32_t vector) override {
        Check(vector == 1u, "vector handle");
        calls.push_back("free");
        return 0x77u;
    }
    bool CheckCandidateActive(std::uint32_t candidate) override {
        calls.push_back("active " + std::to_string(candidate));
        return candidate != 0u;
    }
    void DispatchPair(std::uint32_t source, std::int32_t first,
                      std::int32_t second) override {
        calls.push_back("pair " + std::to_string(source) + " " +
                        std::to_string(first) + " " +
                        std::to_string(second));
    }
};

void SetFloat(Runtime& runtime, std::uint32_t offset, float value) {
    runtime.words[1000u + offset] = std::bit_cast<std::uint32_t>(value);
}

void Prepare(Runtime& runtime) {
    runtime.words[8000u] = 111u;
    runtime.words[8004u] = 0u;
    runtime.words[8008u] = 222u;
}

std::size_t Find(const std::vector<std::string>& calls,
                 const std::string& expected) {
    for (std::size_t i = 0; i < calls.size(); ++i)
        if (calls[i] == expected) return i;
    return calls.size();
}

void MultiTargetGateAndEffects() {
    Runtime failed;
    Prepare(failed);
    SetFloat(failed, 20u, 0.0f);
    failed.random = 1u;
    Check(pal4::CombatEventOutcome::ProcessMultiTarget(
              1000u, 4000u, failed) == 0 &&
              failed.calls == std::vector<std::string>{
                  "random", "read 1020"},
          "failed multi-target gate does not enumerate");

    Runtime runtime;
    Prepare(runtime);
    SetFloat(runtime, 20u, 0.0f);
    SetFloat(runtime, 24u, 1.9f);
    SetFloat(runtime, 28u, 0.5f);
    SetFloat(runtime, 32u, 2.9f);
    SetFloat(runtime, 36u, -3.9f);
    SetFloat(runtime, 40u, 1.5f);
    SetFloat(runtime, 44u, 4.9f);
    Check(pal4::CombatEventOutcome::ProcessMultiTarget(
              1000u, 4000u, runtime) == 0x77 &&
              runtime.calls[2] == "enumerate 4000 232" &&
              runtime.calls.back() == "free",
          "equal gate enumerates with action low byte and returns free result");
    Check(Find(runtime.calls, "convert 2.900000") <
              Find(runtime.calls, "convert 1.900000") &&
              Find(runtime.calls, "set 0 111 1 0.500000 2") <
              Find(runtime.calls, "set 1 111 -3 1.500000 4") &&
              Find(runtime.calls, "set 1 111 -3 1.500000 4") <
              Find(runtime.calls, "process 0 111") &&
              Find(runtime.calls, "process 1 111") <
              Find(runtime.calls, "set 0 222 1 0.500000 2"),
          "each non-null target receives two constructed events in IDA order");
    Check(Find(runtime.calls, "set 0 0 1 0.500000 2") ==
              runtime.calls.size() &&
              Find(runtime.calls, "process 1 222") <
                  Find(runtime.calls, "free"),
          "null target is skipped and vector is freed after final target");

    Runtime throws;
    Prepare(throws);
    SetFloat(throws, 20u, 0.0f);
    throws.throw_during_process = true;
    bool caught = false;
    try {
        static_cast<void>(pal4::CombatEventOutcome::ProcessMultiTarget(
            1000u, 4000u, throws));
    } catch (const std::runtime_error&) {
        caught = true;
    }
    Check(caught && throws.calls.back() == "free",
          "multi-target vector is freed on exceptional exit");
}

void MultiEffectCountAndPairs() {
    Runtime blocked;
    Prepare(blocked);
    SetFloat(blocked, 20u, 1.0f);
    Check(pal4::CombatEventOutcome::ProcessMultiEffect(
              77u, 1000u, 3000u, 4000u, blocked) == 0x77 &&
              blocked.calls[0] == "enumerate 4000 160" &&
              Find(blocked.calls, "active 0") <
                  Find(blocked.calls, "read 1020") &&
              Find(blocked.calls, "pair 3000 1 2") ==
                  blocked.calls.size() &&
              blocked.calls.back() == "free",
          "active count above threshold frees without dispatch");

    Runtime runtime;
    Prepare(runtime);
    SetFloat(runtime, 20u, 2.0f);
    for (std::uint32_t i = 0; i < 8u; ++i)
        SetFloat(runtime, 24u + 4u * i,
                 static_cast<float>(i + 1u));
    Check(pal4::CombatEventOutcome::ProcessMultiEffect(
              77u, 1000u, 3000u, 4000u, runtime) == 0x77 &&
              Find(runtime.calls, "pair 3000 1 2") <
                  Find(runtime.calls, "pair 3000 3 4") &&
              Find(runtime.calls, "pair 3000 3 4") <
                  Find(runtime.calls, "pair 3000 5 6") &&
              Find(runtime.calls, "pair 3000 5 6") <
                  Find(runtime.calls, "pair 3000 7 8") &&
              Find(runtime.calls, "pair 3000 7 8") <
                  Find(runtime.calls, "free"),
          "count equal threshold dispatches four ordered pairs");
    Check(Find(runtime.calls, "convert 2.000000") <
              Find(runtime.calls, "convert 1.000000"),
          "each pair converts its second float before its first");

    Runtime nan;
    Prepare(nan);
    SetFloat(nan, 20u, std::numeric_limits<float>::quiet_NaN());
    Check(pal4::CombatEventOutcome::ProcessMultiEffect(
              0u, 1000u, 3000u, 4000u, nan) == 0x77 &&
              Find(nan.calls, "pair 3000 0 0") < nan.calls.size(),
          "unordered count comparison enters dispatch path");
}

}  // namespace

int main() {
    try {
        MultiTargetGateAndEffects();
        MultiEffectCountAndPairs();
        std::cout << "combat event outcome multi tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
