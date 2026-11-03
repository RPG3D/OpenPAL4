#include "pal4/combat_ending_score_updater.h"

#include <bit>
#include <string>

namespace pal4 {
namespace {

class ActorIterator final {
public:
    ActorIterator(ICombatEndingScoreRuntime& runtime, std::uint32_t handle)
        : runtime_(runtime), handle_(handle) {}
    ~ActorIterator() {
        if (handle_ != 0u) runtime_.ReleaseActors(handle_);
    }
    ActorIterator(const ActorIterator&) = delete;
    ActorIterator& operator=(const ActorIterator&) = delete;
    [[nodiscard]] std::uint32_t Handle() const { return handle_; }

private:
    ICombatEndingScoreRuntime& runtime_;
    std::uint32_t handle_;
};

[[nodiscard]] float ReadFloat(ICombatEndingScoreRuntime& runtime,
                              std::uint32_t address) {
    return std::bit_cast<float>(runtime.ReadWord(address));
}

void SetNumber(ICombatEndingScoreRuntime& runtime, std::uint32_t window,
               std::int32_t number) {
    runtime.SetText(window, runtime.ToUtf8(std::to_string(number)));
}

void SetPair(ICombatEndingScoreRuntime& runtime, std::uint32_t window,
             std::int32_t current, std::int32_t maximum) {
    runtime.SetText(window, runtime.ToUtf8(std::to_string(current) + "/" +
                                          std::to_string(maximum)));
}

void UpdateActorRow(std::uint32_t dialog, std::uint32_t row,
                    std::uint32_t actor, std::uint32_t slot,
                    ICombatEndingScoreRuntime& runtime) {
    const auto record = dialog + 492u + 40u * slot;
    runtime.WriteWord(row + 72u, runtime.ReadWord(record));
    const auto panel = runtime.ReadWord(row);
    runtime.SetVisible(panel, true);
    runtime.SetYPosition(panel, -runtime.GetHeight(panel));
    runtime.SetImage(runtime.ReadWord(row + 40u),
                     runtime.ReadWord(dialog + 632u + 4u * slot));
    runtime.SetImage(runtime.ReadWord(row + 44u),
                     runtime.ReadWord(dialog + 648u + 4u * slot));
    SetNumber(runtime, runtime.ReadWord(row + 4u),
              static_cast<std::int32_t>(runtime.ReadWord(record)));
    runtime.SetText(runtime.ReadWord(row + 20u), runtime.ToUtf8("--"));
    runtime.SetText(runtime.ReadWord(row + 24u), runtime.ToUtf8("--"));

    const auto max_hp = runtime.ActorBaseStat(actor, 0u);
    const auto hp = runtime.ActorCurrentStat(actor, 7u);
    SetPair(runtime, runtime.ReadWord(row + 8u), hp, max_hp);
    const auto max_mp = runtime.ActorBaseStat(actor, 2u);
    const auto mp = runtime.ActorCurrentStat(actor, 9u);
    SetPair(runtime, runtime.ReadWord(row + 12u), mp, max_mp);
    // Getter52 pops only index8; the earlier push100 is the second %d.
    SetPair(runtime, runtime.ReadWord(row + 16u),
            runtime.ActorCurrentStat(actor, 8u), 100);

    const auto hp_width = static_cast<float>(
        static_cast<double>(runtime.ActorCurrentStat(actor, 7u)) *
        ReadFloat(runtime, row + 48u) /
        static_cast<double>(runtime.ActorBaseStat(actor, 0u)));
    runtime.SetWidth(runtime.ReadWord(row + 28u), hp_width);
    const auto mp_width = static_cast<float>(
        static_cast<double>(runtime.ActorCurrentStat(actor, 9u)) *
        ReadFloat(runtime, row + 52u) /
        static_cast<double>(runtime.ActorBaseStat(actor, 2u)));
    runtime.SetWidth(runtime.ReadWord(row + 32u), mp_width);
    const auto rage_width = static_cast<float>(
        static_cast<double>(runtime.ActorCurrentStat(actor, 8u)) *
        ReadFloat(runtime, row + 56u) * 0.0099999998);
    runtime.SetWidth(runtime.ReadWord(row + 36u), rage_width);
}

}  // namespace

int CombatEndingScoreUpdater::Update(const std::uint32_t dialog,
                                     const std::uint32_t result,
                                     ICombatEndingScoreRuntime& runtime) {
    for (std::uint32_t i = 0u; i < 5u; ++i)
        runtime.WriteWord(dialog + 420u + 4u * i,
                          runtime.ReadWord(result + 4u * i));
    runtime.CopyTreeData(dialog + 440u, result + 20u);
    runtime.CopyTreeData(dialog + 456u, result + 36u);
    for (std::uint32_t i = 0u; i < 4u; ++i)
        runtime.CopyPlayerData(dialog + 472u + 40u * i,
                               result + 52u + 40u * i);
    for (std::uint32_t i = 0u; i < 4u; ++i)
        runtime.WriteWord(dialog + 632u + 4u * i, runtime.IconImage(i));

    runtime.WriteWord(dialog + 716u, 0u);
    const auto score = runtime.ReadWord(dialog + 428u);
    if (score <= 4u) {
        runtime.SetImage(runtime.ReadWord(dialog + 64u),
                         runtime.ReadWord(dialog + 664u + 4u * score));
        runtime.SetText(runtime.ReadWord(dialog + 68u),
                        runtime.ToUtf8(runtime.GameString(0xc3u + score)));
    }

    {
        ActorIterator iterator(runtime, runtime.BeginActors());
        auto row = dialog + 116u;
        while (!runtime.ActorsDone(iterator.Handle())) {
            const auto actor = runtime.CurrentActor(iterator.Handle());
            const auto slot = runtime.ReadWord(actor + 0x4a8u);
            if (slot < 4u) {
                UpdateActorRow(dialog, row, actor, slot, runtime);
                row += 76u;
            }
            runtime.AdvanceActor(iterator.Handle());
        }

        const auto money_format = runtime.GameString(0xc2u);
        const auto money = static_cast<std::int32_t>(runtime.ReadWord(dialog + 420u));
        runtime.SetText(runtime.ReadWord(dialog + 92u),
                        runtime.ToUtf8(runtime.FormatMoney(money_format, money)));
        if (runtime.ReadWord(dialog + 452u) != 0u)
            runtime.UpdateScoreTextPart2(dialog);
        if (runtime.ReadWord(dialog + 468u) != 0u)
            runtime.UpdateScoreText(dialog);
        runtime.ShowEndTaskScore(dialog);
    }
    return runtime.CleanupTextBuffer();
}

}  // namespace pal4
