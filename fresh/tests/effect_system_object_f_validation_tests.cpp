#include "pal4/effect_system_object_f_validation.h"

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IEffectSystemObjectFValidationRuntime {
    std::vector<std::string> events;
    std::uint8_t context_flag = 1u;
    std::uint8_t validation = 1u;
    std::uint8_t checked = 0x7bu;
    std::uint32_t validator = 0x9000u;
    std::uint32_t source = 0u;
    std::uint32_t transform = 0u;

    std::uint8_t ReadByte(std::uint32_t address) override {
        events.push_back("context:" + std::to_string(address));
        return context_flag;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        events.push_back("handle:" + std::to_string(address));
        return 0x6000u;
    }
    std::uint32_t GetObjectTransform(std::uint32_t handle) override {
        events.push_back("transform:" + std::to_string(handle));
        return 0x7000u;
    }
    std::uint32_t EffectSystemValidator() override {
        events.push_back("validator");
        return validator;
    }
    std::uint8_t ValidateTransform(std::uint32_t system,
                                   std::uint32_t input,
                                   std::uint32_t area) override {
        events.push_back("validate:" + std::to_string(system));
        source = input;
        transform = area;
        return validation;
    }
    std::uint8_t CheckObject(std::uint32_t object,
                             std::uint32_t ebp,
                             std::uint32_t edi) override {
        events.push_back("check:" + std::to_string(object));
        Check(ebp == 0x1111u && edi == 0x2222u,
              "native preserved registers reach helper");
        return checked;
    }
    [[noreturn]] void ThrowMissingValidator() override {
        events.push_back("throw");
        throw std::runtime_error("missing validator");
    }
};

pal4::EffectSystemObjectFState ActiveState() {
    pal4::EffectSystemObjectFState state{};
    state.words[1u] = 0x01000001u;
    state.words[3u] = 0x4000u;
    state.words[29u] = 0x5000u;
    return state;
}

std::uint8_t Run(const pal4::EffectSystemObjectFState& state,
                 Runtime& runtime) {
    return pal4::EffectSystemObjectFValidation::Check(
        state, 0x1111u, 0x2222u, 0x3333u, runtime);
}

void TestStateGatesAndDirectCheck() {
    Runtime runtime;
    auto state = ActiveState();
    state.words[1u] = 0u;
    Check(Run(state, runtime) == 0u && runtime.events.empty(),
          "inactive object short circuits");
    state.words[1u] = 1u;
    Check(Run(state, runtime) == 0u && runtime.events.empty(),
          "disabled byte at +7 short circuits");
    state.words[1u] = 0x01000001u;
    state.words[3u] = 0u;
    Check(Run(state, runtime) == 0x7bu &&
              runtime.events == std::vector<std::string>{"check:20480"},
          "null context calls object helper directly");
    runtime.events.clear();
    state.words[3u] = 0x4000u;
    runtime.context_flag = 0u;
    Check(Run(state, runtime) == 0x7bu &&
              runtime.events == std::vector<std::string>{
                  "context:16467", "check:20480"},
          "clear context flag skips validation");
}

void TestValidationAndMissingSystem() {
    Runtime runtime;
    const auto state = ActiveState();
    Check(Run(state, runtime) == 0x7bu &&
              runtime.events == std::vector<std::string>{
                  "context:16467", "handle:20500", "transform:24576",
                  "validator", "validate:36864", "check:20480"} &&
              runtime.source == 0x3333u && runtime.transform == 0x7030u,
          "validation resolves handle and transform area before helper");
    runtime.events.clear();
    runtime.validation = 0u;
    Check(Run(state, runtime) == 0u &&
              runtime.events.back() == "validate:36864",
          "rejected transform does not call object helper");
    runtime.events.clear();
    runtime.validator = 0u;
    try {
        static_cast<void>(Run(state, runtime));
        Check(false, "missing validator must throw");
    } catch (const std::runtime_error&) {
        Check(runtime.events.back() == "throw",
              "missing validator follows native exception boundary");
    }
}

}  // namespace

int main() {
    TestStateGatesAndDirectCheck();
    TestValidationAndMissingSystem();
}
