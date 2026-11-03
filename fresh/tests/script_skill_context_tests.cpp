#include "pal4/script_skill_context.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IScriptSkillContextRuntime {
    std::uint32_t read_value = 55u;
    std::vector<std::string> calls;

    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.push_back("read " + std::to_string(address));
        return read_value;
    }
    std::uint32_t InvokeVirtual(
        std::uint32_t receiver, std::uint32_t offset,
        std::span<const std::uint32_t> args) override {
        std::string call = "virtual " + std::to_string(receiver) + " " +
                           std::to_string(offset);
        for (auto arg : args) call += " " + std::to_string(arg);
        calls.push_back(call);
        return 0x9000u + offset;
    }
    std::uint32_t SkillSystemInstance() override {
        calls.push_back("system");
        return 0x6000u;
    }
    void ProcessSkillChain(std::uint32_t system, std::uint32_t skill,
                           std::uint32_t actor, std::uint32_t context,
                           std::uint32_t target) override {
        calls.push_back("chain " + std::to_string(system) + " " +
                        std::to_string(skill) + " " +
                        std::to_string(actor) + " " +
                        std::to_string(context) + " " +
                        std::to_string(target));
    }
};

void ConstructorAndAccessors() {
    pal4::ScriptSkillContextState state{};
    Runtime runtime;
    Check(&pal4::ScriptSkillContext::Construct(
              state, 0x5000u, 0x7000u) == &state &&
              state.skill == 0x5000u && state.context == 0x7000u,
          "constructor stores both words and returns the object");
    Check(pal4::ScriptSkillContext::FirstWord(state, runtime) == 55u &&
              pal4::ScriptSkillContext::WordAt64(state, runtime) == 55u &&
              runtime.calls == std::vector<std::string>{
                  "read 20480", "read 20544"},
          "accessors read the two exact skill offsets");
    state.skill = 0u;
    runtime.calls.clear();
    Check(pal4::ScriptSkillContext::FirstWord(state, runtime) ==
              0xffffffffu &&
              pal4::ScriptSkillContext::WordAt64(state, runtime) ==
                  0xffffffffu && runtime.calls.empty(),
          "null skill returns minus-one sentinel without a read");
}

void Handler() {
    Runtime runtime;
    pal4::ScriptSkillContextState state{0x5000u, 0x7000u};
    Check(pal4::ScriptSkillContext::Handle(
              state, 55u, 0x8000u, 0x9000u, runtime) ==
              0x9000u + 312u &&
              runtime.calls == std::vector<std::string>{
                  "read 20488",
                  "virtual 32768 280 20480 55 28672 36864",
                  "virtual 36864 308 28672 20480 55",
                  "system",
                  "chain 24576 20480 32768 28672 36864",
                  "virtual 32768 284 20480 55 28672 36864",
                  "virtual 36864 312 28672 20480 55"},
          "matching skill executes callbacks and chain in native order");

    runtime.calls.clear();
    Check(pal4::ScriptSkillContext::Handle(
              state, 56u, 0x8000u, 0x9000u, runtime) ==
              0x5000u &&
              runtime.calls == std::vector<std::string>{"read 20488"},
          "mismatched skill does not dispatch callbacks");
    runtime.calls.clear();
    state.context = 0u;
    Check(pal4::ScriptSkillContext::Handle(
              state, 55u, 0x8000u, 0x9000u, runtime) ==
              0x5000u && runtime.calls.empty(),
          "null context skips the ID read and callbacks");
    state.skill = 0u;
    state.context = 0x7000u;
    Check(pal4::ScriptSkillContext::Handle(
              state, 55u, 0x8000u, 0x9000u, runtime) == 0u &&
              runtime.calls.empty(),
          "null skill skips the ID read and callbacks");
}

}  // namespace

int main() {
    try {
        ConstructorAndAccessors();
        Handler();
        std::cout << "script skill context tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
