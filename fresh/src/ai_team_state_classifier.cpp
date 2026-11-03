#include "pal4/ai_action_task.h"

namespace pal4 {
namespace {

constexpr std::string_view kSource = "cbActorAi::getTeamState";
constexpr std::string_view kSelfDanger =
    "return EATS_SELF_IN_DANGER, HP:%d, MaxHp:%d, hpRate:%f, selfInDangerThreshold:%f";
constexpr std::string_view kSelfWounded =
    "return EATS_SELF_WOUNDED, Hp:%d, MaxHp:%d, HpRate:%f, MinSelfHpThreshold:%f MaxSelfHpThreshold:%f";
constexpr std::string_view kTeamSlaughtered =
    "return EATS_TEAMMATE_SLAUGHTERED, DeadMate:%d, TeammateNumber:%d, DeadRate:%f DeadRateThreshold:%f";
constexpr std::string_view kTeamDanger =
    "return EATS_TEAMMATE_IN_DANGER, SatisfiedActor:%s, Hp:%d, MaxHp:%d HpRate:%f mateInDangerThreshold:%f";
constexpr std::string_view kTeamWounded =
    "return EATS_TEAMMATE_WOUNDED, SatisfiedActor:%s, Hp:%d, MaxHp:%d HpRate:%f MaxMateHpThreshold:%f MinMateHpThreshold:%f";
constexpr std::string_view kNormal = "return EATS_NORMAL";

struct IteratorOwner {
    IAiActorStateRuntime& runtime;
    std::uint32_t iterator;
    ~IteratorOwner() {
        if (iterator != 0u) runtime.ReleaseActorIterator(iterator);
    }
};

void LogActorState(const std::uint32_t task,
                   const std::uint32_t actor,
                   const AiTeamState state,
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

AiTeamState AiTeamStateClassifier::Classify(
    const std::uint32_t task, const std::uint32_t record,
    IAiActorStateRuntime& runtime) {
    if (record == 0u) {
        runtime.ReportMissingRecord();
        return AiTeamState::MissingRecord;
    }

    const auto actor = runtime.ReadWord(task + 4u);
    const auto turn = runtime.TurnCount(actor);
    if (turn == runtime.ConvertToInt(runtime.ReadFloat(record + 48u)))
        return AiTeamState::TurnMatch;
    const auto interval = runtime.ConvertToInt(
        runtime.ReadFloat(record + 52u));
    if (interval > 0 && turn % interval == 0)
        return AiTeamState::TurnInterval;

    const auto hp = runtime.CurrentStat(actor, 7u);
    const auto max_hp = runtime.MaxStat(actor, 0u);
    const auto self_rate = static_cast<float>(
        static_cast<double>(hp) / static_cast<double>(max_hp));
    const auto self_danger = runtime.ReadFloat(record + 4u);
    if (!(self_rate > self_danger)) {
        LogActorState(task, actor, AiTeamState::SelfInDanger,
                      kSelfDanger, self_rate, self_danger, 0.0f,
                      runtime);
        return AiTeamState::SelfInDanger;
    }

    const auto self_min = runtime.ReadFloat(record + 8u);
    if (self_rate >= self_min) {
        const auto self_max = runtime.ReadFloat(record + 12u);
        if (!(self_rate > self_max)) {
            LogActorState(task, actor, AiTeamState::SelfWounded,
                          kSelfWounded, self_rate, self_min,
                          self_max, runtime);
            return AiTeamState::SelfWounded;
        }
    }

    const IteratorOwner team{runtime, runtime.CreateActorIterator(
        actor, AiActorGroup::Team)};
    const auto count = runtime.ActorCount(team.iterator);
    float dead_count = 0.0f;
    while (!runtime.ActorEnd(team.iterator)) {
        if (runtime.IsDead(runtime.ActorCurrent(team.iterator)))
            dead_count += 1.0f;
        runtime.ActorNext(team.iterator);
    }
    const auto dead_count_integer = runtime.ConvertToInt(dead_count);
    const auto dead_rate = static_cast<double>(dead_count) /
                           static_cast<double>(count);
    const auto dead_threshold = runtime.ReadFloat(record + 20u);
    if (dead_rate >= dead_threshold) {
        AiActorStateLog details;
        details.state = static_cast<std::uint32_t>(
            AiTeamState::TeammateSlaughtered);
        details.dead_count = static_cast<std::uint32_t>(dead_count_integer);
        details.team_count = count;
        details.dead_rate = dead_rate;
        details.lower_threshold = dead_threshold;
        runtime.LogState(task, kSource, kTeamSlaughtered, details);
        return AiTeamState::TeammateSlaughtered;
    }

    runtime.ActorReset(team.iterator);
    while (!runtime.ActorEnd(team.iterator)) {
        const auto member = runtime.ActorCurrent(team.iterator);
        const auto member_hp = runtime.CurrentStat(member, 7u);
        const auto member_max = runtime.MaxStat(member, 0u);
        const auto rate = static_cast<double>(
            static_cast<float>(member_hp)) /
            static_cast<double>(member_max);
        const auto danger_threshold = runtime.ReadFloat(record + 16u);
        if (!(rate > danger_threshold)) {
            LogActorState(task, member,
                          AiTeamState::TeammateInDanger,
                          kTeamDanger, rate, danger_threshold,
                          0.0f, runtime);
            return AiTeamState::TeammateInDanger;
        }
        runtime.ActorNext(team.iterator);
    }

    runtime.ActorReset(team.iterator);
    while (!runtime.ActorEnd(team.iterator)) {
        const auto member = runtime.ActorCurrent(team.iterator);
        const auto member_hp = runtime.CurrentStat(member, 7u);
        const auto member_max = runtime.MaxStat(member, 0u);
        const auto rate = static_cast<float>(
            static_cast<double>(member_hp) /
            static_cast<double>(member_max));
        const auto max_threshold = runtime.ReadFloat(record + 28u);
        if (!(rate > max_threshold)) {
            const auto min_threshold = runtime.ReadFloat(record + 24u);
            if (rate >= min_threshold) {
                LogActorState(task, member,
                              AiTeamState::TeammateWounded,
                              kTeamWounded, rate, min_threshold,
                              max_threshold, runtime);
                return AiTeamState::TeammateWounded;
            }
        }
        runtime.ActorNext(team.iterator);
    }

    runtime.LogState(task, kSource, kNormal,
                     AiActorStateLog{static_cast<std::uint32_t>(
                         AiTeamState::Normal)});
    return AiTeamState::Normal;
}

}  // namespace pal4
