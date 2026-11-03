#include "pal4/combat_probability_outcome.h"

#include <bit>
#include <cmath>
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

struct Runtime final : pal4::ICombatProbabilityOutcomeRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> calls;
    std::uint32_t random = 0u;
    std::int32_t returned = 71;

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
    std::int32_t ApplyOutcome(std::uint32_t target,
                              std::int32_t value,
                              std::uint32_t context) override {
        calls.push_back("apply " + std::to_string(target) + " " +
                        std::to_string(value) + " " +
                        std::to_string(context));
        return returned;
    }
};

void SetThreshold(Runtime& runtime, float value) {
    runtime.words[1020u] = std::bit_cast<std::uint32_t>(value);
}

void RejectedRoll() {
    Runtime runtime;
    runtime.random = 1500u;
    SetThreshold(runtime, 0.4f);
    Check(pal4::CombatProbabilityOutcome::Process(
              1000u, 99u, 3000u, 4000u, runtime) == 0 &&
              runtime.calls == std::vector<std::string>{
                  "random", "read 1020"},
          "roll uses modulo 1000 and rejects values above threshold");
}

void AcceptedAndLastResult() {
    Runtime runtime;
    runtime.random = 0u;
    SetThreshold(runtime, 0.0f);
    runtime.words[1024u] = std::bit_cast<std::uint32_t>(2.8f);
    runtime.words[1028u] = std::bit_cast<std::uint32_t>(-3.9f);
    runtime.words[1052u] = std::bit_cast<std::uint32_t>(5.0f);
    Check(pal4::CombatProbabilityOutcome::Process(
              1000u, 123u, 3000u, 4000u, runtime) == 71,
          "equal roll invokes callbacks and returns last callback result");
    Check(runtime.calls.size() == 21u &&
              runtime.calls[0] == "random" &&
              runtime.calls[1] == "read 1020" &&
              runtime.calls[2] == "read 1024" &&
              runtime.calls[4] == "apply 4000 2 3000" &&
              runtime.calls[7] == "apply 4000 -3 3000" &&
              runtime.calls.back() == "apply 4000 5 3000",
          "eight entries are converted in order and only nonzero ones dispatch");

    runtime.calls.clear();
    runtime.words[1052u] = 0u;
    Check(pal4::CombatProbabilityOutcome::Process(
              1000u, 0u, 3000u, 4000u, runtime) == 0 &&
              runtime.calls.back() == "convert 0.000000",
          "last zero conversion replaces earlier callback result");
}

void UnorderedComparison() {
    Runtime runtime;
    runtime.random = 999u;
    SetThreshold(runtime, std::numeric_limits<float>::quiet_NaN());
    runtime.words[1052u] = std::bit_cast<std::uint32_t>(1.0f);
    Check(pal4::CombatProbabilityOutcome::Process(
              1000u, 0u, 3000u, 4000u, runtime) == 71 &&
              runtime.calls.back() == "apply 4000 1 3000",
          "unordered x87 comparison enters eight-entry loop");
}

struct ExtendedRuntime final : pal4::ICombatProbabilityOutcomeExtendedRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> calls;
    std::uint32_t random = 0u;
    std::uint32_t event = 9000u;
    float event_value = 4.0f;

    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.push_back("read " + std::to_string(address));
        return words[address];
    }
    std::int32_t FloatToInt(float value) override {
        calls.push_back("convert " + std::to_string(value));
        return static_cast<std::int32_t>(value);
    }
    std::uint32_t RandomNumber() override {
        calls.push_back("random");
        return random;
    }
    std::uint32_t LookupEvent(std::int32_t index) override {
        calls.push_back("lookup " + std::to_string(index));
        return event;
    }
    float QueryEventValue(std::uint32_t address,
                          std::uint32_t actor_data) override {
        calls.push_back("query " + std::to_string(address) + " " +
                        std::to_string(actor_data));
        return event_value;
    }
    std::int32_t ApplyOutcome(std::uint32_t target, std::int32_t value,
                              std::uint32_t actor) override {
        calls.push_back("apply " + std::to_string(target) + " " +
                        std::to_string(value) + " " +
                        std::to_string(actor));
        return 73;
    }
};

void ExtendedEvent() {
    ExtendedRuntime runtime;
    runtime.words[1020u] = std::bit_cast<std::uint32_t>(0.5f);
    runtime.words[1024u] = std::bit_cast<std::uint32_t>(39.0f);
    runtime.words[1028u] = std::bit_cast<std::uint32_t>(4.0f);
    runtime.words[1032u] = std::bit_cast<std::uint32_t>(-7.0f);
    Check(pal4::CombatProbabilityOutcomeExtended::Process(
              1000u, 0u, 3000u, 4000u, runtime) == 73,
          "matching event dispatches the converted value");
    Check(runtime.calls == std::vector<std::string>{
              "read 1024", "convert 39.000000", "random", "read 1020",
              "lookup 39", "query 9000 4340", "convert 4.000000",
              "read 1028", "convert 4.000000", "read 1032",
              "convert -7.000000", "apply 3000 -7 4000"},
          "extended event preserves native read and dispatch order");

    runtime.calls.clear();
    runtime.words[1024u] = std::bit_cast<std::uint32_t>(48.0f);
    Check(pal4::CombatProbabilityOutcomeExtended::Process(
              1000u, 0u, 3000u, 4000u, runtime) == 48 &&
              runtime.calls.size() == 2u,
          "out-of-range event returns its converted index");

    runtime.calls.clear();
    runtime.words[1024u] = std::bit_cast<std::uint32_t>(47.0f);
    runtime.random = 999u;
    Check(pal4::CombatProbabilityOutcomeExtended::Process(
              1000u, 0u, 3000u, 4000u, runtime) == 0 &&
              runtime.calls.size() == 4u,
          "rejected probability does not look up the event");

    runtime.calls.clear();
    runtime.random = 0u;
    runtime.event = 0u;
    Check(pal4::CombatProbabilityOutcomeExtended::Process(
              1000u, 0u, 3000u, 4000u, runtime) == 0 &&
              runtime.calls.back() == "lookup 47",
          "missing event stops before querying its value");

    runtime.calls.clear();
    runtime.event = 9000u;
    runtime.event_value = 3.0f;
    Check(pal4::CombatProbabilityOutcomeExtended::Process(
              1000u, 0u, 3000u, 4000u, runtime) == 4 &&
              runtime.calls.back() == "convert 4.000000",
          "mismatched value returns the converted expected value");

    runtime.calls.clear();
    runtime.event_value = 4.0f;
    runtime.words[1020u] = std::bit_cast<std::uint32_t>(
        std::numeric_limits<float>::quiet_NaN());
    Check(pal4::CombatProbabilityOutcomeExtended::Process(
              1000u, 0u, 3000u, 4000u, runtime) == 73,
          "unordered x87 comparison admits the event");
}

}  // namespace

int main() {
    try {
        RejectedRoll();
        AcceptedAndLastResult();
        UnorderedComparison();
        ExtendedEvent();
        std::cout << "combat probability outcome tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
