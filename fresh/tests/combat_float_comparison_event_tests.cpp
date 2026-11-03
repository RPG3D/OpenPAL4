#include "pal4/combat_float_comparison_event.h"

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

struct Runtime final : pal4::ICombatFloatComparisonEventRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> calls;
    std::uint32_t event = 700u;
    double event_value = -1.0;

    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.push_back("read " + std::to_string(address));
        return words[address];
    }
    std::int32_t FloatToInt(float value) override {
        if (std::isnan(value)) {
            calls.push_back("convert nan");
            return std::numeric_limits<std::int32_t>::min();
        }
        calls.push_back("convert " + std::to_string(value));
        return static_cast<std::int32_t>(value);
    }
    std::uint32_t LookupEvent(std::int32_t id) override {
        calls.push_back("lookup " + std::to_string(id));
        return event;
    }
    double ReadEventValue(std::uint32_t value_event,
                          std::uint32_t context_address) override {
        calls.push_back("value " + std::to_string(value_event) + " " +
                        std::to_string(context_address));
        return event_value;
    }
    void OnEqualZero(std::uint32_t context,
                     std::uint32_t target) override {
        calls.push_back("equal-zero " + std::to_string(context) + " " +
                        std::to_string(target));
    }
    void Dispatch(std::uint32_t target, std::int32_t id,
                  std::uint32_t context) override {
        calls.push_back("dispatch " + std::to_string(target) + " " +
                        std::to_string(id) + " " + std::to_string(context));
    }
};

void SetFloat(Runtime& runtime, std::uint32_t offset, float value) {
    runtime.words[1000u + offset] = std::bit_cast<std::uint32_t>(value);
}

void Prepare(Runtime& runtime) {
    SetFloat(runtime, 20u, 17.9f);
    SetFloat(runtime, 24u, 1.0f);
    SetFloat(runtime, 28u, 6.9f);
    SetFloat(runtime, 32u, -9.8f);
}

void LookupAndEqual() {
    Runtime missing;
    Prepare(missing);
    missing.event = 0u;
    pal4::CombatFloatComparisonEvent::Process(1000u, 300u, 400u, missing);
    Check(missing.calls == std::vector<std::string>{
              "read 1020", "convert 17.900000", "lookup 17"},
          "missing event skips value, threshold and dispatch");

    Runtime equal;
    Prepare(equal);
    equal.event_value = -1.00005;
    pal4::CombatFloatComparisonEvent::Process(1000u, 300u, 400u, equal);
    Check(equal.calls == std::vector<std::string>{
              "read 1020", "convert 17.900000", "lookup 17",
              "value 700 740", "read 1024", "read 1028",
              "convert 6.900000", "dispatch 300 6 400"},
          "absolute values within strict tolerance use equal selection");

    Runtime zero;
    Prepare(zero);
    SetFloat(zero, 28u, -0.0f);
    pal4::CombatFloatComparisonEvent::Process(1000u, 300u, 400u, zero);
    Check(zero.calls.back() == "equal-zero 400 300" &&
              zero.calls[5] == "read 1028",
          "equal selection of zero calls context virtual +0x150");
}

void NotEqualAndUnordered() {
    Runtime outside;
    Prepare(outside);
    outside.event_value = 1.0001;
    pal4::CombatFloatComparisonEvent::Process(1000u, 300u, 400u, outside);
    Check(outside.calls == std::vector<std::string>{
              "read 1020", "convert 17.900000", "lookup 17",
              "value 700 740", "read 1024", "read 1032",
              "convert -9.800000", "dispatch 300 -9 400"},
          "difference at or above native tolerance uses not-equal value");

    Runtime zero;
    Prepare(zero);
    zero.event_value = 2.0;
    SetFloat(zero, 32u, 0.0f);
    pal4::CombatFloatComparisonEvent::Process(1000u, 300u, 400u, zero);
    Check(zero.calls.back() == "read 1032",
          "not-equal selection of zero stops without callback");

    Runtime unordered;
    Prepare(unordered);
    unordered.event_value = std::numeric_limits<double>::quiet_NaN();
    pal4::CombatFloatComparisonEvent::Process(1000u, 300u, 400u,
                                              unordered);
    Check(unordered.calls[5] == "read 1032" &&
              unordered.calls.back() == "dispatch 300 -9 400",
          "unordered difference takes not-equal branch");
}

}  // namespace

int main() {
    try {
        LookupAndEqual();
        NotEqualAndUnordered();
        std::cout << "combat float comparison event tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
