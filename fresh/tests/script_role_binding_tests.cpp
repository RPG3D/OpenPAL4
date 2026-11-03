#include "pal4/script_role_binding.h"

#include <array>
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

using Constructor = std::uint32_t (*)(
    pal4::ScriptRoleBindingState&, std::uint32_t,
    std::uint32_t, std::uint32_t);

struct Runtime final : pal4::IScriptEnumObjectLifecycleRuntime {
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> calls;
    const std::uint32_t* vtable_word = nullptr;
    std::vector<std::uint32_t> observed_vtables;

    std::uint8_t ReadByte(std::uint32_t address) override {
        calls.push_back("read " + std::to_string(address));
        return bytes.at(address);
    }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        calls.push_back("write " + std::to_string(address) + " " +
                        std::to_string(value));
        bytes[address] = value;
    }
    std::uint32_t ReadDword(std::uint32_t) override { return 0u; }
    void Free(std::uint32_t address) override {
        calls.push_back("free " + std::to_string(address));
    }
    void DestroyAdapter(std::uint32_t address,
                        std::uint32_t flags) override {
        if (vtable_word) observed_vtables.push_back(*vtable_word);
        calls.push_back("adapter " + std::to_string(address) + " " +
                        std::to_string(flags));
    }
    void CleanupEmptyTemporary() override {}
    void DestroyArray(std::uint32_t, std::uint32_t,
                      std::uint32_t) override {}
};

void Constructors() {
    constexpr std::array<std::uint32_t, 7> vtables{{
        0x848218u, 0x848230u, 0x848248u, 0x848260u,
        0x848278u, 0x848290u, 0x8482a8u}};
    const std::array<Constructor, 7> constructors{{
        &pal4::ScriptRoleBinding::ConstructFieldThree,
        &pal4::ScriptRoleBinding::ConstructFieldFour,
        &pal4::ScriptRoleBinding::ConstructFieldFive,
        &pal4::ScriptRoleBinding::ConstructFieldSix,
        &pal4::ScriptRoleBinding::ConstructFieldSeven,
        &pal4::ScriptRoleBinding::ConstructFieldEight,
        &pal4::ScriptRoleBinding::ConstructRace}};

    for (std::size_t index = 0; index < constructors.size(); ++index) {
        pal4::ScriptRoleBindingState state;
        state.words.fill(0xffffffffu);
        Check(constructors[index](state, 0x5000u,
                                  0x123456abu, 9u) == 0x5000u &&
                  state.words == std::array<std::uint32_t, 11>{
                      vtables[index], 0x123456abu, 9u,
                      0xffffff00u, 0u, 0xffffff00u, 0u,
                      0xffffffabu, 0u, 0u, 0u},
              "every 44-byte constructor keeps exact fields and selects its vtable");
    }
    Check(pal4::ScriptRoleBinding::NameAddress(0x5000u) ==
              0x501cu,
          "role binding name accessor returns object +28");
}

void Destructors() {
    using Destructor = std::uint8_t (*)(
        pal4::ScriptRoleBindingState&,
        pal4::IScriptEnumObjectLifecycleRuntime&);
    const std::array<Destructor, 11> destructors{{
        &pal4::ScriptRoleBinding::DestroyCombatPosition,
        &pal4::ScriptRoleBinding::DestroyStandaloneOne,
        &pal4::ScriptRoleBinding::DestroyStandaloneTwo,
        &pal4::ScriptRoleBinding::DestroyStandaloneThree,
        &pal4::ScriptRoleBinding::DestroyFieldThree,
        &pal4::ScriptRoleBinding::DestroyFieldFour,
        &pal4::ScriptRoleBinding::DestroyFieldFive,
        &pal4::ScriptRoleBinding::DestroyFieldSix,
        &pal4::ScriptRoleBinding::DestroyFieldSeven,
        &pal4::ScriptRoleBinding::DestroyFieldEight,
        &pal4::ScriptRoleBinding::DestroyRace}};
    constexpr std::array<std::uint32_t, 11> vtables{{
        0x8481e8u, 0x8481b8u, 0x8481a0u, 0x848188u,
        0x848218u, 0x848230u, 0x848248u, 0x848260u,
        0x848278u, 0x848290u, 0x8482a8u}};
    for (std::size_t index = 0; index < destructors.size(); ++index) {
        const auto destroy = destructors[index];
        pal4::ScriptRoleBindingState state;
        state.words = {0x99999999u, 0x8ee2e8u, 6u,
                       0xabcdef01u, 0x2000u, 0x12345601u, 0x3000u,
                       0x77u, 0x4000u, 5u, 8u};
        Runtime runtime;
        runtime.bytes[0x3fffu] = 2u;
        runtime.vtable_word = &state.words[0];
        Check(destroy(state, runtime) == 0u &&
                  state.words == std::array<std::uint32_t, 11>{
                      0x842bd4u, 0x8ee2e8u, 6u,
                      0xabcdef00u, 0u, 0x12345600u, 0u,
                      0x77u, 0u, 0u, 0u} &&
                  runtime.calls == std::vector<std::string>{
                      "adapter 8192 1", "adapter 12288 1",
                      "read 16383", "write 16383 1"} &&
                  runtime.observed_vtables ==
                      std::vector<std::uint32_t>{
                          vtables[index], vtables[index]},
              "all 44-byte destructors install their vtable before release");

        state.words = {0u, 0u, 0u, 0xabcdef01u, 0u,
                       0x12345600u, 0x3000u, 0u,
                       0x4000u, 1u, 1u};
        runtime.calls.clear();
        runtime.observed_vtables.clear();
        runtime.bytes[0x3fffu] = 0xffu;
        Check(destroy(state, runtime) == 1u &&
                  state.words[3] == 0xabcdef01u &&
                  runtime.calls == std::vector<std::string>{
                      "read 16383", "free 16383"},
              "null first adapter retains flag while saturated text is freed");
    }
}

}  // namespace

int main() {
    try {
        Constructors();
        Destructors();
        std::cout << "script role binding tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
