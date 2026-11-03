#include "pal4/script_wait_handler.h"

#include <cmath>
#include <cstdint>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

class Runtime final : public pal4::IScriptWaitHandlerRuntime {
public:
    pal4::ScriptWaitHandlerState state{};
    std::vector<std::string> calls;
    bool allocation_fails = false;
    bool added_null = false;
    int executor_calls = 0;
    int close_calls = 0;
    double expected_duration = 0.0;

    pal4::ScriptWaitHandlerState* Allocate(std::uint32_t size) override {
        Check(size == 0x28u, "wait-handler allocation size");
        calls.emplace_back("allocate");
        return allocation_fails ? nullptr : &state;
    }

    std::uint32_t ScriptExecutor() override {
        calls.emplace_back("executor");
        return 0x5000u + static_cast<std::uint32_t>(executor_calls++) * 0x1000u;
    }

    void ConstructBase(pal4::ScriptWaitHandlerState& handler,
                       std::uint32_t wait_mode,
                       std::uint32_t queue_address) override {
        Check(&handler == &state && wait_mode == 1u &&
                  queue_address == 0x5054u,
              "blocking base constructor and first queue address");
        state.vtable_address = 0xdeadbeefu;
        calls.emplace_back("construct");
    }

    void Flush(pal4::ScriptWaitHandlerState& handler) override {
        Check(&handler == &state &&
                  handler.vtable_address == 0x849514u &&
                  handler.remaining == expected_duration,
              "flush observes duration and derived vtable");
        calls.emplace_back("flush");
    }

    void AddHandler(std::uint32_t queue_address,
                    pal4::ScriptWaitHandlerState* handler) override {
        Check(queue_address == (allocation_fails ? 0x5054u : 0x6054u),
              "handler list uses a fresh executor lookup");
        Check(handler == (allocation_fails ? nullptr : &state),
              "handler list receives allocation result even on failure");
        added_null = handler == nullptr;
        calls.emplace_back("add");
    }

    int SetExecutionFlag(std::uint32_t queue_address) override {
        Check(queue_address == (allocation_fails ? 0x6054u : 0x7054u),
              "execution flag uses its own executor lookup");
        calls.emplace_back("flag");
        return -31;
    }

    void SetClosedFlag(pal4::ScriptWaitHandlerState& handler) override {
        Check(&handler == &state && handler.remaining == 0.0,
              "close clears remaining duration before setting flag");
        ++close_calls;
        calls.emplace_back("close");
    }
};

void CreateAndQueue() {
    Runtime runtime;
    runtime.expected_duration = 3.25;
    pal4::ScriptWaitHandler wait(runtime);
    Check(wait.Create(3.25) == -31, "factory returns execution-flag result");
    Check(runtime.calls == std::vector<std::string>{
              "allocate", "executor", "construct", "flush", "executor",
              "add", "executor", "flag"},
          "successful factory call order");
    Check(runtime.executor_calls == 3 && !runtime.added_null &&
              runtime.state.remaining == 3.25,
          "three executor acquisitions and exact double duration");
}

void AllocationFailureStillQueuesAndFlags() {
    Runtime runtime;
    runtime.allocation_fails = true;
    pal4::ScriptWaitHandler wait(runtime);
    Check(wait.Create(9.0) == -31, "failure path forwards final result");
    Check(runtime.calls == std::vector<std::string>{
              "allocate", "executor", "add", "executor", "flag"} &&
              runtime.executor_calls == 2 && runtime.added_null,
          "allocation failure still queues null and sets execution flag");
}

void TimingAndClose() {
    Runtime runtime;
    pal4::ScriptWaitHandler wait(runtime);
    runtime.state.remaining = 2.5;
    wait.Update(runtime.state, 1.0);
    Check(runtime.state.remaining == 1.5 && runtime.close_calls == 0,
          "positive remainder stays open");
    wait.Update(runtime.state, 1.5);
    Check(runtime.state.remaining == 0.0 && runtime.close_calls == 1,
          "exact zero closes handler");
    runtime.state.remaining = 0.25;
    wait.Update(runtime.state, 1.0);
    Check(runtime.state.remaining == 0.0 && runtime.close_calls == 2,
          "negative remainder closes and clamps to zero");
    runtime.state.remaining = std::numeric_limits<double>::quiet_NaN();
    wait.Update(runtime.state, 1.0);
    Check(std::isnan(runtime.state.remaining) && runtime.close_calls == 2,
          "unordered floating-point comparison does not close");
    runtime.state.remaining = 8.0;
    wait.Close(runtime.state);
    Check(runtime.state.remaining == 0.0 && runtime.close_calls == 3,
          "explicit close zeros duration before setting flag");
}

}  // namespace

int main() {
    try {
        CreateAndQueue();
        AllocationFailureStillQueuesAndFlags();
        TimingAndClose();
        pal4::ScriptWaitHandlerRuntimeStub stub;
        pal4::ScriptWaitHandler wait(stub);
        Check(wait.Create(1.0) == 0, "linkable native-boundary stub");
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
