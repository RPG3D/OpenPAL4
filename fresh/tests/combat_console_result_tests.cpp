#include "pal4/combat_console.h"

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

constexpr std::uint32_t kConsole = 0x5000u;
constexpr std::uint32_t kEvent = 0x6000u;
constexpr std::uint32_t kSystem = 0x7000u;

struct ResultRuntime final : pal4::ICombatConsoleResultRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> words{
        {kConsole + 216u, 0x8000u}, {kEvent + 4u, 17u}};
    std::vector<std::string> calls;
    std::uint32_t actor = 0x9000u;
    bool visible = false;
    std::uint8_t dispatched = 7u;

    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.push_back("read");
        return words.at(address);
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        calls.push_back("write");
        words[address] = value;
    }
    void SetVisible(std::uint32_t window, bool show) override {
        Check(window == 0x8000u, "result panel pointer");
        visible = show;
        calls.push_back(show ? "show" : "hide");
    }
    bool IsVisible(std::uint32_t window, bool recursive) override {
        Check(window == 0x8000u && !recursive,
              "nonrecursive visibility query");
        calls.push_back("visible");
        return visible;
    }
    std::uint32_t ActorFromEvent(std::uint32_t event) override {
        Check(event == kEvent, "event pointer");
        calls.push_back("actor");
        return actor;
    }
    std::uint32_t CombatSystem() override {
        calls.push_back("system");
        return kSystem;
    }
    std::uint32_t ActorManager() override {
        calls.push_back("manager");
        return 0xb000u;
    }
    std::uint32_t CreateCombatAction(std::uint32_t target,
                                     std::uint32_t value,
                                     std::uint32_t mode) override {
        Check(target == actor && value == 17u && mode == 1u,
              "combat action parameters");
        calls.push_back("create-action");
        return 0xa000u;
    }
    void RegisterCombatAction(std::uint32_t manager,
                              std::uint32_t action) override {
        Check(manager == 0xb000u && action == 0xa000u,
              "manager and registered action");
        calls.push_back("register-action");
    }
    std::uint32_t CompleteCombatResult(std::uint32_t target,
                                       std::uint32_t value) override {
        Check(target == actor && value == 17u,
              "result callback arguments");
        calls.push_back("complete");
        return 0xabcdef01u;
    }
    std::uint8_t DispatchSelectJoinAttack(std::uint32_t target,
                                          std::uint32_t value,
                                          std::uint32_t mode) override {
        Check(target == actor && value == 17u && mode == 1u,
              "select_join_atk event payload");
        calls.push_back("dispatch");
        return dispatched;
    }
};

std::size_t First(const ResultRuntime& runtime, const std::string& call) {
    const auto found = std::find(runtime.calls.begin(), runtime.calls.end(),
                                 call);
    Check(found != runtime.calls.end(), "expected result operation");
    return static_cast<std::size_t>(found - runtime.calls.begin());
}

void ShowAndHideResult() {
    ResultRuntime runtime;
    Check(pal4::CombatConsoleWindow::ShowCombatResult(
              kConsole, kEvent, runtime) == kSystem && runtime.visible &&
              runtime.words.at(kConsole + 268u) == runtime.actor &&
              runtime.words.at(kConsole + 272u) == 17u &&
              runtime.words.at(kSystem + 392u) == 0u &&
              runtime.words.at(kSystem + 388u) == 0u,
          "show caches result data and clears combat system result");
    Check(runtime.calls[0] == "read" && runtime.calls[1] == "show" &&
              std::count(runtime.calls.begin(), runtime.calls.end(),
                         "system") == 2,
          "show uses panel before two system lookups");
    runtime.calls.clear();
    Check(pal4::CombatConsoleWindow::HideCombatResult(
              kConsole, kEvent, runtime) == 0xabcdef01u &&
              !runtime.visible && runtime.words.at(kConsole + 268u) == 0u &&
              runtime.words.at(kConsole + 272u) == 0u &&
              runtime.words.at(kSystem + 392u) == 17u &&
              runtime.words.at(kSystem + 388u) == runtime.actor,
          "hide moves cached result to combat system and completes action");
    Check(First(runtime, "hide") < First(runtime, "create-action") &&
              First(runtime, "manager") < First(runtime, "create-action") &&
              First(runtime, "create-action") <
                  First(runtime, "register-action") &&
              First(runtime, "register-action") < First(runtime, "complete"),
          "combat action registration precedes actor callback");
    Check(std::count(runtime.calls.begin(), runtime.calls.end(),
                     "actor") == 4,
          "hide resolves event actor at each original call site");
}

void NullActorAndClick() {
    ResultRuntime runtime;
    runtime.actor = 0u;
    Check(pal4::CombatConsoleWindow::HideCombatResult(
              kConsole, kEvent, runtime) == 0u && !runtime.visible &&
              std::count(runtime.calls.begin(), runtime.calls.end(),
                         "system") == 0,
          "null actor hides result without combat action");
    runtime.calls.clear();
    Check(pal4::CombatConsoleWindow::DispatchVisibleCombatResult(
              kConsole, runtime) == 0u &&
              std::count(runtime.calls.begin(), runtime.calls.end(),
                         "dispatch") == 0,
          "hidden result does not dispatch selection event");
    runtime.actor = 0x9000u;
    runtime.visible = true;
    runtime.words[kConsole + 268u] = runtime.actor;
    runtime.words[kConsole + 272u] = 17u;
    runtime.calls.clear();
    Check(pal4::CombatConsoleWindow::DispatchVisibleCombatResult(
              kConsole, runtime) == 7u &&
              First(runtime, "visible") < First(runtime, "dispatch"),
          "visible result dispatches select_join_atk");
    runtime.dispatched = 0u;
    runtime.calls.clear();
    Check(pal4::CombatConsoleWindow::OnCombatResultClick(
              kConsole, 0xdeadbeefu, runtime) &&
              First(runtime, "dispatch") > First(runtime, "visible"),
          "click wrapper returns true even when dispatch returns false");
}

struct AlphaRuntime final : pal4::ICombatConsoleAlphaRuntime {
    std::uint32_t window = 0x8100u;
    float speed = 0.5f;
    float alpha = 0.25f;
    float applied = -1.0f;
    std::vector<std::string> calls;

    std::uint32_t ReadWord(std::uint32_t address) override {
        Check(address == kConsole + 308u, "alpha target field");
        calls.push_back("target");
        return window;
    }
    float ReadFloat(std::uint32_t address) override {
        if (address == kConsole + 312u) {
            calls.push_back("speed");
            return speed;
        }
        Check(address == window + 232u, "CEGUI alpha offset");
        calls.push_back("alpha");
        return alpha;
    }
    void WriteFloat(std::uint32_t address, float value) override {
        Check(address == kConsole + 312u, "reflected speed address");
        calls.push_back("reflect");
        speed = value;
    }
    void SetAlpha(std::uint32_t target, float value) override {
        Check(target == window, "alpha target");
        calls.push_back("set-alpha");
        applied = value;
    }
};

void AlphaBoundaries() {
    AlphaRuntime missing;
    missing.window = 0u;
    pal4::CombatConsoleWindow::UpdateAlpha(kConsole, 1.0, missing);
    Check(missing.calls == std::vector<std::string>{"target"},
          "missing target leaves alpha and speed untouched");

    AlphaRuntime rising;
    pal4::CombatConsoleWindow::UpdateAlpha(kConsole, 1.0, rising);
    Check(rising.applied == 0.75f && rising.speed == 0.5f &&
              rising.calls == std::vector<std::string>{
                  "target", "speed", "alpha", "set-alpha"},
          "alpha advances without reflection inside bounds");
    rising.alpha = 0.75f;
    pal4::CombatConsoleWindow::UpdateAlpha(kConsole, 1.0, rising);
    Check(rising.applied == 1.0f && rising.speed == -0.5f,
          "upper overshoot clamps and reflects");

    AlphaRuntime falling;
    falling.speed = -0.5f;
    falling.alpha = 0.25f;
    pal4::CombatConsoleWindow::UpdateAlpha(kConsole, 1.0, falling);
    Check(falling.applied == 0.0f && falling.speed == 0.5f,
          "lower overshoot clamps and reflects");

    AlphaRuntime exact;
    exact.alpha = 0.5f;
    pal4::CombatConsoleWindow::UpdateAlpha(kConsole, 1.0, exact);
    Check(exact.applied == 1.0f && exact.speed == 0.5f,
          "exact upper bound does not reflect");
}

}  // namespace

int main() {
    try {
        ShowAndHideResult();
        NullActorAndClick();
        AlphaBoundaries();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
