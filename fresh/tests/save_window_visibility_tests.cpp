#include "pal4/save_window_visibility.h"

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool ok, const char* message) {
    if (!ok) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::SaveWindowVisibilityRuntimeStub {
    std::vector<std::string> calls;
    std::int32_t save_index{37};
    std::uint32_t current_state{0x9000u};
    std::uint32_t state_id{1u};
    bool fail_list{};

    void SetParent(pal4::SaveWindowState&, std::uint8_t showing,
                   std::uint32_t parent) override {
        Check(parent == 0x7000u && showing <= 1u,
              "parent receives save visibility");
        calls.emplace_back("parent");
    }
    std::int32_t GlobalSaveIndex() override {
        calls.emplace_back("save-count");
        return save_index;
    }
    void PopulateSaveList(pal4::SaveWindowState&, std::int32_t page,
                          bool reset) override {
        Check(page == save_index / 10 && reset,
              "save list receives count divided by ten and reset flag");
        calls.emplace_back("populate");
        if (fail_list) throw std::runtime_error("populate");
    }
    std::uint32_t CurrentState() override {
        calls.emplace_back("current_state");
        return current_state;
    }
    std::uint32_t StateId(std::uint32_t object) override {
        Check(object == current_state, "current_state virtual status uses current object");
        calls.emplace_back("state-id");
        return state_id;
    }
    std::uint32_t FrameManager() override {
        calls.emplace_back("manager");
        return 0x8000u;
    }
    void CaptureActiveCameraRaster(
        std::uint32_t manager) override {
        Check(manager == 0x8000u, "current_state init uses UIFrameManager");
        calls.emplace_back("capture-raster");
    }
    void NotifyPropertyReturn(std::uint32_t event_tree,
                            std::uint32_t first,
                            std::uint32_t second) override {
        Check(event_tree == 0x8104u && first == 0u && second == 0u,
              "current_state exit uses manager event tree and two zero words");
        calls.emplace_back("property-return");
    }
    void SetAlwaysOnTop(std::uint32_t window, bool enabled) override {
        Check(window == 0x5000u && enabled,
              "save root window always on top");
        calls.emplace_back("top");
    }
    std::uint32_t InsertActive(std::uint32_t list,
                               std::uint32_t member) override {
        Check(list == 0x8134u && member == 0x6028u,
              "show inserts member in manager active list");
        calls.emplace_back("insert");
        return 0xaau;
    }
    std::uint32_t RemoveActive(std::uint32_t list,
                               std::uint32_t member) override {
        Check(list == 0x8134u && member == 0x6028u,
              "hide removes member from manager active list");
        calls.emplace_back("remove");
        return 0xbbu;
    }
};

pal4::SaveWindowState State() {
    pal4::SaveWindowState state{};
    state.Write(0x24u, 0x5000u);
    return state;
}

void CheckShow() {
    auto state = State();
    Runtime runtime;
    Check(pal4::SaveWindowVisibility::SetVisible(
              state, 0x6000u, 1u, 0x7000u, runtime) == 0xaau &&
              runtime.calls == std::vector<std::string>{
                  "parent", "save-count", "populate", "current_state",
                  "state-id", "manager", "capture-raster", "top",
                  "manager", "insert"},
          "show populates page and initializes inactive current_state");
    runtime = Runtime{};
    runtime.state_id = 2u;
    (void)pal4::SaveWindowVisibility::SetVisible(
        state, 0x6000u, 1u, 0x7000u, runtime);
    Check(runtime.calls == std::vector<std::string>{
              "parent", "save-count", "populate", "current_state",
              "state-id", "top", "manager", "insert"},
          "active current_state skips initialization");
    runtime = Runtime{};
    runtime.current_state = 0u;
    (void)pal4::SaveWindowVisibility::SetVisible(
        state, 0x6000u, 1u, 0x7000u, runtime);
    Check(runtime.calls == std::vector<std::string>{
              "parent", "save-count", "populate", "current_state", "top",
              "manager", "insert"},
          "missing current_state skips status query");
    runtime = Runtime{};
    runtime.save_index = -19;
    runtime.current_state = 0u;
    (void)pal4::SaveWindowVisibility::SetVisible(
        state, 0x6000u, 1u, 0x7000u, runtime);
    Check(runtime.calls[1] == "save-count" &&
              runtime.calls[2] == "populate",
          "negative save count preserves signed page division");
}

void CheckHide() {
    auto state = State();
    Runtime runtime;
    runtime.state_id = 2u;
    Check(pal4::SaveWindowVisibility::SetVisible(
              state, 0x6000u, 0u, 0x7000u, runtime) == 0xbbu &&
              runtime.calls == std::vector<std::string>{
                  "parent", "current_state", "state-id", "manager",
                  "property-return", "top", "manager", "remove"},
          "hide dispatches current_state exit only for active current_state");
    runtime = Runtime{};
    (void)pal4::SaveWindowVisibility::SetVisible(
        state, 0x6000u, 0u, 0x7000u, runtime);
    Check(runtime.calls == std::vector<std::string>{
              "parent", "current_state", "state-id", "top", "manager",
              "remove"},
          "other current_state state skips exit event");
    runtime = Runtime{};
    runtime.current_state = 0u;
    (void)pal4::SaveWindowVisibility::SetVisible(
        state, 0x6000u, 0u, 0x7000u, runtime);
    Check(runtime.calls == std::vector<std::string>{
              "parent", "current_state", "top", "manager", "remove"},
          "missing current_state skips exit event");
}

void CheckFailure() {
    auto state = State();
    Runtime runtime;
    runtime.fail_list = true;
    try {
        (void)pal4::SaveWindowVisibility::SetVisible(
            state, 0x6000u, 1u, 0x7000u, runtime);
        Check(false, "list failure propagates");
    } catch (const std::runtime_error&) {
        Check(runtime.calls == std::vector<std::string>{
                  "parent", "save-count", "populate"},
              "list failure precedes current_state and active-list operations");
    }
}

}  // namespace

int main() {
    CheckShow();
    CheckHide();
    CheckFailure();
    std::cout << "Save window visibility tests passed\n";
}
