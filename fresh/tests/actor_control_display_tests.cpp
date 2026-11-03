#include "pal4/actor_control_display.h"

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <functional>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

using World = pal4::ActorControlWorldPoint;
using Screen = pal4::ActorControlScreenPoint;

struct Runtime final : pal4::ActorControlDisplayRuntimeStub {
    World ActorPosition(std::uint32_t actor) override {
        events.emplace_back("actor position");
        Check(actor == 0x7000u, "display reads actor at state +508");
        return {1.0f, 2.0f, 3.0f};
    }
    float ActorHeight(std::uint32_t actor) override {
        events.emplace_back("actor height");
        Check(actor == 0x7000u, "display reads actor height at +244");
        return 8.0f;
    }
    std::uint32_t CameraSubsystem() override {
        events.emplace_back("camera subsystem");
        return ++subsystem_count;
    }
    std::uint32_t ActiveCamera(std::uint32_t subsystem) override {
        events.emplace_back("active camera");
        camera_order.push_back(subsystem);
        return subsystem + 0x100u;
    }
    Screen Project(std::uint32_t camera, const World& position) override {
        events.emplace_back("project");
        Check(position == World{1.0f, 30.0f, 3.0f},
              "projection uses actor height plus twenty");
        return camera == 0x102u ? Screen{11.0f, 22.0f}
                                : Screen{33.0f, 44.0f};
    }
    std::uint32_t UiFrameManager() override {
        events.emplace_back("ui manager");
        return 0x9000u;
    }
    void ProcessUiState(std::uint32_t manager,
                        std::uint32_t mode) override {
        events.emplace_back("ui state");
        Check(manager == 0x9000u && mode == 1u,
              "display reset enables UI state");
        if (on_ui_state) on_ui_state();
    }
    std::function<void()> on_ui_state;
    std::uint32_t MonsterManager() override {
        events.emplace_back("monster manager");
        return 0xa100u;
    }
    void ClearMonsterFlags(std::uint32_t manager) override {
        events.emplace_back("clear monsters");
        Check(manager == 0xa100u,
              "display reset clears monster manager flags");
    }
    Screen ConvertCoordinates(std::uint32_t manager,
                              const Screen& projection,
                              const Screen& mixed) override {
        events.emplace_back("convert");
        Check(manager == 0x9000u &&
                  projection == Screen{33.0f, 44.0f} &&
                  mixed == Screen{33.0f, 22.0f},
              "UI conversion receives second projection and mixed axes");
        return {55.0f, 66.0f};
    }
    std::uint8_t ProcessUiEvents(std::uint32_t manager,
                                 std::uint32_t mode,
                                 const Screen& point) override {
        events.emplace_back("ui events");
        Check(manager == 0x9000u, "event wrapper uses UI manager");
        if (mode == 0u)
            Check(point == Screen{0.0f, 0.0f},
                  "hide path sends zeroed coordinate pair");
        else
            Check(mode == 1u && point == Screen{55.0f, 66.0f},
                  "show path sends converted coordinate pair");
        if(on_ui_events)on_ui_events();
        return 0x33u;
    }
    std::function<void()> on_ui_events;
    std::uint32_t ProgressBar(std::uint32_t manager) override {
        events.emplace_back("progress bar");
        Check(manager == 0x9000u, "progress bar uses UI manager");
        return 0x9100u;
    }
    void SetProgressVisible(std::uint32_t bar, bool visible) override {
        events.emplace_back("visible");
        Check(bar == 0x9100u && !visible,
              "hide path disables progress bar");
    }
    std::uint32_t GameArenaManager() override {
        events.emplace_back("arena");
        return 0xa000u;
    }
    std::uint8_t RemoveClump(std::uint32_t arena,
                             std::uint32_t clump) override {
        events.emplace_back("remove clump");
        Check(arena == 0xa000u && clump == 0xb000u,
              "hide path removes stored clump");
        return 0x55u;
    }

    std::uint32_t subsystem_count{};
    std::vector<std::uint32_t> camera_order;
    std::vector<std::string> events;
};

void TestStateAccessor() {
    pal4::ActorControlDatabasePrefix state{};
    state.words[28] = 0x12345678u;
    state.words[32] = 0xabcdef01u;
    Check(pal4::ActorControlDatabase::GetState28(state) ==
              0x12345678u &&
              state.words[32] == 0xabcdef01u,
          "0x423230 reads only the field at byte offset 112");
    state.words[32] = 0x44u;
    Check(pal4::ActorControlDatabase::GetDisplayState(state) == 0x44u,
          "display accessor returns state 32 when gates are clear");
    state.words[30] = 1u;
    Check(pal4::ActorControlDatabase::GetDisplayState(state) == 0u,
          "active control masks display state");
    state.words[30] = 0u;
    state.words[33] = 1u;
    Check(pal4::ActorControlDatabase::GetDisplayState(state) == 0u,
          "secondary gate masks display state");
}

void TestHideBranches() {
    for (const bool previous : {false, true}) {
        Runtime runtime;
        pal4::ActorControlDatabasePrefix state{};
        state.words[28] = previous ? 1u : 0u;
        state.words[40] = 0xb000u;
        const auto value = previous ? 0u : 1u;
        Check(pal4::ActorControlDatabase::UpdateDisplay(
                  state, value, runtime) == 0x55u &&
                  state.words[32] == value &&
                  runtime.events == std::vector<std::string>{
                      "ui manager", "ui events", "ui manager",
                      "progress bar", "visible", "arena",
                      "remove clump"},
              "either display flag hides progress and removes clump");
    }
}

void TestProjectedDisplay() {
    Runtime runtime;
    pal4::ActorControlDatabasePrefix state{};
    state.words[127] = 0x7000u;
    state.words[32] = 0xfeedu;
    Check(pal4::ActorControlDatabase::UpdateDisplay(
              state, 0u, runtime) == 0x33u &&
              state.words[32] == 0u &&
              runtime.camera_order ==
                  std::vector<std::uint32_t>{2u, 1u} &&
              runtime.events == std::vector<std::string>{
                  "actor position", "actor height", "camera subsystem",
                  "camera subsystem", "active camera", "project",
                  "active camera", "project", "ui manager", "convert",
                  "ui manager", "ui events"},
          "projected display preserves two-camera and UI-call order");
}

void TestResetAndUpdateDisplay() {
    for (int branch = 0; branch < 3; ++branch) {
        Runtime runtime;
        pal4::ActorControlDatabasePrefix state{};
        state.words[28] = 1u;
        state.words[29] = 1u;
        state.words[30] = 1u;
        state.words[127] = branch == 0 ? 0u : 0x7000u;
        state.words[32] = branch == 1 ? 1u : 0u;
        const auto result = pal4::ActorControlDatabase::ResetAndUpdateDisplay(
            state, runtime);
        Check(state.words[28] == 0u && state.words[29] == 0u &&
                  state.words[30] == 0u &&
                  result == (branch == 0 ? 0u : 0x33u),
              "display reset writes three control flags and returns branch result");
        const std::vector<std::string> prefix{
            "ui manager", "ui state", "monster manager",
            "clear monsters"};
        Check(runtime.events.size() >= prefix.size() &&
                  std::equal(prefix.begin(), prefix.end(),
                             runtime.events.begin()),
              "UI and monster reset precedes actor check");
        if (branch == 0)
            Check(runtime.events.size() == prefix.size(),
                  "null actor ends after reset calls");
        else if (branch == 1)
            Check(runtime.events.size() == prefix.size() + 2u &&
                      runtime.events.back() == "ui events",
                  "active display sends a zero-mode UI event");
        else
            Check(runtime.camera_order ==
                      std::vector<std::uint32_t>{2u, 1u} &&
                      runtime.events.back() == "ui events",
                  "inactive display projects actor after reset");
    }
}

void TestBorrowedDatabaseView() {
    std::array<std::uint32_t, 281> owner{};
    auto* const database = owner.data() + 35u;
    database[28] = database[29] = database[30] = 1u;
    database[127] = 0x7000u;
    owner[28] = 0x1234u;
    Runtime runtime;
    runtime.on_ui_state = [&] {
        Check(database[28] == 0u && database[29] == 0u && database[30] == 0u,
              "synchronous HUD callback sees database reset before dispatch");
        database[32] = 1u;
    };
    Check(pal4::ActorControlDatabase::ResetAndUpdateDisplay(
              std::span<std::uint32_t, 145>(database, 145u), runtime) == 0x33u &&
              database[32] == 1u && owner[28] == 0x1234u && runtime.camera_order.empty(),
          "borrowed database view preserves callback writes and enclosing controller fields");
}

void TestBorrowedDisplayView() {
    std::array<std::uint32_t,281> owner{};
    auto* database=owner.data()+35u;
    database[40]=0xb000u;owner[28]=0x1234u;
    Runtime runtime;
    runtime.on_ui_events=[&] {
        Check(database[32]==1u,"live callback observes display flag before UI dispatch");
        database[32]=7u;
    };
    Check(pal4::ActorControlDatabase::UpdateDisplay(
        std::span<std::uint32_t,145>(database,145u),1u,runtime)==0x55u &&
        database[32]==7u && owner[28]==0x1234u && runtime.events.back()=="remove clump",
        "borrowed display view preserves synchronous callback writes and hide side effects");
}

}  // namespace

int main() {
    TestStateAccessor();
    TestHideBranches();
    TestProjectedDisplay();
    TestResetAndUpdateDisplay();
    TestBorrowedDatabaseView();
    TestBorrowedDisplayView();
}
