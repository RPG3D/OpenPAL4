#include "pal4/actor_control_database.h"

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::ActorControlDatabaseRuntimeStub {
    std::uint32_t FreeAddress(void* address) override {
        freed.push_back(address);
        events.emplace_back("free");
        return 0x7777u;
    }
    void ConstructResourceManager(std::uint32_t* storage) override {
        events.emplace_back("construct resource");
        Check(storage == state->words.data() + 35u &&
                  state->words[0] == pal4::ActorControlBase::kVtable,
              "resource manager receives base-initialized subobject");
        storage[0] = 0x1111u;
    }
    void ConstructNameNode(std::uint32_t* storage) override {
        events.emplace_back("construct name");
        Check(storage == state->words.data() + 128u &&
                  state->words[35] == 0x1111u,
              "name node follows resource manager construction");
        storage[0] = 0x2222u;
    }
    void DestructNameNode(std::uint32_t* storage) override {
        events.emplace_back("destruct name");
        Check(storage == state->words.data() + 128u &&
                  state->words[0] == pal4::ActorControlDatabase::kVtable,
              "name node sees derived vtable during destruction");
    }
    void DestructResourceManager(std::uint32_t* storage) override {
        events.emplace_back("destruct resource");
        Check(storage == state->words.data() + 35u,
              "resource manager follows name-node destruction");
    }
    void ConcatenatePath(pal4::ActorControlPathBuffer& output,
                         std::uint32_t first,
                         std::uint32_t second) override {
        events.emplace_back("concatenate");
        Check(first == pal4::ActorControlDatabase::kPathFirstGlobal &&
                  second ==
                      pal4::ActorControlDatabase::kPathSecondGlobal,
              "dummy clump uses observed path globals");
        output.data = 0x7000u;
    }
    void AppendPath(pal4::ActorControlPathBuffer& path,
                    std::string_view suffix) override {
        events.emplace_back("append");
        Check(path.data == 0x7000u && suffix == "JDumy\\",
              "dummy path appends observed directory");
    }
    std::uint32_t GameInstance() override {
        events.emplace_back("game");
        return 0x8000u;
    }
    void LoadClump(std::uint32_t* resource_manager,
                   std::string_view name, std::uint32_t path,
                   std::string_view model, std::uint32_t palette,
                   std::uint32_t first, std::uint32_t second,
                   std::uint32_t enabled) override {
        events.emplace_back("load");
        Check(resource_manager == state->words.data() + 35u &&
                  name == "ACTCTRL-JDumy" && path == 0x7000u &&
                  model == "JDumy" && palette == 0x8000u + 0x1acu &&
                  first == 0u && second == 0u && enabled == 1u,
              "dummy clump load forwards exact resource arguments");
        if (throw_on_load) throw std::runtime_error("load failed");
    }
    void ReleasePath(pal4::ActorControlPathBuffer& path) override {
        events.emplace_back("release path");
        Check(path.data == 0x7000u,
              "temporary dummy path is released after load");
    }
    std::uint32_t HAnimFrameId(std::uint32_t clump) override {
        events.emplace_back("hanim");
        checked_clump = clump;
        return frame_id;
    }
    std::uint32_t GameArenaManager() override {
        events.emplace_back("arena");
        return 0x8800u;
    }
    void RemoveClump(std::uint32_t arena,
                     std::uint32_t clump) override {
        events.emplace_back("remove clump");
        removed_arena = arena;
        removed_clump = clump;
    }
    void CleanupClump(std::uint32_t* resource_manager,
                      std::uint32_t palette) override {
        events.emplace_back("cleanup clump");
        Check(resource_manager == state->words.data() + 35u &&
                  palette == 0x8000u + 0x1acu,
              "clump cleanup receives embedded manager and game palette");
    }
    bool NameNodeNeedsUpdate(std::uint32_t* name_node) override {
        events.emplace_back("needs update");
        Check(name_node == state->words.data() + 128u,
              "name update checks embedded node");
        return needs_update;
    }
    void HandleNameEvent(std::uint32_t* name_node,
                         std::uint32_t context,
                         std::uint32_t mode) override {
        events.emplace_back("name event");
        Check(name_node == state->words.data() + 128u,
              "name event targets embedded node");
        event_context = context;
        event_mode = mode;
    }
    std::uint32_t UiFrameManager() override {
        events.emplace_back("ui manager");
        return 0x9000u;
    }
    void ProcessUiEvents(std::uint32_t manager,
                         std::uint32_t mode,
                         std::array<std::uint32_t, 2>& scratch) override {
        events.emplace_back("ui events");
        Check(manager == 0x9000u && mode == 0u &&
                  scratch == std::array<std::uint32_t, 2>{0u, 0u},
              "UI wrapper receives zeroed two-word scratch buffer");
    }
    std::uint32_t CreateProgressBar(std::uint32_t manager) override {
        events.emplace_back("progress bar");
        Check(manager == 0x9000u,
              "progress bar uses second UI manager access");
        return 0x9100u;
    }
    void SetProgressBarVisible(std::uint32_t progress_bar,
                               bool visible) override {
        events.emplace_back("visible");
        visible_bar = progress_bar;
        visible_value = visible;
    }
    pal4::ActorControlDatabasePrefix* state{};
    bool throw_on_load{};
    bool needs_update{};
    std::uint32_t frame_id{};
    std::uint32_t checked_clump{};
    std::uint32_t removed_arena{};
    std::uint32_t removed_clump{};
    std::uint32_t event_context{};
    std::uint32_t event_mode{};
    std::uint32_t visible_bar{};
    bool visible_value{true};
    std::vector<void*> freed;
    std::vector<std::string> events;
};

void TestBaseAccessors() {
    Runtime runtime;
    pal4::ActorControlDatabasePrefix state{};
    runtime.state = &state;
    auto* words = state.words.data();
    pal4::ActorControlBase::Construct(words);
    Check(words[0] == pal4::ActorControlBase::kVtable,
          "base constructor writes only the observed vtable");
    Check(pal4::ActorControlBase::SetMember(words, 0x12345678u) ==
              0x12345678u && words[2] == 0x12345678u,
          "base setter writes only field at byte offset eight");
    Check(pal4::ActorControlBase::Delete(words, 2u, runtime) == words &&
              words[0] == pal4::ActorControlBase::kVtable &&
              runtime.freed.empty(),
          "base delete without flag bit retains storage");
    Check(pal4::ActorControlBase::Delete(words, 3u, runtime) == words &&
              runtime.freed == std::vector<void*>{words},
          "base delete with flag bit frees storage");
}

void TestDatabaseLifecycle() {
    Runtime runtime;
    pal4::ActorControlDatabasePrefix state;
    state.words.fill(0xa5a5a5a5u);
    runtime.state = &state;
    Check(&pal4::ActorControlDatabase::Construct(state, runtime) == &state &&
              state.words[0] == pal4::ActorControlDatabase::kVtable &&
              state.words[28] == 0u && state.words[30] == 0u &&
              state.words[31] == 1u && state.words[32] == 1u &&
              state.words[33] == 0u && state.words[34] == 1u &&
              state.words[127] == 0u &&
              state.words[35] == 0x1111u &&
              state.words[128] == 0x2222u &&
              state.words[29] == 0xa5a5a5a5u &&
              state.words[36] == 0xa5a5a5a5u &&
              runtime.events == std::vector<std::string>{
                  "construct resource", "construct name"},
          "database constructor preserves offsets and subobject order");
    runtime.events.clear();
    pal4::ActorControlDatabase::Destruct(state, runtime);
    Check(state.words[0] == pal4::ActorControlBase::kVtable &&
              runtime.events == std::vector<std::string>{
                  "destruct name", "destruct resource"},
          "database destructor releases embedded objects in reverse order");
    runtime.events.clear();
    Check(pal4::ActorControlDatabase::Destroy(&state, 2u,
                                                runtime) == &state &&
              runtime.freed.empty() &&
              runtime.events == std::vector<std::string>{
                  "destruct name", "destruct resource"},
          "database deleting wrapper retains storage without bit zero");
    runtime.events.clear();
    Check(pal4::ActorControlDatabase::Destroy(&state, 3u,
                                                runtime) == &state &&
              runtime.freed == std::vector<void*>{&state} &&
              runtime.events == std::vector<std::string>{
                  "destruct name", "destruct resource", "free"},
          "database deleting wrapper frees after subobject teardown");
}

void TestDummyClumpLoad() {
    Runtime runtime;
    pal4::ActorControlDatabasePrefix state;
    state.words.fill(0xa5a5a5a5u);
    runtime.state = &state;
    Check(pal4::ActorControlDatabase::LoadDummyClump(
              state, runtime) == 1u &&
              state.words[28] == 0u && state.words[29] == 0u &&
              state.words[30] == 0u && state.words[31] == 1u &&
              state.words[32] == 1u && state.words[34] == 1u &&
              state.words[33] == 0xa5a5a5a5u &&
              runtime.events == std::vector<std::string>{
                  "concatenate", "append", "game", "load",
                  "release path"},
          "dummy clump load sets only observed state after resource call");

    Runtime failing;
    pal4::ActorControlDatabasePrefix interrupted;
    interrupted.words.fill(0xa5a5a5a5u);
    failing.state = &interrupted;
    failing.throw_on_load = true;
    try {
        static_cast<void>(pal4::ActorControlDatabase::LoadDummyClump(
            interrupted, failing));
        Check(false, "injected clump load failure must propagate");
    } catch (const std::runtime_error&) {
        Check(interrupted.words[28] == 0xa5a5a5a5u &&
                  failing.events == std::vector<std::string>{
                      "concatenate", "append", "game", "load",
                      "release path"},
              "failed clump load releases path before state writes");
    }
}

void TestClumpUnload() {
    Runtime runtime;
    pal4::ActorControlDatabasePrefix state{};
    runtime.state = &state;
    Check(pal4::ActorControlDatabase::UnloadClump(
              state, 0x1234u, runtime) == 1u &&
              runtime.events == std::vector<std::string>{
                  "game", "cleanup clump", "needs update"},
          "null clump skips HAnim and arena calls");
    state.words[40] = 0x9400u;
    runtime.events.clear();
    Check(pal4::ActorControlDatabase::UnloadClump(
              state, 0x1234u, runtime) == 1u &&
              runtime.checked_clump == 0x9400u &&
              runtime.events == std::vector<std::string>{
                  "hanim", "game", "cleanup clump", "needs update"},
          "clump without HAnim frame is cleaned without arena removal");
    runtime.frame_id = 1u;
    runtime.needs_update = true;
    runtime.events.clear();
    Check(pal4::ActorControlDatabase::UnloadClump(
              state, 0x1234u, runtime) == 1u &&
              runtime.removed_arena == 0x8800u &&
              runtime.removed_clump == 0x9400u &&
              runtime.event_context == 0x1234u &&
              runtime.event_mode == 1u &&
              runtime.events == std::vector<std::string>{
                  "hanim", "arena", "remove clump", "game",
                  "cleanup clump", "needs update", "name event"},
          "HAnim clump removal precedes cleanup and name event");
}

void TestDialogResets() {
    Runtime runtime;
    pal4::ActorControlDatabasePrefix state;
    state.words.fill(0xa5a5a5a5u);
    runtime.state = &state;
    runtime.needs_update = true;
    Check(pal4::ActorControlDatabase::ResetDialog(
              state, 0x1111u, 0x2222u, runtime) == 1u &&
              state.words[28] == 0u && state.words[30] == 0u &&
              state.words[31] == 1u && state.words[127] == 0x2222u &&
              state.words[29] == 0xa5a5a5a5u &&
              runtime.event_context == 0x1111u &&
              runtime.events == std::vector<std::string>{
                  "needs update", "name event"},
          "dialog reset writes selected fields and forwards event context");

    state.words[40] = 0x9500u;
    runtime.events.clear();
    Check(pal4::ActorControlDatabase::ResetWithUi(
              state, 0x3333u, 0x4444u, runtime) == 1u &&
              state.words[28] == 0u && state.words[29] == 0u &&
              state.words[30] == 0u && state.words[31] == 1u &&
              state.words[127] == 0x4444u &&
              runtime.visible_bar == 0x9100u &&
              !runtime.visible_value &&
              runtime.removed_arena == 0x8800u &&
              runtime.removed_clump == 0x9500u &&
              runtime.event_context == 0x3333u &&
              runtime.events == std::vector<std::string>{
                  "ui manager", "ui events", "ui manager", "progress bar",
                  "visible", "arena", "remove clump", "needs update",
                  "name event"},
          "UI reset hides progress and removes clump before name event");
}

}  // namespace

int main() {
    TestBaseAccessors();
    TestDatabaseLifecycle();
    TestDummyClumpLoad();
    TestClumpUnload();
    TestDialogResets();
}
