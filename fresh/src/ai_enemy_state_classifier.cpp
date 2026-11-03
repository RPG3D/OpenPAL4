#include "pal4/ai_action_task.h"

namespace pal4 {
namespace {

constexpr std::string_view kSource = "cbActorAi::getEnemyState";
constexpr std::string_view kSlaughtered =
    "return EAES_ENEMY_SLAUGHTERED, Dead:%d, EnemyNumber:%d, DeadRate:%f, EnemyDeadThreshold:%f";
constexpr std::string_view kInDanger =
    "return EAES_ENEMY_IN_DANGER, satisfiedActor:%s Hp:%d, MaxHp:%d, HpRate:%f, EnemyInDangerThreshold:%f";
constexpr std::string_view kWounded =
    "return EAES_ENEMY_WOUNDED, satisfiedActor:%s Hp:%d, MaxHp:%d, HpRate:%f, MaxThreshold:%f, MinThreshold:%f";
constexpr std::string_view kNormal = "return EAES_NORMAL";

struct IteratorOwner {
    IAiActorStateRuntime& runtime;
    std::uint32_t iterator;
    ~IteratorOwner() {
        if (iterator != 0u) runtime.ReleaseActorIterator(iterator);
    }
};

void LogEnemy(const std::uint32_t task,
              const std::uint32_t actor,
              const AiEnemyState state,
              const std::string_view message,
              const double rate,
              const float lower,
              const float upper,
              IAiActorStateRuntime& runtime) {
    AiActorStateLog details;
    details.state = static_cast<std::uint32_t>(state);
    details.actor = actor;
    details.hp = runtime.CurrentStat(actor, 7u);
    details.max_hp = runtime.MaxStat(actor, 0u);
    details.hp_rate = rate;
    details.lower_threshold = lower;
    details.upper_threshold = upper;
    runtime.LogState(task, kSource, message, details);
}

}  // namespace

AiEnemyState AiEnemyStateClassifier::Classify(
    const std::uint32_t task, const std::uint32_t record,
    IAiActorStateRuntime& runtime) {
    if (record == 0u) {
        runtime.ReportMissingRecord();
        return AiEnemyState::MissingRecord;
    }

    const auto actor = runtime.ReadWord(task + 4u);
    const IteratorOwner enemies{runtime, runtime.CreateActorIterator(
        actor, AiActorGroup::Enemy)};
    const auto count = runtime.ActorCount(enemies.iterator);
    float dead_count = 0.0f;
    while (!runtime.ActorEnd(enemies.iterator)) {
        if (runtime.IsDead(runtime.ActorCurrent(enemies.iterator)))
            dead_count += 1.0f;
        runtime.ActorNext(enemies.iterator);
    }
    const auto dead_count_integer = runtime.ConvertToInt(dead_count);
    const auto dead_rate = static_cast<double>(dead_count) /
                           static_cast<double>(count);
    const auto dead_threshold = runtime.ReadFloat(record + 32u);
    if (dead_rate >= dead_threshold) {
        AiActorStateLog details;
        details.state = static_cast<std::uint32_t>(
            AiEnemyState::Slaughtered);
        details.dead_count = static_cast<std::uint32_t>(dead_count_integer);
        details.team_count = count;
        details.dead_rate = dead_rate;
        details.lower_threshold = dead_threshold;
        runtime.LogState(task, kSource, kSlaughtered, details);
        return AiEnemyState::Slaughtered;
    }

    runtime.ActorReset(enemies.iterator);
    while (!runtime.ActorEnd(enemies.iterator)) {
        const auto enemy = runtime.ActorCurrent(enemies.iterator);
        const auto hp = runtime.CurrentStat(enemy, 7u);
        const auto max_hp = runtime.MaxStat(enemy, 0u);
        const auto rate = static_cast<double>(static_cast<float>(hp)) /
                          static_cast<double>(max_hp);
        const auto threshold = runtime.ReadFloat(record + 36u);
        if (!(rate > threshold)) {
            LogEnemy(task, enemy, AiEnemyState::InDanger,
                     kInDanger, rate, threshold, 0.0f, runtime);
            return AiEnemyState::InDanger;
        }
        runtime.ActorNext(enemies.iterator);
    }

    runtime.ActorReset(enemies.iterator);
    while (!runtime.ActorEnd(enemies.iterator)) {
        const auto enemy = runtime.ActorCurrent(enemies.iterator);
        const auto hp = runtime.CurrentStat(enemy, 7u);
        const auto max_hp = runtime.MaxStat(enemy, 0u);
        const auto rate = static_cast<float>(
            static_cast<double>(hp) / static_cast<double>(max_hp));
        const auto min_threshold = runtime.ReadFloat(record + 40u);
        if (rate >= min_threshold) {
            const auto max_threshold = runtime.ReadFloat(record + 44u);
            if (!(rate > max_threshold)) {
                LogEnemy(task, enemy, AiEnemyState::Wounded,
                         kWounded, rate, min_threshold,
                         max_threshold, runtime);
                return AiEnemyState::Wounded;
            }
        }
        runtime.ActorNext(enemies.iterator);
    }

    runtime.LogState(task, kSource, kNormal,
                     AiActorStateLog{static_cast<std::uint32_t>(
                         AiEnemyState::Normal)});
    return AiEnemyState::Normal;
}

}  // namespace pal4
