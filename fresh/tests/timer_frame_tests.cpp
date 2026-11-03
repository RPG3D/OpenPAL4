#include "pal4/timer_frame.h"
#include "pal4/timer_frame_buttons.h"

#include <cstdlib>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::ITimerFrameRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> calls;
    std::uint32_t allocation = 0x4000u;
    std::uint32_t free_address = 0u;
    std::uint32_t event_manager = 0u;
    std::uint32_t event_name = 0u;
    std::uint32_t event_create = 0u;
    std::string throw_at;

    void MaybeThrow(const char* point) {
        if (throw_at == point) throw std::runtime_error(point);
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.emplace_back("read");
        return words.at(address);
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        calls.emplace_back("write");
        words[address] = value;
    }
    std::uint32_t Allocate(std::uint32_t bytes) override {
        Check(bytes == pal4::TimerFrame::kSize,
              "frame allocation has observed 120-byte size");
        calls.emplace_back("allocate");
        return allocation;
    }
    void Free(std::uint32_t address) override {
        calls.emplace_back("free");
        free_address = address;
    }
    std::uint32_t FrameManager() override {
        calls.emplace_back("manager");
        return 0x6000u;
    }
    std::uint32_t ProcessEvent(std::uint32_t manager,
                               std::uint32_t script_name,
                               std::uint32_t create_function) override {
        calls.emplace_back("process");
        event_manager = manager;
        event_name = script_name;
        event_create = create_function;
        return 0x7000u;
    }
    void LoadLayout(std::uint32_t object, std::uint32_t name,
                    std::uint32_t first_flag,
                    std::uint32_t second_flag) override {
        Check(object == 0x4000u && name == 0x5000u &&
                  first_flag == 0u && second_flag == 0u,
              "layout receives frame, name and zero flags");
        calls.emplace_back("load");
        MaybeThrow("load");
    }
    void ConstructMember(std::uint32_t member) override {
        Check(member == 0x4028u, "member offset");
        calls.emplace_back("member");
        MaybeThrow("member");
    }
    void CleanupMember(std::uint32_t member) override {
        Check(member == 0x4028u, "member cleanup offset");
        calls.emplace_back("cleanup-member");
    }
    void ConstructVector2(std::uint32_t vector) override {
        Check(vector == 0x4040u || vector == 0x4048u ||
                  vector == 0x4050u,
              "vector offset belongs to three-element sequence");
        calls.emplace_back("vector:" + std::to_string(vector));
        MaybeThrow("vector");
    }
    void InitializeButtons(std::uint32_t object) override {
        Check(object == 0x4000u, "button initializer receives frame");
        calls.emplace_back("buttons");
        MaybeThrow("buttons");
    }
    std::uint32_t DestroyDialog(std::uint32_t object) override {
        Check(object == 0x4000u, "base destructor receives frame");
        calls.emplace_back("destroy-dialog");
        return 0x7777u;
    }
};

struct ButtonsRuntime final : pal4::ITimerFrameButtonsRuntime {
    std::vector<std::string> names;
    std::uint32_t end_count = 0u;
    std::uint32_t BeginName(std::uint32_t frame) override {
        Check(frame == 0x4000u, "composed buttons receive constructed frame");
        return 0x9000u;
    }
    std::uint32_t ResetName(std::uint32_t buffer,
                            std::uint32_t frame,
                            std::string_view suffix) override {
        Check(buffer == 0x9000u && frame == 0x4000u,
              "composed name buffer and frame");
        names.emplace_back(suffix);
        return 0x9001u;
    }
    std::uint32_t EndName(std::uint32_t buffer) override {
        Check(buffer == 0x9000u, "composed name cleanup");
        ++end_count;
        return 0u;
    }
    std::uint32_t FindWindow(std::uint32_t) override { return 0x8000u; }
    std::uint32_t Allocate(std::uint32_t bytes) override {
        Check(bytes == 0x18u, "composed callback allocation size");
        return 0u;
    }
    std::uint32_t ConstructCallback(std::uint32_t,
                                    std::uint32_t,
                                    std::uint32_t) override {
        Check(false, "null callback allocation skips constructor");
        return 0u;
    }
    void Free(std::uint32_t) override {}
    std::uint32_t Subscribe(std::uint32_t,
                            std::uint32_t,
                            std::uint32_t) override { return 0u; }
    void ReleaseConnection(std::uint32_t) override {}
    std::uint32_t TriggerText(std::uint32_t) override { return 0u; }
    std::string_view ToUtf8(std::uint32_t) override { return {}; }
    void SetText(std::uint32_t, std::string_view) override {}
    std::array<std::uint32_t, 2> Position(std::uint32_t) override {
        return {0u, 0u};
    }
    std::uint32_t HeightBits(std::uint32_t) override { return 0u; }
    std::uint32_t ReadWord(std::uint32_t) override { return 0u; }
    void WriteWord(std::uint32_t, std::uint32_t) override {}
};

void CheckScriptNameAndFactory() {
    Runtime runtime;
    runtime.words[pal4::TimerFrame::kScriptNameSource] = 0x1234u;
    Check(pal4::TimerFrame::GetScriptName(runtime) == 0x1234u &&
              runtime.words[pal4::TimerFrame::kScriptNameStorage] ==
                  0x1234u &&
              runtime.calls ==
                  std::vector<std::string>{"read", "write"},
          "script name thunk copies source pointer into global storage");
    runtime.calls.clear();
    runtime.words[pal4::TimerFrame::kScriptNameSource] = 0x5678u;
    Check(pal4::TimerFrame::InitializeScriptName(runtime) == 0x5678u &&
              runtime.words[pal4::TimerFrame::kScriptNameStorage] ==
                  0x5678u,
          "script name initializer rereads source on each call");

    runtime.calls.clear();
    Check(pal4::TimerFrame::GetFactory(runtime) == 0x7000u &&
              runtime.event_manager == 0x6000u &&
              runtime.event_name == 0x5678u &&
              runtime.event_create ==
                  pal4::TimerFrame::kCreateFunction &&
              runtime.calls ==
                  std::vector<std::string>{"manager", "read", "process"},
          "factory thunk passes stored name and create entry to manager");
}

void CheckConstruction() {
    Runtime runtime;
    Check(pal4::TimerFrame::Create(0x5000u, runtime) == 0x4000u &&
              runtime.words[0x4000u] ==
                  pal4::TimerFrame::kObjectVtable &&
              runtime.words[0x4028u] ==
                  pal4::TimerFrame::kMemberVtable &&
              runtime.words[0x4070u] == 0u &&
              runtime.calls == std::vector<std::string>{
                                   "allocate", "load", "member", "write",
                                   "vector:16448", "vector:16456",
                                   "vector:16464", "write", "write",
                                   "buttons", "write"},
          "frame construction follows layout, member, vector and button order");

    Runtime failed_allocation;
    failed_allocation.allocation = 0u;
    Check(pal4::TimerFrame::Create(0x5000u, failed_allocation) == 0u &&
              failed_allocation.calls ==
                  std::vector<std::string>{"allocate"},
          "failed allocation skips constructor");
}

void CheckComposedConstruction() {
    Runtime frame_runtime;
    ButtonsRuntime buttons_runtime;
    Check(pal4::TimerFrame::Create(
              0x5000u, frame_runtime, buttons_runtime) == 0x4000u &&
              buttons_runtime.end_count == 1u &&
              buttons_runtime.names == std::vector<std::string>{
                  "BtnSettle", "BtnDelegate", "BtnExit",
                  "PanelButton", "imgBottom"} &&
              frame_runtime.words[0x4070u] == 0u &&
              frame_runtime.calls.size() == 10u,
          "frame constructor composes rebuilt button initializer");
}

void CheckFailureCleanup(const char* stage,
                         const std::vector<std::string>& expected_tail) {
    Runtime runtime;
    runtime.throw_at = stage;
    bool caught = false;
    try {
        (void)pal4::TimerFrame::Create(0x5000u, runtime);
    } catch (const std::runtime_error&) {
        caught = true;
    }
    Check(caught && runtime.free_address == 0x4000u &&
              runtime.calls.size() >= expected_tail.size(),
          "constructor exception frees allocated frame");
    const auto start = runtime.calls.end() -
                       static_cast<std::ptrdiff_t>(expected_tail.size());
    Check(std::vector<std::string>(start, runtime.calls.end()) ==
              expected_tail,
          "constructor exception cleanup follows SEH stage");
}

void CheckDestruction() {
    Runtime runtime;
    runtime.words[0x4030u] = 0x8000u;
    Check(pal4::TimerFrame::Destroy(0x4000u, runtime) == 0x7777u &&
              runtime.free_address == 0x8000u &&
              runtime.words[0x4030u] == 0u &&
              runtime.words[0x4034u] == 0u &&
              runtime.words[0x4038u] == 0u &&
              runtime.calls == std::vector<std::string>{
                                   "read", "free", "write", "write",
                                   "write", "destroy-dialog"},
          "destructor frees member data, clears triplet and destroys dialog");
}

}  // namespace

int main() {
    CheckScriptNameAndFactory();
    CheckConstruction();
    CheckComposedConstruction();
    CheckFailureCleanup("load", {"load", "free"});
    CheckFailureCleanup("member", {"member", "destroy-dialog", "free"});
    CheckFailureCleanup("vector", {"vector:16448", "cleanup-member",
                                   "destroy-dialog", "free"});
    CheckFailureCleanup("buttons", {"buttons", "cleanup-member",
                                    "destroy-dialog", "free"});
    CheckDestruction();
}
