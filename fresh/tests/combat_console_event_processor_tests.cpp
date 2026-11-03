#include "pal4/combat_console.h"

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

constexpr std::uint32_t kConsole = 0x5000u;
constexpr std::uint32_t kEvent = 0x6000u;

struct Iterator final : pal4::ICombatConsoleActorIterator {
    std::vector<std::string>& calls;
    std::string label;
    std::vector<std::uint32_t> actors;
    std::size_t position = 0u;

    Iterator(std::vector<std::string>& log, std::string name,
             std::vector<std::uint32_t> values)
        : calls(log), label(std::move(name)), actors(std::move(values)) {}
    ~Iterator() override { calls.push_back("destroy-" + label); }
    bool AtEnd() override {
        calls.push_back("end-" + label);
        return position == actors.size();
    }
    std::uint32_t Current() override {
        calls.push_back("current-" + label);
        return actors.at(position);
    }
    void Advance() override {
        calls.push_back("advance-" + label);
        ++position;
    }
};

struct ResultStub final : pal4::ICombatConsoleResultRuntime {
    std::vector<std::string>& calls;
    bool visible = true;
    explicit ResultStub(std::vector<std::string>& log) : calls(log) {}

    std::uint32_t ReadWord(std::uint32_t address) override {
        if (address == kConsole + 216u) return 0x8000u;
        if (address == kConsole + 268u) return 0x9000u;
        Check(address == kConsole + 272u, "result event field");
        return 17u;
    }
    void WriteWord(std::uint32_t, std::uint32_t) override {
        throw std::runtime_error("unexpected result write");
    }
    void SetVisible(std::uint32_t, bool) override {
        throw std::runtime_error("unexpected result visibility set");
    }
    bool IsVisible(std::uint32_t window, bool recursive) override {
        Check(window == 0x8000u && !recursive, "visible result panel");
        calls.push_back("result-visible");
        return visible;
    }
    std::uint32_t ActorFromEvent(std::uint32_t) override {
        throw std::runtime_error("unexpected result actor lookup");
    }
    std::uint32_t CombatSystem() override {
        throw std::runtime_error("unexpected result system lookup");
    }
    std::uint32_t ActorManager() override {
        throw std::runtime_error("unexpected result manager lookup");
    }
    std::uint32_t CreateCombatAction(std::uint32_t, std::uint32_t,
                                     std::uint32_t) override {
        throw std::runtime_error("unexpected combat action");
    }
    void RegisterCombatAction(std::uint32_t,
                              std::uint32_t) override {
        throw std::runtime_error("unexpected action registration");
    }
    std::uint32_t CompleteCombatResult(std::uint32_t,
                                       std::uint32_t) override {
        throw std::runtime_error("unexpected result completion");
    }
    std::uint8_t DispatchSelectJoinAttack(std::uint32_t actor,
                                          std::uint32_t value,
                                          std::uint32_t mode) override {
        Check(actor == 0x9000u && value == 17u && mode == 1u,
              "result dispatch payload");
        calls.push_back("result-dispatch");
        return 0u;
    }
};

struct Runtime final : pal4::ICombatConsoleEventRuntime {
    std::vector<std::string> calls;
    ResultStub result{calls};
    std::uint32_t key = 0u;
    std::uint32_t current = 0xa2u;
    std::uint32_t selected = 0u;
    std::vector<std::uint32_t> role_actors{0xa0u, 0xa1u, 0xa2u};
    std::vector<std::uint32_t> candidates{0xa4u, 0xa1u, 0xa3u, 0xa2u};

    std::uint32_t ReadWord(std::uint32_t address) override {
        Check(address == kEvent + 16u, "event key field");
        calls.push_back("key");
        return key;
    }
    void DispatchInputEvent(std::string_view name,
                            std::uint32_t event,
                            std::uint32_t mode) override {
        Check(name == "repeat_action" && event == kEvent && mode == 1u,
              "repeat action event");
        calls.push_back("repeat");
    }
    void DispatchActorEvent(std::string_view name,
                            std::uint32_t actor,
                            std::uint32_t mode) override {
        Check(name == "re_substitute" && mode == 1u,
              "substitution event");
        selected = actor;
        calls.push_back("substitute");
    }
    pal4::ICombatConsoleResultRuntime& ResultRuntime() override {
        calls.push_back("result-runtime");
        return result;
    }
    std::unique_ptr<pal4::ICombatConsoleActorIterator> RoleActors() override {
        calls.push_back("roles");
        return std::make_unique<Iterator>(calls, "roles", role_actors);
    }
    std::uint32_t CombatSystem() override {
        calls.push_back("system");
        return 0xb000u;
    }
    void StartCombat(std::uint32_t system) override {
        Check(system == 0xb000u, "start combat system");
        calls.push_back("start");
    }
    void ConfirmCombat(std::uint32_t system) override {
        Check(system == 0xb000u, "confirm combat system");
        calls.push_back("confirm");
    }
    std::uint32_t CurrentTarget(std::uint32_t system) override {
        Check(system == 0xb000u, "current target system");
        calls.push_back("current-target");
        return current;
    }
    std::unique_ptr<pal4::ICombatConsoleActorIterator>
    TargetCandidates(std::uint32_t target) override {
        Check(target == current, "target collection owner");
        calls.push_back("candidates");
        return std::make_unique<Iterator>(calls, "candidates", candidates);
    }
    bool IsValidTarget(std::uint32_t system,
                       std::uint32_t actor) override {
        Check(system == 0xb000u, "target validity system");
        calls.push_back("valid");
        return actor != 0xa4u;
    }
    std::int32_t ActorOrder(std::uint32_t actor) override {
        calls.push_back("order");
        return static_cast<std::int32_t>(actor - 0xa0u);
    }
    void SelectTarget(std::uint32_t system,
                      std::uint32_t actor) override {
        Check(system == 0xb000u, "selection system");
        calls.push_back("select");
        selected = actor;
    }
};

std::size_t First(const Runtime& runtime, const std::string& call) {
    const auto found = std::find(runtime.calls.begin(), runtime.calls.end(),
                                 call);
    Check(found != runtime.calls.end(), "missing event operation");
    return static_cast<std::size_t>(found - runtime.calls.begin());
}

void DirectEvents() {
    Runtime repeat;
    repeat.key = 19u;
    Check(!pal4::CombatConsoleWindow::ProcessCombatEvent(
              kConsole, kEvent, repeat) &&
              First(repeat, "repeat") > First(repeat, "key"),
          "repeat key dispatches then falls through with false return");

    Runtime start;
    start.key = 1u;
    Check(pal4::CombatConsoleWindow::ProcessCombatEvent(
              kConsole, kEvent, start) &&
              First(start, "system") < First(start, "start"),
          "start key enters combat");

    Runtime confirm;
    confirm.key = 28u;
    Check(pal4::CombatConsoleWindow::ProcessCombatEvent(
              kConsole, kEvent, confirm) &&
              First(confirm, "result-visible") <
                  First(confirm, "result-dispatch") &&
              First(confirm, "result-dispatch") < First(confirm, "confirm"),
          "confirm dispatches visible result before combat confirmation");

    Runtime hidden;
    hidden.key = 28u;
    hidden.result.visible = false;
    Check(pal4::CombatConsoleWindow::ProcessCombatEvent(
              kConsole, kEvent, hidden) &&
              std::count(hidden.calls.begin(), hidden.calls.end(),
                         "result-dispatch") == 0 &&
              First(hidden, "result-visible") < First(hidden, "confirm"),
          "hidden result skips selection but confirms combat");
}

void ActorTypeKeys() {
    for (std::uint32_t desired = 0u; desired < 3u; ++desired) {
        Runtime runtime;
        runtime.key = 59u + desired;
        Check(!pal4::CombatConsoleWindow::ProcessCombatEvent(
                  kConsole, kEvent, runtime) &&
                  runtime.selected == 0xa0u + desired &&
                  First(runtime, "substitute") <
                      First(runtime, "destroy-roles"),
              "actor-type key selects matching role and releases iterator");
    }
    Runtime missing;
    missing.key = 60u;
    missing.role_actors = {0xa0u, 0xa2u};
    Check(!pal4::CombatConsoleWindow::ProcessCombatEvent(
              kConsole, kEvent, missing) &&
              missing.selected == 0u &&
              First(missing, "destroy-roles") > First(missing, "roles"),
          "missing role type skips event and releases iterator");
}

void TargetCycling() {
    for (const auto [key, expected] :
         {std::pair{203u, 0xa1u}, std::pair{208u, 0xa1u},
          std::pair{200u, 0xa3u}, std::pair{205u, 0xa3u}}) {
        Runtime runtime;
        runtime.key = key;
        Check(pal4::CombatConsoleWindow::ProcessCombatEvent(
                  kConsole, kEvent, runtime) &&
                  runtime.selected == expected &&
                  First(runtime, "select") <
                      First(runtime, "destroy-candidates"),
              "direction key cycles sorted eligible targets");
        Check(std::count(runtime.calls.begin(), runtime.calls.end(),
                         "valid") == 4,
              "all four target-cycle modes apply validity filtering");
    }
    Runtime wrap_previous;
    wrap_previous.key = 203u;
    wrap_previous.current = 0xa1u;
    Check(pal4::CombatConsoleWindow::ProcessCombatEvent(
              kConsole, kEvent, wrap_previous) &&
              wrap_previous.selected == 0xa3u,
          "previous wraps first eligible target to last");
    Runtime wrap_next;
    wrap_next.key = 205u;
    wrap_next.current = 0xa3u;
    Check(pal4::CombatConsoleWindow::ProcessCombatEvent(
              kConsole, kEvent, wrap_next) &&
              wrap_next.selected == 0xa1u,
          "next wraps last eligible target to first");
    Runtime no_current;
    no_current.key = 200u;
    no_current.current = 0u;
    Check(pal4::CombatConsoleWindow::ProcessCombatEvent(
              kConsole, kEvent, no_current) &&
              std::count(no_current.calls.begin(), no_current.calls.end(),
                         "candidates") == 0,
          "null current target still consumes cycle key");
    Runtime unhandled;
    unhandled.key = 44u;
    Check(!pal4::CombatConsoleWindow::ProcessCombatEvent(
              kConsole, kEvent, unhandled) &&
              unhandled.calls == std::vector<std::string>{"key"},
          "unhandled event returns false without side effects");
}

}  // namespace

int main() {
    try {
        DirectEvents();
        ActorTypeKeys();
        TargetCycling();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
