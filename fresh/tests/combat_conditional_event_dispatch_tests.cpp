#include "pal4/combat_conditional_event_dispatch.h"

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

struct Runtime final : pal4::ICombatConditionalEventDispatchRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> calls;
    bool exists = false;

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
    bool ElementExists(std::uint32_t context, std::int32_t id) override {
        calls.push_back("exists " + std::to_string(context) + " " +
                        std::to_string(id));
        return exists;
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

void Branches() {
    Runtime found;
    found.exists = true;
    SetFloat(found, 20u, 11.9f);
    SetFloat(found, 24u, -4.9f);
    SetFloat(found, 28u, 99.0f);
    pal4::CombatConditionalEventDispatch::Process(
        1000u, 300u, 400u, found);
    Check(found.calls == std::vector<std::string>{
              "read 1020", "convert 11.900000", "exists 400 11",
              "read 1024", "convert -4.900000", "dispatch 300 -4 400"},
          "existing element selects and dispatches action plus 24");

    Runtime missing;
    SetFloat(missing, 20u, 11.9f);
    SetFloat(missing, 24u, 99.0f);
    SetFloat(missing, 28u, 7.8f);
    pal4::CombatConditionalEventDispatch::Process(
        1000u, 300u, 400u, missing);
    Check(missing.calls == std::vector<std::string>{
              "read 1020", "convert 11.900000", "exists 400 11",
              "read 1028", "convert 7.800000", "dispatch 300 7 400"},
          "missing element selects and dispatches action plus 28");
}

void ZeroAndUnordered() {
    Runtime zero;
    zero.exists = true;
    SetFloat(zero, 20u, 3.0f);
    SetFloat(zero, 24u, -0.0f);
    pal4::CombatConditionalEventDispatch::Process(
        1000u, 300u, 400u, zero);
    Check(zero.calls == std::vector<std::string>{
              "read 1020", "convert 3.000000", "exists 400 3",
              "read 1024"},
          "negative zero skips conversion and dispatch");

    Runtime unordered;
    SetFloat(unordered, 20u, 3.0f);
    SetFloat(unordered, 28u, std::numeric_limits<float>::quiet_NaN());
    pal4::CombatConditionalEventDispatch::Process(
        1000u, 300u, 400u, unordered);
    Check(unordered.calls.back() == "dispatch 300 -2147483648 400" &&
              unordered.calls[4] == "convert nan",
          "unordered selected float reaches CRT conversion and dispatch");
}

}  // namespace

int main() {
    try {
        Branches();
        ZeroAndUnordered();
        std::cout << "combat conditional event dispatch tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
