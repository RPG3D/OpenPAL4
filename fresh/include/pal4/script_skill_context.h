#pragma once

#include <cstdint>
#include <span>

namespace pal4 {

struct ScriptSkillContextState {
    std::uint32_t skill = 0u;
    std::uint32_t context = 0u;
};
static_assert(sizeof(ScriptSkillContextState) == 8u);

class IScriptSkillContextRuntime {
public:
    virtual ~IScriptSkillContextRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t InvokeVirtual(
        std::uint32_t receiver, std::uint32_t vtable_offset,
        std::span<const std::uint32_t> arguments) = 0;
    [[nodiscard]] virtual std::uint32_t SkillSystemInstance() = 0;
    virtual void ProcessSkillChain(
        std::uint32_t system, std::uint32_t skill,
        std::uint32_t actor, std::uint32_t context,
        std::uint32_t target) = 0;
};

class ScriptSkillContext final {
public:
    // IDA 0x588ea0.
    [[nodiscard]] static ScriptSkillContextState& Construct(
        ScriptSkillContextState& state, std::uint32_t skill,
        std::uint32_t context);

    // IDA 0x588ec0.
    [[nodiscard]] static std::uint32_t Handle(
        const ScriptSkillContextState& state, std::uint32_t requested_id,
        std::uint32_t actor, std::uint32_t target,
        IScriptSkillContextRuntime& runtime);

    // IDA 0x588f50 / 0x588f60.
    [[nodiscard]] static std::uint32_t FirstWord(
        const ScriptSkillContextState& state,
        IScriptSkillContextRuntime& runtime);
    [[nodiscard]] static std::uint32_t WordAt64(
        const ScriptSkillContextState& state,
        IScriptSkillContextRuntime& runtime);
};

}  // namespace pal4
