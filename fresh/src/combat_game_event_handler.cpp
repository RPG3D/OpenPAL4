#include "pal4/combat_game_event_handler.h"

namespace pal4 {

namespace {

std::uint32_t FindDatabaseRecord(std::uint32_t collection,
                                 std::int32_t key,
                                 ICombatGameStatsRuntime& runtime) {
    auto begin = runtime.ReadWord(collection + 8u);
    const auto end = runtime.ReadWord(collection + 12u);
    auto count = (end - begin) / 8u;
    while (count != 0u) {
        const auto half = count / 2u;
        const auto candidate = begin + half * 8u;
        if (static_cast<std::int32_t>(runtime.ReadWord(candidate)) < key) {
            begin = candidate + 8u;
            count -= half + 1u;
        } else {
            count = half;
        }
    }
    if (begin == end ||
        static_cast<std::int32_t>(runtime.ReadWord(begin)) > key)
        return 0u;
    return runtime.ReadWord(collection + 0x60u) +
           (runtime.ReadWord(begin + 4u) << 7u);
}

}  // namespace

std::uint32_t ICombatGameStatsRuntime::FindStatRecord(std::int32_t key) {
    return FindDatabaseRecord(ReadWord(GetGameDatabase()+0x30u),key,*this);
}

void CombatGameStats::Initialize(
    std::uint32_t object, ICombatGameStatsRuntime& runtime) {
    for (std::uint32_t index = 0u; index < 26u; ++index)
        runtime.WriteWord(object + 0xf0u + index * 4u, 0u);
    for (std::uint32_t index = 0u; index < 26u; ++index)
        runtime.WriteWord(object + 0x88u + index * 4u, 0u);
    for (std::uint32_t index = 0u; index < 12u; ++index)
        runtime.WriteWord(object + 0x228u + index * 4u, 0u);

    const auto first_record = runtime.FindStatRecord(0);
    const auto second_record = runtime.FindStatRecord(1);
    if (first_record == 0u || second_record == 0u) return;

    for (std::uint32_t index = 0u; index < 26u; ++index) {
        runtime.WriteWord(object + 0x1c0u + index * 4u,
                          runtime.ReadWord(first_record + 0x18u + index * 4u));
        runtime.WriteWord(object + 0x158u + index * 4u,
                          runtime.ReadWord(second_record + 0x18u + index * 4u));
    }
    for (std::uint32_t index = 0u; index < 5u; ++index)
        runtime.WriteWord(object + 0x258u + index * 4u,
                          runtime.ReadWord(first_record + 4u + index * 4u));
}

std::uint32_t CombatGameStats::CalculateTotal(
    std::uint32_t object, ICombatGameStatsRuntime& runtime) {
    std::uint32_t total = 0u;
    for (std::uint32_t index = 0u; index < 26u; ++index)
        total += runtime.ReadWord(object + 0x88u + index * 4u);
    return total + runtime.ReadWord(object + 0x258u);
}

std::uint32_t CombatGameStats::GetBattleRank(
    std::uint32_t object, std::int32_t value,
    ICombatGameStatsRuntime& runtime) {
    const auto combat_system = runtime.GetCombatSystem();
    if (runtime.IsCombatSystemInactive(combat_system)) return 4u;
    if (value > static_cast<std::int32_t>(
                    runtime.ReadWord(object + 0x25cu))) return 0u;
    if (value > static_cast<std::int32_t>(
                    runtime.ReadWord(object + 0x260u))) return 1u;
    if (value > static_cast<std::int32_t>(
                    runtime.ReadWord(object + 0x264u))) return 2u;
    return value <= static_cast<std::int32_t>(
                        runtime.ReadWord(object + 0x268u)) ? 4u : 3u;
}

std::uint32_t CombatGameStats::Update(
    std::uint32_t object, std::uint32_t stat,
    std::uint32_t increment,
    ICombatGameStatUpdateRuntime& runtime) {
    const auto delta = increment *
        runtime.ReadWord(object + 0x1c0u + stat * 4u);
    const auto score_slot = object + 0x88u + stat * 4u;
    const auto count_slot = object + 0xf0u + stat * 4u;
    const auto score = runtime.ReadWord(score_slot) + delta;
    const auto count = runtime.ReadWord(count_slot) + 1u;
    runtime.WriteWord(score_slot, score);
    runtime.WriteWord(count_slot, count);
    const auto limit = runtime.ReadWord(object + 0x158u + stat * 4u);
    const auto signed_score = static_cast<std::int32_t>(score);
    const auto signed_limit = static_cast<std::int32_t>(limit);
    if ((signed_score > 0 && signed_score > signed_limit) ||
        (signed_score <= 0 && signed_score < signed_limit))
        runtime.WriteWord(score_slot, limit);
    const auto router = runtime.GetScriptEventRouter();
    return runtime.ProcessScoreChanged(
        router, 0x8b8b50u, delta, 1u);
}

std::uint32_t CombatGameEventHandler::Construct(
    std::uint32_t object, ICombatGameEventHandlerRuntime& runtime) {
    for (std::uint32_t index = 0u;
         index < CombatScriptEventHandlers::kHandlerCount * 2u; ++index)
        runtime.WriteWord(object + index * 4u, 0u);
    CombatGameStats::Initialize(object, runtime);
    (void)CombatScriptEventHandlers::Initialize(object, runtime);
    return object;
}

}  // namespace pal4
