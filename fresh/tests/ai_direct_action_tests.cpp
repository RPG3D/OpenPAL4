#include "pal4/ai_action_task.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

constexpr std::uint32_t kTask = 0x4000u;
constexpr std::uint32_t kActor = 0x5000u;
constexpr std::uint32_t kRequested = 0x6000u;
constexpr std::uint32_t kTarget = 0x7000u;

struct Runtime final : pal4::IAiDirectActionRuntime {
    bool can_attack = true;
    bool can_flee = true;
    bool throw_build = false;
    bool throw_select = false;
    std::uint32_t target = kTarget;
    std::uint32_t actor_targets = 0x8000u;
    std::uint32_t pool = 0x9000u;
    std::vector<std::string> calls;

    std::uint32_t ReadWord(std::uint32_t address) override {
        Check(address == kTask + 4u, "actor lives at task +4");
        calls.emplace_back("actor");
        return kActor;
    }
    bool ActorCapability(std::uint32_t actor,
                         std::uint32_t code) override {
        Check(actor == kActor && (code == 0u || code == 5u),
              "capability uses actor and action code");
        calls.emplace_back(code == 0u ? "attack-cap" : "flee-cap");
        return code == 0u ? can_attack : can_flee;
    }
    std::uint32_t AcquireActorTargets(std::uint32_t actor) override {
        Check(actor == kActor, "target list uses actor");
        calls.emplace_back("acquire");
        return actor_targets;
    }
    std::uint32_t BuildTargetPool(
        std::uint32_t task, std::uint32_t targets) override {
        Check(task == kTask && targets == actor_targets,
              "pool builder gets task and actor targets");
        calls.emplace_back("build");
        if (throw_build) throw std::runtime_error("build failure");
        return pool;
    }
    void ReleaseActorTargets(std::uint32_t targets) override {
        Check(targets == actor_targets && targets != 0u,
              "releases only nonnull actor target list");
        calls.emplace_back("release-targets");
    }
    std::uint32_t SelectCombatTarget(
        std::uint32_t task, std::uint32_t selected_pool,
        std::uint32_t requested) override {
        Check(task == kTask && selected_pool == pool &&
                  requested == kRequested,
              "selector receives task, pool and requested target");
        calls.emplace_back("select");
        if (throw_select) throw std::runtime_error("select failure");
        return target;
    }
    void ReleaseTargetPool(std::uint32_t selected_pool) override {
        Check(selected_pool == pool, "releases selected pool");
        calls.emplace_back("release-pool");
    }
    std::string_view TargetName(std::uint32_t selected) override {
        Check(selected == kTarget, "name reads selected target");
        calls.emplace_back("name");
        return "target";
    }
    void LogAction(std::uint32_t task, std::string_view source,
                   std::string_view message,
                   std::string_view value) override {
        Check(task == kTask, "log receives task");
        if (message == "selectTarget: %s") {
            Check(source == "cbActorAi::doNormalAttackAction" &&
                      value == "target", "normal attack log literal");
        } else {
            Check(source == "cbActorAi::doFleeAction" &&
                      message == "return fleePipeline" && value.empty(),
                  "flee log literal");
        }
        calls.emplace_back("log");
    }
    std::uint32_t CreateCombatAction(
        std::uint32_t actor, std::uint32_t selected,
        std::uint32_t flags) override {
        Check(actor == kActor && selected == kTarget && flags == 0u,
              "combat action receives actor, target, zero flags");
        calls.emplace_back("combat");
        return 0xa000u;
    }
    std::uint32_t CreateCombatFlee(std::uint32_t actor) override {
        Check(actor == kActor, "combat flee receives actor");
        calls.emplace_back("flee");
        return 0xb000u;
    }
    std::uint32_t FallbackNormal(
        std::uint32_t task, std::uint32_t requested) override {
        Check(task == kTask && requested == kRequested,
              "flee fallback preserves requested target");
        calls.emplace_back("fallback");
        return 0xc000u;
    }
};

void NormalAttackPaths() {
    Runtime runtime;
    Check(pal4::AiDirectAction::NormalAttack(
              kTask, kRequested, runtime) == 0xa000u &&
          runtime.calls == std::vector<std::string>{
              "actor", "attack-cap", "acquire", "build",
              "release-targets", "select", "release-pool",
              "name", "log", "combat"},
          "normal attack target acquisition, cleanup and dispatch order");

    runtime = Runtime{};
    runtime.can_attack = false;
    Check(pal4::AiDirectAction::NormalAttack(
              kTask, kRequested, runtime) == 0u &&
          runtime.calls ==
              std::vector<std::string>{"actor", "attack-cap"},
          "disabled attack does not acquire targets");

    runtime = Runtime{};
    runtime.target = 0u;
    Check(pal4::AiDirectAction::NormalAttack(
              kTask, kRequested, runtime) == 0u &&
          runtime.calls.back() == "release-pool",
          "missing target releases pool without combat dispatch");

    runtime = Runtime{};
    runtime.actor_targets = 0u;
    (void)pal4::AiDirectAction::NormalAttack(kTask, kRequested, runtime);
    Check(runtime.calls[3] == "build" &&
          runtime.calls[4] == "select",
          "empty actor-target handle skips release");
}

void ExceptionsAndFlee() {
    Runtime runtime;
    runtime.throw_build = true;
    try {
        (void)pal4::AiDirectAction::NormalAttack(
            kTask, kRequested, runtime);
        Check(false, "build must throw");
    } catch (const std::runtime_error&) {}
    Check(runtime.calls.back() == "release-targets",
          "build failure releases acquired actor targets");

    runtime = Runtime{};
    runtime.throw_select = true;
    try {
        (void)pal4::AiDirectAction::NormalAttack(
            kTask, kRequested, runtime);
        Check(false, "selector must throw");
    } catch (const std::runtime_error&) {}
    Check(runtime.calls.back() == "release-pool",
          "selection failure releases temporary pool");

    runtime = Runtime{};
    Check(pal4::AiDirectAction::Flee(
              kTask, kRequested, runtime) == 0xb000u &&
          runtime.calls == std::vector<std::string>{
              "actor", "flee-cap", "log", "flee"},
          "enabled flee logs then dispatches combat flee");
    runtime = Runtime{};
    runtime.can_flee = false;
    Check(pal4::AiDirectAction::Flee(
              kTask, kRequested, runtime) == 0xc000u &&
          runtime.calls == std::vector<std::string>{
              "actor", "flee-cap", "fallback"},
          "disabled flee delegates to normal attack");
}

}  // namespace

int main() {
    try {
        NormalAttackPaths();
        ExceptionsAndFlee();
        std::cout << "ai_direct_action_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
