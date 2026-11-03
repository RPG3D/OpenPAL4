#include "pal4/pal_test_window.h"

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

struct Runtime final : pal4::PalTestWindowRuntimeStub {
    std::vector<std::string> calls;
    bool fail_member{};
    bool fail_controls{};

    void LoadLayout(pal4::PalTestWindowState& state,
                    std::uint32_t name, std::uint32_t first,
                    std::uint32_t second) override {
        Check(name == 0x12345678u && first == 0u && second == 0u,
              "layout receives name and zero flags");
        calls.emplace_back("layout");
        state.Write(0x20u, 0x5555u);
    }
    void ConstructMember(pal4::PalTestWindowState& state) override {
        calls.emplace_back("member");
        if (fail_member) throw std::runtime_error("member");
        state.Write(0x30u, 0x6000u);
    }
    void InitializeControls(pal4::PalTestWindowState& state) override {
        Check(state.Read(0u) == 0x8437ecu &&
                  state.Read(0x28u) == 0x8437c4u &&
                  state.Read(0x84u) == 0x843804u &&
                  state.bytes[0x68u] == 0x78u,
              "final vtables and name byte precede controls");
        calls.emplace_back("controls");
        if (fail_controls) throw std::runtime_error("controls");
    }
    void CleanupNameBlock(pal4::PalTestWindowState&) override {
        calls.emplace_back("cleanup-name");
    }
    void CleanupMember(pal4::PalTestWindowState&) override {
        calls.emplace_back("cleanup-member");
    }
    void DestroyDialogBase(pal4::PalTestWindowState&) override {
        calls.emplace_back("destroy-base");
    }
    void Free(std::uint32_t pointer) override {
        calls.emplace_back("free:" + std::to_string(pointer));
    }
    void SetParent(pal4::PalTestWindowState&, std::uint8_t showing,
                   std::uint32_t parent) override {
        Check((showing == 0u || showing == 1u) && parent == 0x7000u,
              "visibility forwards parent");
        calls.emplace_back("parent");
    }
    void ResetUI(pal4::PalTestWindowState&) override {
        calls.emplace_back("reset");
    }
    std::uint32_t FrameManager() override {
        calls.emplace_back("manager");
        return 0x8000u;
    }
    void InsertActive(std::uint32_t array,
                      std::uint32_t member) override {
        Check(array == 0x8134u && member == 0x6028u,
              "show inserts member into active list");
        calls.emplace_back("insert");
    }
    void RemoveActive(std::uint32_t array,
                      std::uint32_t member) override {
        Check(array == 0x8134u && member == 0x6028u,
              "hide removes member from active list");
        calls.emplace_back("remove");
    }
    void SetCurrentQuestion(pal4::PalTestWindowState&,
                            std::uint32_t question) override {
        Check(question == 0u, "show selects first question");
        calls.emplace_back("question");
    }
    void UpdateSystemDialogVisibility(std::uint32_t manager,
                                      std::uint32_t dialog) override {
        Check(manager == 0x8000u && dialog == 11u,
              "show updates system dialog 11");
        calls.emplace_back("system-dialog");
    }
};

void CheckLifecycle() {
    pal4::PalTestWindowState state{};
    Runtime runtime;
    Check(&pal4::PalTestWindow::Construct(state, 0x12345678u, runtime) ==
                  &state && state.Read(0x20u) == 0x5555u &&
                  runtime.calls == std::vector<std::string>{
                      "layout", "member", "controls"},
          "constructor initializes object in IDA order");
    state.Write(0x6cu, 0x5000u);
    state.Write(0x70u, 4u);
    state.Write(0x74u, 8u);
    state.Write(0x34u, 4u);
    state.Write(0x38u, 8u);
    runtime.calls.clear();
    pal4::PalTestWindow::Destroy(state, runtime);
    Check(runtime.calls == std::vector<std::string>{
              "free:20480", "free:24576", "destroy-base"} &&
              state.Read(0x6cu) == 0u && state.Read(0x70u) == 0u &&
              state.Read(0x74u) == 0u && state.Read(0x30u) == 0u &&
              state.Read(0x34u) == 0u && state.Read(0x38u) == 0u,
          "destructor frees both buffers and resets their triples");
    state = {};
    runtime = Runtime{};
    runtime.fail_member = true;
    try {
        (void)pal4::PalTestWindow::Construct(state, 0x12345678u, runtime);
        Check(false, "member failure must propagate");
    } catch (const std::runtime_error&) {
        Check(runtime.calls == std::vector<std::string>{
                  "layout", "member", "destroy-base"},
              "member failure destroys base only");
    }
    state = {};
    runtime = Runtime{};
    runtime.fail_controls = true;
    try {
        (void)pal4::PalTestWindow::Construct(state, 0x12345678u, runtime);
        Check(false, "controls failure must propagate");
    } catch (const std::runtime_error&) {
        Check(runtime.calls == std::vector<std::string>{
                  "layout", "member", "controls", "cleanup-name",
                  "cleanup-member", "destroy-base"},
              "controls failure unwinds three initialized layers");
    }
}

void CheckVisibility() {
    pal4::PalTestWindowState state{};
    Runtime runtime;
    pal4::PalTestWindow::SetVisible(state, 0x6000u, 1u, 0x7000u,
                                    runtime);
    Check(runtime.calls == std::vector<std::string>{
              "parent", "reset", "manager", "insert", "question",
              "manager", "system-dialog"},
          "show follows reset, insertion and question order");
    runtime.calls.clear();
    state.Write(0x3cu, 0x9000u);
    pal4::PalTestWindow::SetVisible(state, 0x6000u, 0u, 0x7000u,
                                    runtime);
    Check(runtime.calls == std::vector<std::string>{
              "parent", "manager", "remove"} &&
              state.Read(0x3cu) == 0u,
          "hide removes member and clears active question state");
}

}  // namespace

int main() {
    CheckLifecycle();
    CheckVisibility();
    std::cout << "Pal test window tests passed\n";
}
