#include "pal4/combat_game_event_iterations.h"

namespace pal4 {

namespace {

class IteratorOwner final {
public:
    IteratorOwner(std::uint32_t iterator,
                  ICombatGameEventIterationRuntime& runtime)
        : iterator_(iterator), runtime_(runtime) {}

    ~IteratorOwner() {
        if (iterator_ != 0u) {
            try {
                (void)runtime_.DestroyIterator(iterator_, 1u);
            } catch (...) {
            }
        }
    }

    [[nodiscard]] std::uint32_t Get() const { return iterator_; }

    [[nodiscard]] std::uint32_t Close() {
        const auto iterator = iterator_;
        iterator_ = 0u;
        return runtime_.DestroyIterator(iterator, 1u);
    }

private:
    std::uint32_t iterator_;
    ICombatGameEventIterationRuntime& runtime_;
};

}  // namespace

std::uint32_t CombatGameEventIterations::OnMonsterFlee(
    std::uint32_t object, ICombatGameEventIterationRuntime& runtime) {
    const auto manager = runtime.GetActorManager();
    IteratorOwner iterator(runtime.CreateIterator(manager + 32u), runtime);
    auto result = runtime.IteratorDone(iterator.Get());
    while (static_cast<std::uint8_t>(result) == 0u) {
        const auto actor = runtime.IteratorCurrent(iterator.Get());
        if (runtime.MonsterState(actor) == 2u)
            (void)runtime.UpdateStat(object, 14u, 1u);
        runtime.IteratorAdvance(iterator.Get());
        result = runtime.IteratorDone(iterator.Get());
    }
    return iterator.Get() != 0u ? iterator.Close() : result;
}

std::uint32_t CombatGameEventIterations::OnBattleEnd(
    std::uint32_t object, ICombatGameEventIterationRuntime& runtime) {
    for (std::uint32_t index = 0u; index < 4u; ++index)
        if (runtime.ReadWord(object + 0x248u + index * 4u) == 0u)
            (void)runtime.UpdateStat(object, 12u, 1u);

    {
        const auto manager = runtime.GetActorManager();
        IteratorOwner iterator(runtime.CreateIterator(manager + 32u), runtime);
        while (static_cast<std::uint8_t>(
                   runtime.IteratorDone(iterator.Get())) == 0u) {
            const auto actor = runtime.IteratorCurrent(iterator.Get());
            if (runtime.ActorProperty52(actor, 7u) ==
                runtime.ActorProperty48(actor, 0u))
                (void)runtime.UpdateStat(object, 13u, 1u);
            (void)runtime.UpdateStat(object, 15u,
                                     runtime.ActorMetric252(actor));
            runtime.IteratorAdvance(iterator.Get());
        }
        if (iterator.Get() != 0u) (void)iterator.Close();
    }

    {
        const auto manager = runtime.GetActorManager();
        IteratorOwner iterator(runtime.CreateIterator(manager + 32u), runtime);
        auto done = runtime.IteratorDone(iterator.Get());
        while (static_cast<std::uint8_t>(done) == 0u) {
            const auto actor = runtime.IteratorCurrent(iterator.Get());
            if (runtime.ActorMetric252(actor) != 0u) break;
            runtime.IteratorAdvance(iterator.Get());
            done = runtime.IteratorDone(iterator.Get());
        }
        if (static_cast<std::uint8_t>(done) != 0u)
            (void)runtime.UpdateStat(object, 16u, 1u);
        if (iterator.Get() != 0u) (void)iterator.Close();
    }

    const auto first_manager = runtime.GetActorManager();
    const auto first_average = runtime.AverageProperty4(first_manager);
    const auto second_manager = runtime.GetActorManager();
    const auto second_average = runtime.AverageProperty4(second_manager + 32u);
    if (first_average < second_average)
        return runtime.UpdateStat(
            object, 19u,
            static_cast<std::uint32_t>(second_average) -
                static_cast<std::uint32_t>(first_average));
    if (first_average > second_average)
        return runtime.UpdateStat(
            object, 20u,
            static_cast<std::uint32_t>(first_average) -
                static_cast<std::uint32_t>(second_average));
    return static_cast<std::uint32_t>(second_average);
}

}  // namespace pal4
