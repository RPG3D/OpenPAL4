#pragma once

#include <array>
#include <cstdint>

#include "pal4/script_enum_object_lifecycle.h"

namespace pal4 {

struct ScriptRoleBindingState {
    std::array<std::uint32_t, 11> words{};
};
static_assert(sizeof(ScriptRoleBindingState) == 44u);

class ScriptRoleBinding final {
public:
    // IDA 0x585960, 0x5859a0, 0x5859e0, 0x585a20, 0x585a60,
    // 0x585ad0 and 0x585b10.
    [[nodiscard]] static std::uint32_t ConstructFieldThree(
        ScriptRoleBindingState& state, std::uint32_t object_address,
        std::uint32_t source, std::uint32_t count);
    [[nodiscard]] static std::uint32_t ConstructFieldFour(
        ScriptRoleBindingState& state, std::uint32_t object_address,
        std::uint32_t source, std::uint32_t count);
    [[nodiscard]] static std::uint32_t ConstructFieldFive(
        ScriptRoleBindingState& state, std::uint32_t object_address,
        std::uint32_t source, std::uint32_t count);
    [[nodiscard]] static std::uint32_t ConstructFieldSix(
        ScriptRoleBindingState& state, std::uint32_t object_address,
        std::uint32_t source, std::uint32_t count);
    [[nodiscard]] static std::uint32_t ConstructFieldSeven(
        ScriptRoleBindingState& state, std::uint32_t object_address,
        std::uint32_t source, std::uint32_t count);
    [[nodiscard]] static std::uint32_t ConstructFieldEight(
        ScriptRoleBindingState& state, std::uint32_t object_address,
        std::uint32_t source, std::uint32_t count);
    [[nodiscard]] static std::uint32_t ConstructRace(
        ScriptRoleBindingState& state, std::uint32_t object_address,
        std::uint32_t source, std::uint32_t count);
    // IDA 0x585ac0.
    [[nodiscard]] static std::uint32_t NameAddress(
        std::uint32_t object_address);
    // IDA 0x585b70 / 0x585ce0 / 0x585e50 / 0x585fc0.
    [[nodiscard]] static std::uint8_t DestroyCombatPosition(
        ScriptRoleBindingState& state,
        IScriptEnumObjectLifecycleRuntime& runtime);
    [[nodiscard]] static std::uint8_t DestroyStandaloneOne(
        ScriptRoleBindingState& state,
        IScriptEnumObjectLifecycleRuntime& runtime);
    [[nodiscard]] static std::uint8_t DestroyStandaloneTwo(
        ScriptRoleBindingState& state,
        IScriptEnumObjectLifecycleRuntime& runtime);
    [[nodiscard]] static std::uint8_t DestroyStandaloneThree(
        ScriptRoleBindingState& state,
        IScriptEnumObjectLifecycleRuntime& runtime);
    // IDA 0x586130..0x5869d0.
    [[nodiscard]] static std::uint8_t DestroyFieldThree(
        ScriptRoleBindingState& state,
        IScriptEnumObjectLifecycleRuntime& runtime);
    [[nodiscard]] static std::uint8_t DestroyFieldFour(
        ScriptRoleBindingState& state,
        IScriptEnumObjectLifecycleRuntime& runtime);
    [[nodiscard]] static std::uint8_t DestroyFieldFive(
        ScriptRoleBindingState& state,
        IScriptEnumObjectLifecycleRuntime& runtime);
    [[nodiscard]] static std::uint8_t DestroyFieldSix(
        ScriptRoleBindingState& state,
        IScriptEnumObjectLifecycleRuntime& runtime);
    [[nodiscard]] static std::uint8_t DestroyFieldSeven(
        ScriptRoleBindingState& state,
        IScriptEnumObjectLifecycleRuntime& runtime);
    [[nodiscard]] static std::uint8_t DestroyFieldEight(
        ScriptRoleBindingState& state,
        IScriptEnumObjectLifecycleRuntime& runtime);
    [[nodiscard]] static std::uint8_t DestroyRace(
        ScriptRoleBindingState& state,
        IScriptEnumObjectLifecycleRuntime& runtime);
};

}  // namespace pal4
