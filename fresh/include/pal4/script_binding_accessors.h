#pragma once

#include "pal4/script_enum_object_lifecycle.h"
#include "pal4/script_role_binding.h"

#include <cstdint>

namespace pal4 {

class IScriptBindingAccessorRuntime {
public:
    virtual ~IScriptBindingAccessorRuntime() = default;
    [[nodiscard]] virtual std::int32_t InvokeInteger(
        std::uint32_t callback, std::uint32_t argument) = 0;
    [[nodiscard]] virtual double InvokeFloating(
        std::uint32_t callback, std::uint32_t argument) = 0;
    [[nodiscard]] virtual std::int32_t InvokeSetter(
        std::uint32_t callback, std::uint32_t first,
        std::uint32_t second) = 0;
};

class ScriptBindingAccessors final {
public:
    // IDA 0x585600 / 0x585610.
    [[nodiscard]] static std::int32_t GetInteger(
        const ScriptEnumObjectState& state, std::uint32_t argument,
        IScriptBindingAccessorRuntime& runtime);
    [[nodiscard]] static std::int32_t SetInteger(
        const ScriptEnumObjectState& state, std::uint32_t first,
        std::uint32_t second, std::int32_t incoming_eax,
        IScriptBindingAccessorRuntime& runtime);
    // IDA 0x585840 / 0x585860.
    [[nodiscard]] static double GetFloating(
        const ScriptEnumObjectState& state, std::uint32_t argument,
        IScriptBindingAccessorRuntime& runtime);
    [[nodiscard]] static std::int32_t SetFloating(
        const ScriptEnumObjectState& state, std::uint32_t first,
        std::uint32_t second, std::int32_t incoming_eax,
        IScriptBindingAccessorRuntime& runtime);
    // IDA 0x585aa0 / 0x585ab0.
    [[nodiscard]] static std::int32_t GetRoleField(
        const ScriptRoleBindingState& state, std::uint32_t argument,
        IScriptBindingAccessorRuntime& runtime);
    [[nodiscard]] static std::int32_t SetRoleField(
        const ScriptRoleBindingState& state, std::uint32_t first,
        std::uint32_t second, std::int32_t incoming_eax,
        IScriptBindingAccessorRuntime& runtime);
};

}  // namespace pal4
