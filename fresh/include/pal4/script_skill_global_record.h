#pragma once

#include "pal4/game_data_record_text.h"
#include "pal4/script_skill_context.h"

#include <cstdint>

namespace pal4 {

class IScriptSkillGlobalRuntime : public IGameDataRecordTextRuntime {
public:
    [[nodiscard]] virtual int RegisterAtExit(std::uint32_t callback) = 0;
};

class ScriptSkillGlobalRecord final {
public:
    static constexpr std::uint32_t kRecordAddress = 0x8faa50u;
    static constexpr std::uint32_t kGuardAddress = 0x8faad4u;
    static constexpr std::uint32_t kCleanupEntry = 0x588ff0u;

    // IDA 0x589000. Four text headers inherit the high byte of the address.
    [[nodiscard]] static std::uint32_t InitializeTextFields(
        std::uint32_t record, IScriptSkillGlobalRuntime& runtime);

    // IDA 0x588f70. The input slot is read but is not assigned.
    [[nodiscard]] static std::uint32_t GetOrInitialize(
        std::uint32_t slot, std::uint8_t& guard,
        IScriptSkillGlobalRuntime& runtime);

    // IDA 0x588ff0: tail call to the existing skill text destructor.
    [[nodiscard]] static std::uint32_t Release(
        IScriptSkillGlobalRuntime& runtime);
};

struct ScriptSkillCounterState {
    ScriptSkillContextState context{};
    std::uint32_t count = 0u;
};
static_assert(sizeof(ScriptSkillCounterState) == 12u);

class ScriptSkillCounter final {
public:
    // IDA 0x589050, 0x589080 and 0x589090.
    [[nodiscard]] static ScriptSkillCounterState& Construct(
        ScriptSkillCounterState& state, std::uint32_t skill,
        std::uint32_t context);
    [[nodiscard]] static std::uint32_t Add(
        ScriptSkillCounterState& state, std::uint32_t amount);
    [[nodiscard]] static bool ReachedLimit(
        const ScriptSkillCounterState& state,
        IScriptSkillContextRuntime& runtime);
};

}  // namespace pal4
