#include "pal4/monster_behavior_manager.h"

#include <algorithm>
#include <bit>
#include <cmath>

namespace pal4 {
namespace {

std::uint32_t Read(const MonsterBehaviorManagerState& state,
                   std::uint32_t offset) {
    return state.words[offset / 4u];
}

void Write(MonsterBehaviorManagerState& state, std::uint32_t offset,
           std::uint32_t value) {
    state.words[offset / 4u] = value;
}

float ReadFloat(const MonsterBehaviorManagerState& state,
                std::uint32_t offset) {
    return std::bit_cast<float>(Read(state, offset));
}

void WriteFloat(MonsterBehaviorManagerState& state,
                std::uint32_t offset, float value) {
    Write(state, offset, std::bit_cast<std::uint32_t>(value));
}

char UpperAscii(char value) {
    return value >= 'a' && value <= 'z'
        ? static_cast<char>(value - 'a' + 'A') : value;
}

bool SameAnimation(std::string_view actual, std::string_view expected) {
    return actual.size() == expected.size() &&
        std::equal(actual.begin(), actual.end(), expected.begin(),
                   [](char a, char b) {
                       return UpperAscii(a) == UpperAscii(b);
                   });
}

void EnsureMovingAnimation(std::uint32_t address,
                           IMonsterPursuitRuntime& runtime) {
    const auto animation = address + 0x170u;
    if (SameAnimation(runtime.AnimationName(animation), "Z02") ||
        SameAnimation(runtime.AnimationName(animation), "C03"))
        return;
    if (runtime.SelectAnimation(animation, "Z02") == 0u)
        (void)runtime.SelectAnimation(animation, "C03");
}

std::uint32_t EnsureIdleAnimation(std::uint32_t address,
                                  IMonsterPursuitResetRuntime& runtime) {
    const auto animation = address + 0x170u;
    if (SameAnimation(runtime.AnimationName(animation), "Z01") ||
        SameAnimation(runtime.AnimationName(animation), "C01"))
        return 0u;
    const auto selected = runtime.SelectAnimation(animation, "C01");
    if (selected == 0u)
        return runtime.SelectAnimation(animation, "Z01");
    return selected;
}

void ClearEvents(MonsterBehaviorManagerState& state,
                 std::uint32_t address, std::uint8_t flag,
                 IMonsterPursuitResetRuntime& runtime) {
    ScriptEventProcessorState processor{};
    constexpr auto first = MonsterBehaviorManager::kProcessorOffset / 4u;
    std::copy_n(state.words.begin() + first, processor.words.size(),
                processor.words.begin());
    (void)ScriptEventProcessor::ClearEvents(
        processor, runtime.ProcessorRuntime());
    runtime.CleanupScriptObjectCollection(
        address + MonsterBehaviorManager::kScriptOffset);
    (void)ScriptEventProcessor::SetFlag24(processor, flag);
    std::copy(processor.words.begin(), processor.words.end(),
              state.words.begin() + first);
}

void UpdateIdle(MonsterBehaviorManagerState& state,
                std::uint32_t address, float elapsed,
                IMonsterPursuitRuntime& runtime) {
    if (runtime.CheckProximity(state, address, elapsed) &&
        Read(state, 0x704u) == 0u && !runtime.PlayerControlBusy()) {
        Write(state, 0x6e0u, 1u);
        Write(state, 0x6fcu, Read(state, 0x6f8u));
        // Both copy loops in 0x433b75 and 0x433bab compare a register
        // with itself before their first iteration.
        Write(state, 0x718u, Read(state, 0x714u));
        for (auto record = Read(state, 0x724u);
             record != Read(state, 0x728u); record += 16u)
            runtime.SetRecordErrorFlag(record, 1u);
        Write(state, 0x728u, Read(state, 0x724u));
        runtime.ProcessActionWithScript(address, elapsed);
        ClearEvents(state, address, 0u, runtime);
    }
    Write(state, 0x70cu, 0u);
    Write(state, 0x708u, 0u);
}

void UpdatePursuing(MonsterBehaviorManagerState& state,
                    std::uint32_t address, float elapsed,
                    IMonsterPursuitRuntime& runtime) {
    WriteFloat(state, 0x708u,
        static_cast<float>(static_cast<double>(elapsed) +
                           static_cast<double>(ReadFloat(state, 0x708u))));
    EnsureMovingAnimation(address, runtime);
    const auto behavior = Read(state, 0x5f4u);
    if (behavior == 3u || behavior == 2u ||
        Read(state, 0x704u) != 0u ||
        runtime.CheckDistanceAndFlag(state, address, elapsed)) {
        runtime.ProcessCollisionAndConfig(state, address, elapsed);
        return;
    }

    const auto current = ReadFloat(state, 0x6fcu);
    const auto limit = ReadFloat(state, 0x6f8u);
    if (current < limit || std::isunordered(current, limit)) {
        WriteFloat(state, 0x6fcu,
            static_cast<float>(static_cast<double>(elapsed) +
                               static_cast<double>(current)));
    } else {
        Write(state, 0x6fcu, 0u);
        if (!runtime.ProcessPursuitCollisionAndSpeed(
                state, address, elapsed, Read(state, 0x6e4u)))
            runtime.ProcessCollisionAndConfig(state, address, elapsed);
    }
    if (runtime.ProcessObjectInteraction(
            state, address, Read(state, 0x6e4u), elapsed)) {
        runtime.ProcessConfigAndEvents(address, elapsed);
        runtime.InitializeCombatSystem(address, -1);
    }
}

void UpdateReturning(MonsterBehaviorManagerState& state,
                     std::uint32_t address, float elapsed,
                     IMonsterPursuitRuntime& runtime) {
    const auto accumulated = static_cast<double>(elapsed) +
                             static_cast<double>(ReadFloat(state, 0x70cu));
    WriteFloat(state, 0x70cu, static_cast<float>(accumulated));
    const auto threshold = static_cast<double>(ReadFloat(state, 0x708u)) * 3.0;
    if (accumulated > threshold) {
        (void)runtime.ResetPursuit(state, address);
        return;
    }

    EnsureMovingAnimation(address, runtime);
    const auto behavior = Read(state, 0x5f4u);
    if (behavior == 3u || behavior == 2u) {
        (void)runtime.ResetPursuit(state, address);
        return;
    }
    if (!runtime.CheckTargetReached(state, address, elapsed)) return;
    if (Read(state, 0x654u) != 0u && Read(state, 0x6d0u) != 0u) {
        ClearEvents(state, address, 1u, runtime);
        runtime.LoadFuzzyBehaviorConfig(address, Read(state, 0x6d0u));
    }
    Write(state, 0x6e0u, 0u);
    Write(state, 0x708u, 0u);
    Write(state, 0x70cu, 0u);
    (void)EnsureIdleAnimation(address, runtime);
}

}  // namespace

bool MonsterBehaviorManager::UpdatePursuitState(
    MonsterBehaviorManagerState& state, std::uint32_t address,
    float elapsed, IMonsterPursuitRuntime& runtime) {
    switch (Read(state, 0x6e0u)) {
    case 0u: UpdateIdle(state, address, elapsed, runtime); break;
    case 1u: UpdatePursuing(state, address, elapsed, runtime); break;
    case 2u: UpdateReturning(state, address, elapsed, runtime); break;
    default: break;
    }
    runtime.VisitArenaObjects(address);
    return Read(state, 0x6e0u) != 0u;
}

std::uint32_t MonsterPursuitReset::Reset(
    MonsterBehaviorManagerState& state, std::uint32_t address,
    IMonsterPursuitResetRuntime& runtime) {
    std::uint32_t result = 0u;
    const auto source = Read(state, 0x654u);
    const auto child = Read(state, 0x6d0u);
    if (source != 0u && child != 0u) {
        ClearEvents(state, address, 1u, runtime);
        runtime.LoadFuzzyBehaviorConfig(address, child);
        runtime.DispatchComponentField(address, 4u, source + 0x18u, 0u);
        runtime.DispatchComponentField(address, 8u, source + 0x24u, 0u);
        result = EnsureIdleAnimation(address, runtime);
    }
    Write(state, 0x6e0u, 0u);
    Write(state, 0x708u, 0u);
    Write(state, 0x70cu, 0u);
    return result;
}

}  // namespace pal4
