#include "pal4/script_skill_global_record.h"

#include <array>
#include <bit>
#include <utility>

namespace pal4 {

std::uint32_t ScriptSkillGlobalRecord::InitializeTextFields(
    std::uint32_t record, IScriptSkillGlobalRuntime& runtime) {
    const auto high_byte = static_cast<std::uint8_t>(record >> 24u);
    for (const auto offset : {68u, 84u, 100u, 116u}) {
        runtime.WriteByte(record + offset, high_byte);
        runtime.WriteWord(record + offset + 4u, 0u);
        runtime.WriteWord(record + offset + 8u, 0u);
        runtime.WriteWord(record + offset + 12u, 0u);
    }
    return record;
}

std::uint32_t ScriptSkillGlobalRecord::GetOrInitialize(
    std::uint32_t slot, std::uint8_t& guard,
    IScriptSkillGlobalRuntime& runtime) {
    if (slot != 0u) return slot;
    if ((guard & 1u) == 0u) {
        guard = static_cast<std::uint8_t>(guard | 1u);
        static_cast<void>(InitializeTextFields(kRecordAddress, runtime));
        static_cast<void>(runtime.RegisterAtExit(kCleanupEntry));
    }
    for (const auto [offset, value] : std::array{
             std::pair{4u, 4u}, std::pair{0u, 0xffffffffu},
             std::pair{60u, 0xffffffffu}, std::pair{64u, 0xffffffffu},
             std::pair{8u, 15u}, std::pair{12u, 0u},
             std::pair{16u, 0u}, std::pair{56u, 0u}})
        runtime.WriteWord(kRecordAddress + offset, value);
    return kRecordAddress;
}

std::uint32_t ScriptSkillGlobalRecord::Release(
    IScriptSkillGlobalRuntime& runtime) {
    return SkillDataRecordText::Release(kRecordAddress, runtime);
}

ScriptSkillCounterState& ScriptSkillCounter::Construct(
    ScriptSkillCounterState& state, std::uint32_t skill,
    std::uint32_t context) {
    static_cast<void>(ScriptSkillContext::Construct(state.context,
                                                    skill, context));
    state.count = 0u;
    return state;
}

std::uint32_t ScriptSkillCounter::Add(
    ScriptSkillCounterState& state, std::uint32_t amount) {
    state.count += amount;
    return amount;
}

bool ScriptSkillCounter::ReachedLimit(
    const ScriptSkillCounterState& state,
    IScriptSkillContextRuntime& runtime) {
    return std::bit_cast<std::int32_t>(state.count) >=
           std::bit_cast<std::int32_t>(
               runtime.ReadWord(state.context.skill + 12u));
}

}  // namespace pal4
