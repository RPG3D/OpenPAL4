#include "pal4/main_window_visibility.h"

#include <cstdlib>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::MainWindowVisibilityRuntimeStub {
    std::int32_t global_count = 1;
    std::vector<std::string> calls;
    std::vector<std::uint32_t> registered;
    std::vector<std::pair<std::uint32_t, bool>> visibility;

    void SetParent(pal4::MainWindowState&,
                   std::uint32_t showing,
                   std::uint32_t parent) override {
        Check(showing == 0x101u || showing == 0x100u,
              "parent receives full show word");
        Check(parent == 0x7000u, "parent pointer is forwarded");
        calls.emplace_back("parent");
    }
    void RegisterMouse(std::uint32_t member,
                       std::uint32_t window) override {
        Check(member == 0x6028u, "mouse setup uses +0x28 member");
        registered.push_back(window);
        calls.emplace_back("mouse");
    }
    std::uint32_t GlobalData() override {
        calls.emplace_back("global");
        return 0x8000u;
    }
    std::int32_t ReadSignedWord(std::uint32_t address) override {
        Check(address == 0x8028u, "availability count is global +0x28");
        calls.emplace_back("count");
        return global_count;
    }
    void SetVisible(std::uint32_t window, bool visible) override {
        visibility.emplace_back(window, visible);
        calls.emplace_back("visible");
    }
    std::uint32_t FrameManager() override {
        calls.emplace_back("manager");
        return 0x9000u;
    }
    std::int32_t InsertActive(std::uint32_t collection,
                              std::uint32_t member) override {
        Check(collection == 0x9134u && member == 0x6028u,
              "show inserts member into active array +0x134");
        calls.emplace_back("insert");
        return 0x123;
    }
    std::int32_t RemoveActive(std::uint32_t collection,
                              std::uint32_t member) override {
        Check(collection == 0x9134u && member == 0x6028u,
              "hide removes member from active array +0x134");
        calls.emplace_back("remove");
        return -7;
    }
};

pal4::MainWindowState MakeState() {
    pal4::MainWindowState state{};
    state.Write(0x30u, 0x3010u);
    state.Write(0x34u, 0x3400u);
    for (std::uint32_t offset = 0x40u;
         offset <= 0x64u; offset += 4u)
        state.Write(offset, 0x1000u + offset);
    return state;
}

}  // namespace

int main() {
    auto state = MakeState();
    Runtime runtime;
    Check(pal4::MainWindowVisibility::SetVisibility(
              state, 0x6000u, 0x101u, 0x7000u, runtime) == 0x123 &&
              state.Read(0x34u) == 0x3010u &&
              runtime.registered ==
                  (std::vector<std::uint32_t>{
                      0x1040u, 0x1044u, 0x1048u, 0x104cu,
                      0x1050u, 0x1054u, 0x1058u, 0x105cu}) &&
              runtime.visibility ==
                  (std::vector<std::pair<std::uint32_t, bool>>{
                      {0x1064u, true}, {0x105cu, true}}) &&
              runtime.calls.front() == "parent" &&
              runtime.calls.back() == "insert",
          "show connects eight controls and inserts active member");
    runtime = Runtime{};
    runtime.global_count = 0;
    state = MakeState();
    Check(pal4::MainWindowVisibility::SetVisibility(
              state, 0x6000u, 0x101u, 0x7000u, runtime) == 0x123 &&
              runtime.registered.size() == 7u &&
              runtime.visibility ==
                  (std::vector<std::pair<std::uint32_t, bool>>{
                      {0x1064u, false}, {0x105cu, false}}),
          "zero count hides optional controls and omits eighth mouse setup");
    runtime = Runtime{};
    runtime.global_count = -1;
    state = MakeState();
    (void)pal4::MainWindowVisibility::SetVisibility(
        state, 0x6000u, 0x101u, 0x7000u, runtime);
    Check(runtime.registered.size() == 7u &&
              !runtime.visibility[0].second,
          "negative signed count takes hidden branch");
    runtime = Runtime{};
    state = MakeState();
    Check(pal4::MainWindowVisibility::SetVisibility(
              state, 0x6000u, 0x100u, 0x7000u, runtime) == -7 &&
              state.Read(0x34u) == 0x3400u &&
              runtime.calls ==
                  (std::vector<std::string>{
                      "parent", "manager", "remove"}),
          "low show byte selects hide path and preserves menu state");
    std::cout << "MainWindow visibility tests passed\n";
}
