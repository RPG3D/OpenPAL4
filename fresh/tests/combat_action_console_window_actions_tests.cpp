#include "pal4/combat_action_console_window_actions.h"

#include <array>
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

struct Runtime final : pal4::ICombatActionConsoleWindowActionRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> calls;
    std::uint32_t invoked{};
    std::uint32_t member{};
    std::uint32_t callback{};
    std::uint32_t setup_context{};
    std::uint32_t visibility_context{};
    std::uint32_t visible{};
    std::uint32_t reset_count{};
    std::uint32_t built_context{};
    std::uint32_t processed{};
    std::uint32_t validation_result = 1u;
    std::uint32_t extra_reset_member = 0x105cu;

    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.emplace_back("read");
        return words.at(address);
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        calls.emplace_back("write");
        words[address] = value;
    }
    void InvokeAction(std::uint32_t window,
                      std::uint32_t action) override {
        Check(window == 0x1000u, "action owner");
        calls.emplace_back("action");
        invoked = action;
    }
    void SetupSelector(std::uint32_t address, std::uint32_t window,
                       std::uint32_t action, std::uint32_t context,
                       std::uint32_t flags) override {
        Check(window == 0x1000u && flags == 0u,
              "selector owner and flags");
        calls.emplace_back("selector");
        member = address;
        callback = action;
        setup_context = context;
    }
    std::uint32_t DialogDataFromParam(std::uint32_t event) override {
        Check(event == 0x3000u, "show event");
        calls.emplace_back("dialog");
        return 0x8000u;
    }
    std::uint32_t UpdateVisibility(std::uint32_t window,
                                   std::uint32_t context,
                                   std::uint32_t show) override {
        Check(window == 0x1000u, "visibility owner");
        calls.emplace_back("visibility");
        visibility_context = context;
        visible = show;
        return 0x99u;
    }
    std::uint32_t FrameManager() override {
        calls.emplace_back("manager");
        return 0x4000u;
    }
    void ShowTutorial(std::uint32_t manager,
                      std::uint32_t index) override {
        Check(manager == 0x4000u && index == 9u,
              "tutorial manager and index");
        calls.emplace_back("tutorial");
    }
    std::uint32_t ResetSelector(std::uint32_t member,
                                std::uint32_t first,
                                std::uint32_t second) override {
        constexpr std::array<std::uint32_t, 3> members{
            0x105cu, 0x1110u, 0x10b4u};
        Check(first == 0u && second == 0u &&
                  member == (reset_count < 3u
                                 ? members[reset_count]
                                 : extra_reset_member),
              "selector reset order and zero flags");
        calls.emplace_back("reset:" + std::to_string(member));
        return 0x30u + reset_count++;
    }
    std::uint32_t ValidateMagic(std::uint32_t selection) override {
        Check(selection == 0xa000u, "magic selection address");
        calls.emplace_back("validate-magic");
        return validation_result;
    }
    std::uint32_t ValidateStunt(std::uint32_t selection) override {
        Check(selection == 0xa000u, "stunt selection address");
        calls.emplace_back("validate-stunt");
        return validation_result;
    }
    std::uint32_t BuildActionPayload(std::uint32_t context) override {
        built_context = context;
        calls.emplace_back("build-payload");
        return 0x9000u;
    }
    std::uint32_t EventRouter() override {
        calls.emplace_back("event-router");
        return 0x4000u;
    }
    std::uint32_t ProcessEvent(std::uint32_t router,
                               std::uint32_t key,
                               std::uint32_t payload,
                               std::uint32_t flags) override {
        Check(router == 0x4000u && key == 0x8b62d0u &&
                  payload == 0x9000u && flags == 1u,
              "selection event router, key, payload and flags");
        ++processed;
        calls.emplace_back("process-event");
        return 0x77u;
    }
};

void CheckEventGateAndCases() {
    Runtime missing;
    missing.words[0x1058u] = 0u;
    Check(pal4::CombatActionConsoleWindowActions::HandleActionEvent(
              0x1000u, 0x3000u, missing) == 1u &&
              missing.calls == std::vector<std::string>{"read"},
          "empty context returns before event dereference");

    constexpr std::array<std::uint32_t, 6> callbacks{
        0x541b10u, 0x541b50u, 0x5419c0u,
        0x541910u, 0x541a70u, 0x541b90u};
    constexpr std::array<std::uint32_t, 6> members{
        0u, 0u, 0x110u, 0x5cu, 0xb4u, 0u};
    for (std::uint32_t type = 0u; type < 6u; ++type) {
        Runtime runtime;
        runtime.words[0x1058u] = 0x8000u;
        runtime.words[0x3008u] = 0x5000u;
        runtime.words[0x5118u] = type;
        Check(pal4::CombatActionConsoleWindowActions::HandleActionEvent(
                  0x1000u, 0x3000u, runtime) == 1u &&
                  runtime.words[0x11e4u] == 0x1168u + 20u * type,
              "selected navigation node follows event type");
        if (members[type] == 0u)
            Check(runtime.invoked == callbacks[type] &&
                      runtime.member == 0u,
                  "direct action callback");
        else
            Check(runtime.member == 0x1000u + members[type] &&
                      runtime.callback == callbacks[type] &&
                      runtime.setup_context == 0x8000u,
                  "selector setup callback and context");
        Check(runtime.calls == (std::vector<std::string>{
                  "read", "read", "read", "write",
                  members[type] == 0u ? "action" : "selector"}),
              "context, payload, type and selected node precede dispatch");
    }

    Runtime unknown;
    unknown.words[0x1058u] = 0x8000u;
    unknown.words[0x3008u] = 0x5000u;
    unknown.words[0x5118u] = 6u;
    Check(pal4::CombatActionConsoleWindowActions::HandleActionEvent(
              0x1000u, 0x3000u, unknown) == 1u &&
              unknown.words[0x11e4u] == 0x11e0u &&
              unknown.calls == (std::vector<std::string>{
                  "read", "read", "read", "write"}),
          "unknown type still updates node pointer without dispatch");
}

void CheckShowHide() {
    Runtime runtime;
    pal4::CombatActionConsoleWindowActions::ShowDialog(
        0x1000u, 0x3000u, runtime);
    Check(runtime.words[0x1058u] == 0x8000u &&
              runtime.visibility_context == 0x8000u &&
              runtime.visible == 1u &&
              runtime.calls == (std::vector<std::string>{
                  "dialog", "write", "visibility", "manager", "tutorial"}),
          "show stores dialog context then updates and opens tutorial 9");
    runtime.calls.clear();
    Check(pal4::CombatActionConsoleWindowActions::HideDialog(
              0x1000u, runtime) == 0x99u &&
              runtime.words[0x1058u] == 0u &&
              runtime.visibility_context == 0u && runtime.visible == 0u &&
              runtime.calls == (std::vector<std::string>{
                  "write", "visibility"}),
          "hide clears context and forwards visibility result");
}

void CheckSelections() {
    using Actions = pal4::CombatActionConsoleWindowActions;
    for (std::uint32_t kind = 0u; kind < 3u; ++kind) {
        Runtime runtime;
        runtime.words[0x1058u] = 0x8000u;
        runtime.words[0xa000u] = 0x1234u;
        const auto select = [&](std::uint32_t selection) {
            switch (kind) {
            case 0u: return Actions::SelectMagic(0x1000u, selection, runtime);
            case 1u: return Actions::SelectStunt(0x1000u, selection, runtime);
            default: return Actions::SelectArticle(0x1000u, selection, runtime);
            }
        };
        Check(select(0xa000u) == 0x77u && runtime.reset_count == 3u &&
                  runtime.built_context == 0x8000u &&
                  runtime.words[0x9008u] == 0x1234u &&
                  runtime.words[0x900cu] == kind &&
                  runtime.processed == 1u,
              "selected action resets all selectors and sends value and kind");
        Check(runtime.calls.back() == "process-event",
              "event processing is last");
    }

    for (std::uint32_t kind = 0u; kind < 3u; ++kind) {
        Runtime runtime;
        constexpr std::array<std::uint32_t, 3> members{
            0x105cu, 0x1110u, 0x10b4u};
        runtime.extra_reset_member = members[kind];
        std::uint32_t result{};
        if (kind == 0u)
            result = Actions::SelectMagic(0x1000u, 0u, runtime);
        else if (kind == 1u)
            result = Actions::SelectStunt(0x1000u, 0u, runtime);
        else
            result = Actions::SelectArticle(0x1000u, 0u, runtime);
        Check(result == 0x33u && runtime.reset_count == 4u &&
                  runtime.processed == 0u,
              "null selection resets its selector twice and returns fourth reset");
    }

    Runtime invalid_magic;
    invalid_magic.validation_result = 0u;
    Check(Actions::SelectMagic(0x1000u, 0xa000u, invalid_magic) == 0u &&
              invalid_magic.processed == 0u,
          "failed magic validation does not read context or send event");
    Runtime invalid_stunt;
    invalid_stunt.validation_result = 0u;
    Check(Actions::SelectStunt(0x1000u, 0xa000u, invalid_stunt) == 0u &&
              invalid_stunt.processed == 0u,
          "failed stunt validation does not send event");

    Runtime no_context;
    no_context.words[0x1058u] = 0u;
    Check(Actions::SelectArticle(0x1000u, 0xa000u, no_context) == 0x32u &&
              no_context.processed == 0u,
          "article selection without context returns third reset result");
}

void CheckDirectActions() {
    using Actions = pal4::CombatActionConsoleWindowActions;
    constexpr std::array<std::uint32_t, 3> codes{4u, 5u, 3u};
    for (std::uint32_t kind = 0u; kind < 3u; ++kind) {
        Runtime runtime;
        runtime.words[0x1058u] = kind == 0u ? 0u : 0x8000u;
        std::uint32_t result{};
        if (kind == 0u) result = Actions::Attack(0x1000u, runtime);
        else if (kind == 1u) result = Actions::Defend(0x1000u, runtime);
        else result = Actions::Flee(0x1000u, runtime);
        Check(result == 0x77u && runtime.processed == 1u &&
                  runtime.words[0x900cu] == codes[kind] &&
                  runtime.built_context == runtime.words[0x1058u] &&
                  runtime.calls == (std::vector<std::string>{
                      "read", "build-payload", "write", "event-router",
                      "process-event"}),
              "direct action sends fixed code even with null context");
    }
}

}  // namespace

int main() {
    try {
        CheckEventGateAndCases();
        CheckShowHide();
        CheckSelections();
        CheckDirectActions();
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
    return 0;
}
