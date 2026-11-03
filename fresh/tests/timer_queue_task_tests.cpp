#include "pal4/timer_queue_task.h"

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <map>
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

struct NameRuntime final : pal4::ITimerQueueEntryRuntime {
    std::vector<std::string>& calls;
    std::uint8_t count = 2u;

    explicit NameRuntime(std::vector<std::string>& calls) : calls(calls) {}
    void EraseName(pal4::TimerQueueEntryState&,
                   std::uint32_t, std::uint32_t) override {}
    void SetTextFlag(pal4::TimerQueueEntryState&,
                     std::uint8_t) override {}
    std::uint8_t ReadReferenceCount(std::uint32_t data) override {
        Check(data == 0x6001u, "local name data pointer");
        calls.emplace_back("read-name-count");
        return count;
    }
    void WriteReferenceCount(std::uint32_t data,
                             std::uint8_t value) override {
        Check(data == 0x6001u, "local name reference update");
        count = value;
        calls.emplace_back("write-name-count");
    }
    bool AllocateName(pal4::TimerQueueEntryState&,
                      std::uint32_t, std::uint8_t) override { return false; }
    void CopyName(std::uint32_t, std::uint32_t,
                  std::uint32_t) override {}
    void SetNameSize(pal4::TimerQueueEntryState&,
                     std::uint32_t) override {}
    void ReleaseCallback(std::uint32_t callback,
                         std::uint32_t deleting) override {
        Check(callback == 0x5000u && deleting == 1u,
              "exceptional callback destructor");
        calls.emplace_back("destroy-callback");
    }
    std::uint32_t FreeName(std::uint32_t header) override {
        Check(header == 0x6000u, "local name header");
        calls.emplace_back("free-name");
        return 0x1234u;
    }
};

struct Runtime final : pal4::ITimerQueueTaskRuntime {
    std::vector<std::string> calls;
    NameRuntime name_runtime{calls};
    std::map<std::uint32_t, std::uint32_t> memory{
        {0x1004u, 0x2000u}, {0x2004u, 0x3000u}, {0x1008u, 2u}};
    bool fail_callback_allocation = false;
    bool fail_node_allocation = false;
    bool throw_assign = false;
    bool throw_copy_name = false;
    std::uint32_t allocation_count = 0u;

    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.emplace_back("read");
        return memory[address];
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        calls.emplace_back("write");
        memory[address] = value;
    }
    std::uint32_t Allocate(std::uint32_t bytes) override {
        ++allocation_count;
        calls.emplace_back("allocate");
        if (allocation_count == 1u) {
            Check(bytes == 0x10u, "callback allocation size");
            return fail_callback_allocation ? 0u : 0x5000u;
        }
        Check(bytes == 0x28u, "task node allocation size");
        return fail_node_allocation ? 0u : 0x4000u;
    }
    void CopyName(pal4::TimerQueueEntryState& entry,
                  std::uint32_t source) override {
        Check(source == 0x7000u, "task name source");
        entry.name.data = 0x6001u;
        calls.emplace_back("copy-name");
        if (throw_copy_name) {
            throw std::runtime_error("copy timer name");
        }
    }
    void AssignTimerData(std::uint32_t destination,
                         const pal4::TimerQueueEntryState& entry) override {
        Check(destination == 0x4008u &&
                  entry.name.data == 0x6001u &&
                  entry.initial_period_bits == 0x3f800000u &&
                  entry.current_period_bits == 0x3f800000u &&
                  entry.owns_callback == !fail_callback_allocation &&
                  entry.callback ==
                      (fail_callback_allocation ? 0u : 0x5000u),
              "assigned task entry fields");
        calls.emplace_back("assign-data");
        if (throw_assign) {
            throw std::runtime_error("assign timer data");
        }
    }
    void ReleaseCallback(std::uint32_t callback,
                         std::uint32_t deleting) override {
        Check(callback == 0x5000u && deleting == 1u,
              "success-path callback release");
        calls.emplace_back("release-callback");
    }
    pal4::ITimerQueueEntryRuntime& EntryRuntime() override {
        return name_runtime;
    }
};

void Create(Runtime& runtime, std::uint32_t function = 0x8888u) {
    pal4::TimerQueueTask::Create(0x1000u, 0x7000u, 0x3f800000u,
                                 0x9000u, function, 0xabu, runtime);
}

void SchedulePuzzle(Runtime& runtime,
                    std::uint32_t function = 0x4a1710u) {
    pal4::TimerQueueTask::SchedulePuzzle(
        0x1000u, 0x7000u, 0x3f800000u,
        0x9000u, function, 0xabu, runtime);
}

struct InvocationRuntime final : pal4::ITimerCallbackInvocationRuntime {
    std::map<std::uint32_t, std::uint32_t> words{
        {0x5004u, 0xfffffff0u}, {0x5008u, 0x12345678u},
        {0x500cu, 0x20u}};
    std::vector<std::uint32_t> reads;
    std::uint32_t function = 0u;
    std::uint32_t context = 0u;

    std::uint32_t ReadWord(std::uint32_t address) override {
        reads.push_back(address);
        return words.at(address);
    }
    std::uint32_t InvokeThiscall(std::uint32_t target,
                                 std::uint32_t this_pointer) override {
        function = target;
        context = this_pointer;
        return 0xa5u;
    }
};

}  // namespace

int main() {
    {
        InvocationRuntime runtime;
        Check(pal4::TimerCallbackInvocation::InvokeWithOffset(
                  0x5000u, runtime) == 0xa5u &&
                  runtime.function == 0x12345678u &&
                  runtime.context == 0x10u &&
                  runtime.reads == (std::vector<std::uint32_t>{
                      0x500cu, 0x5004u, 0x5008u}),
              "callback tail call adds context offset in 32-bit space");
    }
    {
        Runtime runtime;
        Create(runtime, 0u);
        Check(runtime.calls.empty() && runtime.memory[0x1008u] == 2u,
              "null callback function skips all task work");
    }
    {
        Runtime runtime;
        Create(runtime);
        Check(runtime.memory[0x5000u] ==
                  pal4::TimerQueueTask::kCallbackVtable &&
                  runtime.memory[0x5004u] == 0x9000u &&
                  runtime.memory[0x5008u] == 0x8888u &&
                  runtime.memory[0x500cu] == 0xabu &&
                  runtime.memory[0x4000u] == 0x2000u &&
                  runtime.memory[0x4004u] == 0x3000u &&
                  runtime.memory[0x2004u] == 0x4000u &&
                  runtime.memory[0x3000u] == 0x4000u &&
                  runtime.memory[0x1008u] == 3u &&
                  runtime.name_runtime.count == 1u &&
                  runtime.calls[runtime.calls.size() - 3u] ==
                      "release-callback" &&
                  runtime.calls[runtime.calls.size() - 2u] ==
                      "read-name-count" &&
                  runtime.calls.back() == "write-name-count",
              "callback, task links, count and temporary ownership");
    }
    {
        Runtime runtime;
        runtime.memory[0x2004u] = 0u;
        runtime.fail_callback_allocation = true;
        Create(runtime);
        Check(runtime.memory[0x4000u] == 0x4000u &&
                  runtime.memory[0x4004u] == 0x4000u &&
                  runtime.memory[0x1008u] == 3u &&
                  runtime.name_runtime.count == 1u &&
                  std::find(runtime.calls.begin(), runtime.calls.end(),
                            "release-callback") == runtime.calls.end(),
              "callback allocation failure still inserts self-linked task");
    }
    {
        Runtime runtime;
        runtime.fail_node_allocation = true;
        Create(runtime);
        Check(runtime.memory[0x2004u] == 0x3000u &&
                  runtime.memory[0x1008u] == 2u &&
                  runtime.name_runtime.count == 1u &&
                  runtime.calls[runtime.calls.size() - 3u] ==
                      "destroy-callback",
              "node allocation failure cleans local task safely");
    }
    {
        Runtime runtime;
        runtime.throw_assign = true;
        bool caught = false;
        try {
            Create(runtime);
        } catch (const std::runtime_error&) {
            caught = true;
        }
        Check(caught && runtime.memory[0x2004u] == 0x4000u &&
                  runtime.memory[0x1008u] == 2u &&
                  runtime.name_runtime.count == 1u &&
                  runtime.calls[runtime.calls.size() - 3u] ==
                      "destroy-callback",
              "assign failure retains original partial link and cleans local data");
    }
    {
        Runtime runtime;
        SchedulePuzzle(runtime);
        Check(runtime.memory[0x5000u] ==
                  pal4::TimerQueueTask::kPuzzleCallbackVtable &&
                  runtime.memory[0x5004u] == 0x9000u &&
                  runtime.memory[0x5008u] == 0x4a1710u &&
                  runtime.memory[0x500cu] == 0xabu &&
                  runtime.memory[0x1008u] == 3u &&
                  runtime.name_runtime.count == 1u,
              "puzzle scheduler uses its own callback vtable and shared queue path");
    }
    {
        Runtime runtime;
        SchedulePuzzle(runtime, 0u);
        Check(runtime.calls.empty() && runtime.memory[0x1008u] == 2u,
              "puzzle scheduler skips null function");
    }
    {
        Runtime runtime;
        runtime.throw_copy_name = true;
        bool caught = false;
        try {
            SchedulePuzzle(runtime);
        } catch (const std::runtime_error&) {
            caught = true;
        }
        Check(caught && runtime.memory[0x2004u] == 0x3000u &&
                  runtime.memory[0x1008u] == 2u &&
                  runtime.name_runtime.count == 1u &&
                  runtime.calls[runtime.calls.size() - 3u] ==
                      "destroy-callback" &&
                  runtime.calls.back() == "write-name-count",
              "partial name-copy failure releases temporary callback and name");
    }
}
