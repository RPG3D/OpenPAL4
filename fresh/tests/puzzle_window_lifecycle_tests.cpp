#include "pal4/puzzle_window_lifecycle.h"

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

struct StorageRuntime final : pal4::IPuzzleWindowStorageRuntime {
    static constexpr std::uint32_t frame = 0x1000u;
    static constexpr std::uint32_t puzzle = frame + 0x2cu;
    std::unordered_map<std::uint32_t, std::uint32_t> words{
        {puzzle + 0x1cu, 0x3000u}, {puzzle + 0x20u, 0x30c8u},
        {puzzle + 4u, 0x4000u}, {puzzle + 8u, 0x4010u},
        {0x4004u, 0x5000u}, {0x4008u, 0x5008u},
        {0x400cu, 0x5010u}};
    std::vector<std::string> calls;
    std::vector<std::uint32_t> strings;
    std::uint32_t step_name = 0u;
    std::uint32_t base_frame = 0u;
    std::uint32_t sized_address = 0u;
    std::int32_t sized_count = 0;

    void ReleaseStepName(std::uint32_t address) override {
        step_name = address;
        calls.emplace_back("step-name");
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words.at(address);
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
        calls.emplace_back("write");
    }
    void DestroyString(std::uint32_t address) override {
        strings.push_back(address);
        calls.emplace_back("string");
    }
    void TouchCell(std::uint32_t, std::uint32_t) override {
        Check(false, "cell callback is not used by destructor");
    }
    void TouchRange(std::uint32_t begin,
                    std::uint32_t end) override {
        Check(begin == 0x5000u && end == 0x5008u,
              "inner range callback");
        calls.emplace_back("range");
    }
    void TouchNoop() override {
        Check(false, "no-op is not used by destructor");
    }
    void FreeRaw(std::uint32_t) override {
        Check(false, "raw free is not used by destructor");
    }
    std::uint32_t FreeZero(std::uint32_t address) override {
        Check(address == 0x3000u || address == 0x4000u,
              "outer vector free");
        calls.emplace_back("free-zero");
        return 0u;
    }
    void FreeSized(std::uint32_t address,
                   std::int32_t count) override {
        sized_address = address;
        sized_count = count;
        calls.emplace_back("free-sized");
    }
    std::uint32_t DestroyBase(std::uint32_t address) override {
        base_frame = address;
        calls.emplace_back("base");
        return 0xbeefu;
    }
};

struct UiRuntime final : pal4::IPuzzleWindowUiRuntime {
    static constexpr std::uint32_t frame = 0x1000u;
    static constexpr std::uint32_t message = 0x2000u;
    static constexpr std::uint32_t manager = 0x3000u;
    static constexpr std::uint32_t queue = 0x4000u;
    static constexpr std::uint32_t name = 0x8b12e0u;
    std::unordered_map<std::uint32_t, std::uint32_t> words{
        {0x8b12dcu, name}, {message, 1u}, {message + 4u, 3u},
        {manager + 0x1a0u, queue}};
    std::vector<std::string> calls;
    std::uint32_t removed_name = 0u;
    std::uint32_t removed_queue = 0u;
    std::uint32_t step_applied = 0xffffffffu;
    std::uint32_t found = frame;
    std::uint32_t event_type = 0u;
    std::uint32_t event_payload = 0xffffffffu;
    std::uint8_t active = 0u;
    std::uint32_t parent = 0u;
    std::uint32_t inserted = 0u;
    std::uint32_t removed = 0u;
    bool throw_initialize = false;

    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.emplace_back("read");
        return words.at(address);
    }
    void WriteByte(std::uint32_t address,
                   std::uint8_t value) override {
        Check(address == frame + 0x190u && value == 0u,
              "puzzle finish flag reset");
        calls.emplace_back("clear-flag");
    }
    void SetParent(std::uint32_t owner, std::uint8_t state,
                   std::uint32_t target) override {
        Check(owner == frame, "parent frame");
        active = state;
        parent = target;
        calls.emplace_back("parent");
    }
    std::uint32_t FrameManager() override {
        calls.emplace_back("manager");
        return manager;
    }
    std::uint32_t InsertActive(std::uint32_t array,
                               std::uint32_t value) override {
        Check(array == manager + 0x134u, "active array");
        inserted = value;
        calls.emplace_back("insert");
        return 0xa5u;
    }
    void RemoveActive(std::uint32_t array,
                      std::uint32_t value) override {
        Check(array == manager + 0x134u, "active array removal");
        removed = value;
        calls.emplace_back("remove");
    }
    void RemoveTimerByName(std::uint32_t target,
                           std::uint32_t name_address) override {
        removed_queue = target;
        removed_name = name_address;
        calls.emplace_back("remove-timer");
    }
    std::uint32_t CreateXmlDocument() override {
        calls.emplace_back("xml-create");
        return 0x5000u;
    }
    std::uint32_t Game() override {
        calls.emplace_back("game");
        return 0x6000u;
    }
    void InitializeStateAndUI(std::uint32_t game,
                              std::uint32_t mode,
                              std::uint32_t document) override {
        Check(game == 0x6000u && mode == 1u && document == 0x5000u,
              "game UI initialization arguments");
        calls.emplace_back("initialize");
        if (throw_initialize) throw std::runtime_error("initialize failed");
    }
    std::uint32_t DestroyXmlDocument(std::uint32_t document) override {
        Check(document == 0x5000u, "XML document cleanup");
        calls.emplace_back("xml-destroy");
        return 0xb6u;
    }
    void HandleFrameEvent(std::uint32_t window_name,
                          std::uint32_t type,
                          std::uint32_t payload) override {
        Check(window_name == name, "message frame name");
        event_type = type;
        event_payload = payload;
        calls.emplace_back("event");
    }
    std::uint32_t FindFrame(std::uint32_t target,
                            std::uint32_t window_name) override {
        Check(target == manager && window_name == name,
              "puzzle frame lookup");
        calls.emplace_back("find");
        return found;
    }
    void SetStepCount(std::uint32_t owner,
                      std::uint32_t step) override {
        Check(owner == frame, "step frame");
        step_applied = step;
        calls.emplace_back("step");
    }
};

struct StepRuntime final : pal4::IPuzzleWindowStepRuntime {
    static constexpr std::uint32_t frame = 0x1000u;
    static constexpr std::uint32_t manager = 0x2000u;
    static constexpr std::uint32_t queue = 0x3000u;
    std::vector<std::string> calls;
    std::uint8_t gate = 0u;
    std::uint32_t applied_step = 0xffffffffu;
    std::uint32_t scheduled_queue = 0u;

    void ProcessPuzzleStep(std::uint32_t puzzle,
                           std::uint32_t step) override {
        Check(puzzle == frame + 0x2cu, "puzzle subobject");
        applied_step = step;
        calls.emplace_back("process");
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.emplace_back("read");
        if (address == frame + 0x18cu) return 0x4000u;
        Check(address == manager + 0x1a0u, "timer queue offset");
        return queue;
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        Check(address == frame + 0x78u, "puzzle gate offset");
        calls.emplace_back("gate");
        return gate;
    }
    void SetTextFromString(std::uint32_t window,
                           std::uint32_t string) override {
        Check(window == 0x4000u && string == frame + 0xc4u,
              "puzzle state text");
        calls.emplace_back("text");
    }
    std::uint32_t FrameManager() override {
        calls.emplace_back("manager");
        return manager;
    }
    void ScheduleTimer(std::uint32_t target,
                       std::uint32_t name,
                       std::uint32_t period_bits,
                       std::uint32_t owner,
                       std::uint32_t callback,
                       std::uint32_t mode) override {
        Check(name == frame + 0x194u && period_bits == 0x3f800000u &&
                  owner == frame && callback == 0x4a1710u && mode == 0u,
              "timer arguments");
        scheduled_queue = target;
        calls.emplace_back("schedule");
    }
    std::uint32_t UpdateDisplay(std::uint32_t owner) override {
        Check(owner == frame, "display frame");
        calls.emplace_back("display");
        return 0x1234abu;
    }
};

void CheckStorage() {
    StorageRuntime runtime;
    Check(pal4::PuzzleWindowStorage::Destruct(
              StorageRuntime::frame, runtime) == 0xbeefu &&
              runtime.step_name == StorageRuntime::frame + 0x194u &&
              runtime.strings == (std::vector<std::uint32_t>{
                  StorageRuntime::puzzle + 0x98u, 0x3010u}) &&
              runtime.sized_address == 0x5000u &&
              runtime.sized_count == 4 &&
              runtime.base_frame == StorageRuntime::frame &&
              runtime.calls.front() == "step-name" &&
              runtime.calls.back() == "base",
          "puzzle step text precedes shared subobject destruction");
}

void CheckVisibilityAndMessages() {
    UiRuntime visible;
    Check(pal4::PuzzleWindowUi::SetVisible(
              UiRuntime::frame, 1u, 0x7777u, visible) == 0xa5u &&
              visible.parent == 0x7777u &&
              visible.inserted == UiRuntime::frame + 0x28u &&
              visible.calls == (std::vector<std::string>{
                  "parent", "clear-flag", "manager", "insert"}),
          "show inserts active frame record");
    UiRuntime hidden;
    Check(pal4::PuzzleWindowUi::SetVisible(
              UiRuntime::frame, 0u, 0u, hidden) == 0xb6u &&
              hidden.removed == UiRuntime::frame + 0x28u &&
              hidden.removed_queue == UiRuntime::queue &&
              hidden.removed_name == UiRuntime::frame + 0x194u &&
              hidden.calls == (std::vector<std::string>{
                  "parent", "clear-flag", "manager", "remove", "manager",
                  "read", "remove-timer", "xml-create", "game",
                  "initialize", "xml-destroy"}),
          "hide removes timer then reinitializes game UI");
    UiRuntime failing;
    failing.throw_initialize = true;
    bool caught = false;
    try {
        (void)pal4::PuzzleWindowUi::SetVisible(
            UiRuntime::frame, 0u, 0u, failing);
    } catch (const std::runtime_error&) {
        caught = true;
    }
    Check(caught && failing.calls.back() == "xml-destroy",
          "failed game UI initialization destroys XML document");

    for (const auto step : {0u, 5u, 6u, 0xffffffffu}) {
        UiRuntime runtime;
        runtime.words[UiRuntime::message + 4u] = step;
        Check(pal4::PuzzleWindowUi::HandleShowMessage(
                  UiRuntime::message, runtime) == 1u &&
                  runtime.event_type == 1u &&
                  runtime.event_payload == 0u &&
                  runtime.step_applied ==
                      (step == 6u ? 0xffffffffu : step) &&
                  (runtime.calls.back() == "step") == (step != 6u),
              "signed step boundary and message dispatch");
    }
    UiRuntime other_type;
    other_type.words[UiRuntime::message] = 2u;
    Check(pal4::PuzzleWindowUi::HandleShowMessage(
              UiRuntime::message, other_type) == 1u &&
              other_type.step_applied == 0xffffffffu,
          "other message type skips step");
    UiRuntime missing;
    missing.found = 0u;
    Check(pal4::PuzzleWindowUi::HandleShowMessage(
              UiRuntime::message, missing) == 1u &&
              missing.step_applied == 0xffffffffu,
          "missing puzzle frame skips setter");
}

void CheckStep() {
    StepRuntime active;
    Check(pal4::PuzzleWindowStep::SetStepCount(
              StepRuntime::frame, 4u, active) == 0xabu &&
              active.applied_step == 4u &&
              active.scheduled_queue == StepRuntime::queue &&
              active.calls == (std::vector<std::string>{
                  "process", "read", "text", "gate", "manager", "read",
                  "schedule", "display"}),
          "step update schedules one-second countdown");
    StepRuntime gated;
    gated.gate = 1u;
    Check(pal4::PuzzleWindowStep::SetStepCount(
              StepRuntime::frame, 0u, gated) == 0xabu &&
              gated.scheduled_queue == 0u &&
              gated.calls == (std::vector<std::string>{
                  "process", "read", "text", "gate", "display"}),
          "puzzle gate skips countdown");
}

}  // namespace

int main() {
    try {
        CheckStorage();
        CheckVisibilityAndMessages();
        CheckStep();
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
