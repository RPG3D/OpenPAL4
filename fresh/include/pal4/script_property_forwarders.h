#pragma once

#include <cstdint>
#include <span>

namespace pal4 {

struct ScriptForwardResult {
    std::uint32_t eax = 0u;
    double st0 = 0.0;
    bool has_st0 = false;
};

class IScriptPropertyForwardRuntime {
public:
    virtual ~IScriptPropertyForwardRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual ScriptForwardResult PreservedRegisters() = 0;
    [[nodiscard]] virtual ScriptForwardResult CallVirtual(
        std::uint32_t link, std::uint32_t virtual_offset,
        std::span<const std::uint32_t> arguments) = 0;
};

class ScriptPropertyForwarders final {
public:
    using Arguments = std::span<const std::uint32_t>;

    // IDA 0x56e680-0x56eaf0. These tail jumps do not check the linked object.
    [[nodiscard]] static ScriptForwardResult ForwardUnguarded(
        std::uint32_t entry, std::uint32_t object, Arguments arguments,
        IScriptPropertyForwardRuntime& runtime);

    // IDA 0x56e530-0x56e670. Missing links preserve the incoming result.
    [[nodiscard]] static ScriptForwardResult Forward28(
        std::uint32_t object, Arguments arguments,
        IScriptPropertyForwardRuntime& runtime);
    [[nodiscard]] static ScriptForwardResult Forward32(
        std::uint32_t object, Arguments arguments,
        IScriptPropertyForwardRuntime& runtime);
    [[nodiscard]] static ScriptForwardResult Forward36(
        std::uint32_t object, Arguments arguments,
        IScriptPropertyForwardRuntime& runtime);
    [[nodiscard]] static ScriptForwardResult Forward40(
        std::uint32_t object, Arguments arguments,
        IScriptPropertyForwardRuntime& runtime);
    [[nodiscard]] static ScriptForwardResult Forward44(
        std::uint32_t object, Arguments arguments,
        IScriptPropertyForwardRuntime& runtime);
    [[nodiscard]] static ScriptForwardResult Forward48(
        std::uint32_t object, Arguments arguments,
        IScriptPropertyForwardRuntime& runtime);
    [[nodiscard]] static ScriptForwardResult Forward52(
        std::uint32_t object, Arguments arguments,
        IScriptPropertyForwardRuntime& runtime);
    [[nodiscard]] static ScriptForwardResult Forward56(
        std::uint32_t object, Arguments arguments,
        IScriptPropertyForwardRuntime& runtime);
    [[nodiscard]] static ScriptForwardResult Forward60(
        std::uint32_t object, Arguments arguments,
        IScriptPropertyForwardRuntime& runtime);
    [[nodiscard]] static ScriptForwardResult Forward64(
        std::uint32_t object, Arguments arguments,
        IScriptPropertyForwardRuntime& runtime);
    [[nodiscard]] static ScriptForwardResult Forward68(
        std::uint32_t object, Arguments arguments,
        IScriptPropertyForwardRuntime& runtime);
    [[nodiscard]] static ScriptForwardResult Forward72(
        std::uint32_t object, Arguments arguments,
        IScriptPropertyForwardRuntime& runtime);
    [[nodiscard]] static ScriptForwardResult Forward76(
        std::uint32_t object, Arguments arguments,
        IScriptPropertyForwardRuntime& runtime);
    [[nodiscard]] static ScriptForwardResult Forward80(
        std::uint32_t object, Arguments arguments,
        IScriptPropertyForwardRuntime& runtime);
    [[nodiscard]] static ScriptForwardResult Forward84(
        std::uint32_t object, Arguments arguments,
        IScriptPropertyForwardRuntime& runtime);
    [[nodiscard]] static ScriptForwardResult Forward88(
        std::uint32_t object, Arguments arguments,
        IScriptPropertyForwardRuntime& runtime);
    [[nodiscard]] static ScriptForwardResult Forward92(
        std::uint32_t object, Arguments arguments,
        IScriptPropertyForwardRuntime& runtime);
    [[nodiscard]] static ScriptForwardResult Forward96(
        std::uint32_t object, Arguments arguments,
        IScriptPropertyForwardRuntime& runtime);
    [[nodiscard]] static ScriptForwardResult Forward100(
        std::uint32_t object, Arguments arguments,
        IScriptPropertyForwardRuntime& runtime);
    [[nodiscard]] static ScriptForwardResult Forward104(
        std::uint32_t object, Arguments arguments,
        IScriptPropertyForwardRuntime& runtime);
    [[nodiscard]] static ScriptForwardResult Forward108(
        std::uint32_t object, Arguments arguments,
        IScriptPropertyForwardRuntime& runtime);
};

}  // namespace pal4
