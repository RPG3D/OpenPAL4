#include "pal4/normal_attack_effects.h"

#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

constexpr std::uint32_t kTask = 0x4000u;
constexpr std::uint32_t kAttacker = 0x5000u;
constexpr std::uint32_t kTarget = 0x6000u;

struct StateNotice {
    std::uint32_t actor;
    std::uint32_t code;
    std::uint32_t other;
};

struct Runtime final : pal4::INormalAttackEffectRuntime {
    std::map<std::uint32_t, std::uint32_t> words{{kTask + 60u, kAttacker}};
    std::map<std::uint32_t, std::int32_t> values{{kTarget, 10},
                                                  {kAttacker, 20}};
    std::vector<std::string> calls;
    std::vector<StateNotice> notices;
    pal4::NormalAttackDamage damage{-3.0f, 2.0f};
    pal4::NormalAttackDamageKind calculated_kind =
        pal4::NormalAttackDamageKind::Physical;
    pal4::NormalAttackDamageKind notified_kind =
        pal4::NormalAttackDamageKind::Physical;
    bool ready = true;
    bool property = true;
    bool block_reflection = false;
    std::uint32_t target_state = 0;
    std::uint32_t wounded = 0;
    std::uint32_t unchanged = 0;
    std::uint32_t fend = 0;
    std::uint32_t miss = 0;
    float transferred_scale = 0;

    std::uint32_t ReadWord(const std::uint32_t address) override {
        Check(address == kTask + 60u, "attacker task offset");
        calls.emplace_back("attacker");
        return words[address];
    }
    std::uint8_t ReadByte(const std::uint32_t address) override {
        Check(address == kTask + 73u, "damage mode task offset");
        calls.emplace_back("damage_mode");
        return 1u;
    }
    float ReadFloat(const std::uint32_t address) override {
        Check(address == kTask + 68u, "scaled metric task offset");
        calls.emplace_back("scale");
        return 0.5f;
    }
    std::uint8_t ActorReady(const std::uint32_t actor) override {
        Check(actor == kTarget || actor == kAttacker + 1u,
              "readiness target");
        calls.emplace_back("ready");
        return ready ? 1u : 0u;
    }
    pal4::NormalAttackDamage CalculateDamage(
        const std::uint32_t attacker, const std::uint32_t target,
        const std::uint8_t mode,
        const pal4::NormalAttackDamageKind kind) override {
        Check(attacker == kAttacker && target == kTarget && mode == 1u,
              "damage calculation arguments");
        calculated_kind = kind;
        calls.emplace_back("calculate");
        return damage;
    }
    void NotifyAttacker(const std::uint32_t attacker,
                        const std::uint32_t target,
                        const pal4::NormalAttackDamageKind kind) override {
        Check(attacker == kAttacker && target == kTarget,
              "attacker notification arguments");
        notified_kind = kind;
        calls.emplace_back("notify_attacker");
    }
    void NotifyTarget(const std::uint32_t target,
                      const std::uint32_t attacker) override {
        Check(target == kTarget && attacker == kAttacker,
              "target notification arguments");
        calls.emplace_back("notify_target");
    }
    std::int32_t Value7(const std::uint32_t actor) override {
        calls.emplace_back("value7");
        return values[actor];
    }
    bool HasProperty12(const std::uint32_t actor) override {
        Check(actor == kTarget, "property source target");
        calls.emplace_back("property12");
        return property;
    }
    std::int32_t FloatToInt(const float value) override {
        calls.emplace_back("convert");
        return static_cast<std::int32_t>(value);
    }
    void AddValue7(const std::uint32_t actor,
                   const std::int32_t value) override {
        calls.emplace_back("add7");
        values[actor] += value;
    }
    bool BlocksReflectedDamage(const std::uint32_t attacker) override {
        Check(attacker == kAttacker, "reflection attacker");
        calls.emplace_back("block_reflection");
        return block_reflection;
    }
    void TransferScaledMetric(const std::uint32_t attacker,
                              const std::uint32_t target,
                              const float scale) override {
        Check(attacker == kAttacker && target == kTarget,
              "scaled metric participants");
        transferred_scale = scale;
        calls.emplace_back("transfer");
    }
    void NotifyCombatState(const std::uint32_t actor,
                           const std::uint32_t code,
                           const std::uint32_t other) override {
        notices.push_back({actor, code, other});
        calls.emplace_back("state");
    }
    std::uint8_t CombatWounded(const std::uint32_t target,
                               const std::uint32_t attacker) override {
        Check(target == kTarget && attacker == kAttacker,
              "wounded event participants");
        ++wounded;
        calls.emplace_back("wounded");
        return 0x45u;
    }
    std::uint8_t UnchangedTarget(const std::uint32_t target,
                                 const std::uint32_t attacker) override {
        Check(target == kTarget && attacker == kAttacker,
              "unchanged event participants");
        ++unchanged;
        calls.emplace_back("unchanged");
        return 0x46u;
    }
    std::uint32_t TargetState(const std::uint32_t target) override {
        Check(target == kTarget, "item target state");
        calls.emplace_back("target_state");
        return target_state;
    }
    std::uint8_t RegisterFendOff(const std::uint32_t attacker,
                                 const std::uint32_t target) override {
        Check(attacker == kAttacker && target == kTarget,
              "fend-off event participants");
        ++fend;
        calls.emplace_back("fend");
        return 0x47u;
    }
    std::uint8_t RegisterMiss(const std::uint32_t attacker,
                              const std::uint32_t target) override {
        Check(attacker == kAttacker && target == kTarget,
              "miss event participants");
        ++miss;
        calls.emplace_back("miss");
        return 0x48u;
    }
};

void ReadinessAndSelfGates() {
    Runtime runtime;
    runtime.ready = false;
    Check(pal4::NormalAttackEffects::Attack(kTask, kTarget, runtime) == 0u &&
          runtime.calls == std::vector<std::string>{"ready"},
          "physical damage rejects unready target");
    runtime.calls.clear();
    Check(pal4::NormalAttackEffects::Magic(kTask, kTarget, runtime) == 0u &&
          runtime.calls == std::vector<std::string>{"ready"},
          "magic damage rejects unready target");
    runtime.calls.clear();
    Check(pal4::NormalAttackEffects::Item(kTask, kTarget, runtime) == 0u &&
          runtime.calls == std::vector<std::string>{"ready"},
          "item rejects unready target");
    runtime.calls.clear();
    Check(pal4::NormalAttackEffects::Support(kTask, kTarget, runtime) == 0u &&
          runtime.calls == std::vector<std::string>{"ready"},
          "support rejects unready target");

    runtime.ready = true;
    runtime.words[kTask + 60u] = kAttacker + 1u;
    runtime.calls.clear();
    Check(pal4::NormalAttackEffects::Attack(
              kTask, kAttacker + 1u, runtime) == 1u &&
          runtime.calls == std::vector<std::string>{"ready", "attacker"},
          "physical self target returns low pointer byte");
    runtime.words[kTask + 60u] = kAttacker;
}

void DamageAndEvents() {
    Runtime physical;
    Check(pal4::NormalAttackEffects::Attack(kTask, kTarget, physical) ==
              0x45u &&
          physical.calculated_kind ==
              pal4::NormalAttackDamageKind::Physical &&
          physical.notified_kind ==
              pal4::NormalAttackDamageKind::Physical &&
          physical.values[kTarget] == 7 &&
          physical.values[kAttacker] == 22 &&
          physical.transferred_scale == 0.5f &&
          physical.wounded == 1u && physical.unchanged == 0u &&
          physical.notices.size() == 4u &&
          physical.notices[0].actor == kAttacker &&
          physical.notices[0].code == 4u &&
          physical.notices[0].other == kTarget &&
          physical.notices[1].code == 13u &&
          physical.notices[1].other == kAttacker &&
          physical.notices[2].actor == kTarget &&
          physical.notices[2].code == 3u &&
          physical.notices[3].code == 12u &&
          physical.notices[3].other == kAttacker,
          "physical damage updates values, state notices and wounded event");

    Runtime magic;
    magic.property = false;
    Check(pal4::NormalAttackEffects::Magic(kTask, kTarget, magic) == 0x46u &&
          magic.calculated_kind == pal4::NormalAttackDamageKind::Magic &&
          magic.notified_kind == pal4::NormalAttackDamageKind::Magic &&
          magic.values[kTarget] == 10 && magic.values[kAttacker] == 20 &&
          magic.unchanged == 1u && magic.wounded == 0u,
          "magic uses extended calculation and unchanged event");

    Runtime blocked;
    blocked.block_reflection = true;
    (void)pal4::NormalAttackEffects::Attack(kTask, kTarget, blocked);
    Check(blocked.values[kTarget] == 7 && blocked.values[kAttacker] == 20,
          "attacker property blocks reflected value update");

    Runtime increased;
    increased.damage = {2.0f, 0.0f};
    Check(pal4::NormalAttackEffects::Attack(kTask, kTarget, increased) ==
              12u && increased.wounded == 0u && increased.unchanged == 0u,
          "increased target value returns low byte of final stat");
}

void ItemAndSupport() {
    Runtime runtime;
    runtime.target_state = 4u;
    Check(pal4::NormalAttackEffects::Item(kTask, kTarget, runtime) == 4u &&
          runtime.values[kTarget] == 10 && runtime.fend == 0u,
          "item leaves state-four target unchanged");
    runtime.target_state = 2u;
    Check(pal4::NormalAttackEffects::Item(kTask, kTarget, runtime) == 0x47u &&
          runtime.values[kTarget] == 9 && runtime.fend == 1u,
          "item decrements value seven and registers fend-off event");
    Check(pal4::NormalAttackEffects::Support(kTask, kTarget, runtime) ==
              0x48u && runtime.miss == 1u,
          "support registers miss event");
}

}  // namespace

int main() {
    try {
        ReadinessAndSelfGates();
        DamageAndEvents();
        ItemAndSupport();
        std::cout << "normal_attack_effects_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
