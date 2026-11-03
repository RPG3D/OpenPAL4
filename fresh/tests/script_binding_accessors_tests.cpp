#include "pal4/script_binding_accessors.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IScriptBindingAccessorRuntime {
    std::vector<std::string> calls;

    std::int32_t InvokeInteger(std::uint32_t callback,
                               std::uint32_t argument) override {
        calls.push_back("integer " + std::to_string(callback) + " " +
                        std::to_string(argument));
        return -31;
    }
    double InvokeFloating(std::uint32_t callback,
                           std::uint32_t argument) override {
        calls.push_back("floating " + std::to_string(callback) + " " +
                        std::to_string(argument));
        return 2.75;
    }
    std::int32_t InvokeSetter(std::uint32_t callback,
                              std::uint32_t first,
                              std::uint32_t second) override {
        calls.push_back("setter " + std::to_string(callback) + " " +
                        std::to_string(first) + " " +
                        std::to_string(second));
        return 83;
    }
};

void Accessors() {
    pal4::ScriptEnumObjectState state;
    Runtime runtime;
    Check(pal4::ScriptBindingAccessors::GetInteger(
              state, 3000u, runtime) == 0 &&
              pal4::ScriptBindingAccessors::GetFloating(
                  state, 3000u, runtime) == 0.0 &&
              pal4::ScriptBindingAccessors::SetInteger(
                  state, 4000u, 5000u, -19, runtime) == -19 &&
              pal4::ScriptBindingAccessors::SetFloating(
                  state, 4000u, 5000u, 41, runtime) == 41 &&
              runtime.calls.empty(),
          "null callbacks preserve each native fallback, including incoming EAX");

    state.words[2] = 0x1000u;
    state.words[4] = 0x2000u;
    Check(pal4::ScriptBindingAccessors::GetInteger(
              state, 3000u, runtime) == -31 &&
              pal4::ScriptBindingAccessors::SetInteger(
                  state, 4000u, 5000u, -19, runtime) == 83 &&
              pal4::ScriptBindingAccessors::GetFloating(
                  state, 6000u, runtime) == 2.75 &&
              pal4::ScriptBindingAccessors::SetFloating(
                  state, 7000u, 8000u, 41, runtime) == 83,
          "all four accessors forward their callback result");
    Check(runtime.calls == std::vector<std::string>{
              "integer 4096 3000", "setter 8192 4000 5000",
              "floating 4096 6000", "setter 8192 7000 8000"},
          "getter +8 and setter +16 callbacks receive native argument order");
}

void RoleAccessors() {
    pal4::ScriptRoleBindingState state;
    Runtime runtime;
    Check(pal4::ScriptBindingAccessors::GetRoleField(
              state, 1000u, runtime) == 0 &&
              pal4::ScriptBindingAccessors::SetRoleField(
                  state, 2000u, 3000u, -18, runtime) == -18 &&
              runtime.calls.empty(),
          "role callback null paths preserve zero or the incoming register");
    state.words[4] = 0x4000u;
    state.words[6] = 0x5000u;
    Check(pal4::ScriptBindingAccessors::GetRoleField(
              state, 6000u, runtime) == -31 &&
              pal4::ScriptBindingAccessors::SetRoleField(
                  state, 7000u, 8000u, -18, runtime) == 83 &&
              runtime.calls == std::vector<std::string>{
                  "integer 16384 6000", "setter 20480 7000 8000"},
          "role getter +16 and setter +24 forward arguments and results");
}

}  // namespace

int main() {
    try {
        Accessors();
        RoleAccessors();
        std::cout << "script binding accessors tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
