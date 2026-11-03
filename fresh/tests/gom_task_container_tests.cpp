#include "pal4/gom_task_container.h"
#include "pal4/gom_task_base.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <iostream>
#include <map>
#include <new>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

class Runtime final : public pal4::IGomTaskContainerRuntime {
public:
    std::uint32_t next_allocation = 0x1000u;
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, pal4::GomTaskBaseState*> tasks;
    std::vector<std::string> events;
    std::vector<std::uint32_t> frees;
    std::vector<std::uint32_t> deleted_tasks;
    std::uint32_t allocated_bytes = 0u;
    bool wait_complete = false;
    bool duration_complete = false;

    std::uint32_t AllocateMemory(std::uint32_t bytes) override {
        allocated_bytes = bytes;
        events.emplace_back("allocate");
        return next_allocation;
    }
    std::uint32_t FreeMemory(std::uint32_t address) override {
        frees.push_back(address);
        events.emplace_back("free");
        return address == 0x1000u ? 0x77u : 0x33u;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        events.emplace_back("read");
        return words.at(address);
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        events.emplace_back("write");
        words[address] = value;
        for (const auto& [task_address, task] : tasks) {
            if (address == task_address + 0x28u) task->phase = value;
            if (address == task_address + 0x34u) task->next = value;
        }
    }
    void DeleteTask(std::uint32_t task, std::uint32_t flags) override {
        Check(flags == 1u, "task deleting destructor receives flag one");
        deleted_tasks.push_back(task);
        events.emplace_back("delete");
    }
    pal4::GomTaskBaseState& TaskAt(std::uint32_t address) override {
        return *tasks.at(address);
    }
    bool IsWaitComplete(std::uint32_t) override {
        events.emplace_back("wait");
        return wait_complete;
    }
    void BeginRun(std::uint32_t) override { events.emplace_back("begin"); }
    void Tick(std::uint32_t, float) override { events.emplace_back("tick"); }
    bool IsDurationComplete(std::uint32_t) override {
        events.emplace_back("duration");
        return duration_complete;
    }
    void Finish(std::uint32_t) override { events.emplace_back("finish"); }
};

class BaseRuntime final : public pal4::IGomTaskBaseRuntime {
public:
    std::uint32_t next_allocation = 0x9000u;
    std::uint32_t copied_bytes = 0u;
    std::string copied_name;
    std::uint32_t AllocateMemory(std::uint32_t bytes) override {
        copied_bytes = bytes;
        return next_allocation;
    }
    std::uint32_t FreeMemory(std::uint32_t) override { return 0u; }
    std::uint32_t ReadWord(std::uint32_t) override { return 0u; }
    void CopyCString(std::uint32_t, const char* source,
                     std::uint32_t bytes) override {
        copied_name.assign(source, bytes);
    }
};

void EmptyContainer() {
    Runtime runtime;
    BaseRuntime base_runtime;
    pal4::GomTaskBase base(base_runtime);
    pal4::GomTaskContainer container(runtime, base);
    pal4::GomTaskContainerState state{};
    state.padding = {0xaau, 0xbbu, 0xccu};
    Check(&container.Initialize(state, 0x5au) == &state &&
              state.opaque_byte == 0x5au &&
              state.padding == std::array<std::uint8_t, 3>{0xaau, 0xbbu, 0xccu} &&
              state.sentinel == 0x1000u && state.count == 0u &&
              state.ready == 1u && runtime.allocated_bytes == 12u &&
              runtime.words.at(0x1000u) == 0x1000u &&
              runtime.words.at(0x1004u) == 0x1000u &&
              runtime.events ==
                  std::vector<std::string>{"allocate", "write", "write"},
          "init creates a self-linked 12-byte sentinel and observed state");
    runtime.events.clear();
    Check(container.BeginWaiting(state) == 0u && state.ready == 1u,
          "empty begin-wait leaves readiness set");
    runtime.events.clear();
    Check(container.Destroy(state) == 0x77u &&
              runtime.events ==
                  std::vector<std::string>{"read", "read", "read", "free"} &&
              runtime.frees == std::vector<std::uint32_t>{0x1000u} &&
              state.sentinel == 0u && state.count == 0u &&
              state.ready == 1u && state.opaque_byte == 0x5au,
          "empty destroy cleans objects, frees sentinel and preserves flags");
}

void PopulatedContainer() {
    Runtime runtime;
    BaseRuntime base_runtime;
    pal4::GomTaskBase base(base_runtime);
    pal4::GomTaskContainer container(runtime, base);
    pal4::GomTaskContainerState state{};
    static_cast<void>(container.Initialize(state, 0x12u));
    runtime.words[0x1000u] = 0x2000u;
    runtime.words[0x1004u] = 0x3000u;
    runtime.words[0x2000u] = 0x3000u;
    runtime.words[0x2004u] = 0x1000u;
    runtime.words[0x2008u] = 0u;
    runtime.words[0x3000u] = 0x1000u;
    runtime.words[0x3004u] = 0x2000u;
    runtime.words[0x3008u] = 0u;
    state.count = 2u;
    runtime.events.clear();
    Check(container.Destroy(state) == 0x77u &&
              runtime.frees ==
                  std::vector<std::uint32_t>{0x2000u, 0x3000u, 0x1000u} &&
              runtime.words.at(0x1000u) == 0x1000u &&
              runtime.words.at(0x1004u) == 0x1000u &&
              state.sentinel == 0u && state.count == 0u &&
              runtime.deleted_tasks.empty(),
          "destroy unlinks nodes in forward order before freeing sentinel");
}

void AllocationFailure() {
    Runtime runtime;
    runtime.next_allocation = 0u;
    BaseRuntime base_runtime;
    pal4::GomTaskBase base(base_runtime);
    pal4::GomTaskContainer container(runtime, base);
    pal4::GomTaskContainerState state{};
    state.sentinel = 0x5555u;
    bool threw = false;
    try {
        static_cast<void>(container.Initialize(state, 0x33u));
    } catch (const std::bad_alloc&) {
        threw = true;
    }
    Check(threw && state.opaque_byte == 0x33u &&
              state.sentinel == 0x5555u &&
              runtime.events == std::vector<std::string>{"allocate"},
          "failed allocation leaves existing list state untouched");
}

void AppendTasks() {
    Runtime runtime;
    BaseRuntime base_runtime;
    pal4::GomTaskBase base(base_runtime);
    pal4::GomTaskContainer container(runtime, base);
    pal4::GomTaskContainerState state{};
    static_cast<void>(container.Initialize(state, 0u));
    pal4::GomTaskBaseState first{};
    pal4::GomTaskBaseState second{};
    runtime.tasks[0x5000u] = &first;
    runtime.tasks[0x6000u] = &second;
    pal4::GomTaskTimingConfig timing{2u, 1.5f, 4.0f};
    runtime.next_allocation = 0x2000u;
    Check(container.AddTaskAndInitialize(
              state, 0x8000u, first, 0x5000u, "first", timing) == 1u &&
              runtime.allocated_bytes == 12u &&
              runtime.words.at(0x1000u) == 0x2000u &&
              runtime.words.at(0x1004u) == 0x2000u &&
              runtime.words.at(0x2000u) == 0x1000u &&
              runtime.words.at(0x2004u) == 0x1000u &&
              runtime.words.at(0x2008u) == 0x5000u &&
              first.previous == 0u && first.next == 0u &&
              first.context == 0x8000u && first.wait_mode == 2u &&
              first.wait_time == 1.5f && first.expect_time == 4.0f &&
              base_runtime.copied_name == std::string("first\0", 6),
          "first task initializes base and links before sentinel");

    runtime.next_allocation = 0x3000u;
    Check(container.AddTaskAndInitialize(
              state, 0x8000u, second, 0x6000u, "second", timing) == 2u &&
              runtime.words.at(0x5034u) == 0x6000u &&
              first.next == 0x6000u &&
              second.previous == 0x5000u && second.next == 0u &&
              second.context == 0x8000u &&
              runtime.words.at(0x1000u) == 0x2000u &&
              runtime.words.at(0x1004u) == 0x3000u &&
              runtime.words.at(0x2000u) == 0x3000u &&
              runtime.words.at(0x3000u) == 0x1000u &&
              runtime.words.at(0x3004u) == 0x2000u &&
              runtime.words.at(0x3008u) == 0x6000u &&
              base_runtime.copied_name == std::string("second\0", 7),
          "later task links previous task and appends after old tail");
    Check(container.BeginWaiting(state) == 2u &&
              runtime.words.at(0x5028u) == 2u &&
              runtime.words.at(0x6028u) == 2u && state.ready == 0u,
          "begin-wait marks all tasks and clears readiness only when populated");
    Check(container.Destroy(state) == 0x77u &&
              runtime.deleted_tasks ==
                  std::vector<std::uint32_t>{0x5000u, 0x6000u} &&
              runtime.frees ==
                  std::vector<std::uint32_t>{0x2000u, 0x3000u, 0x1000u},
          "appended task nodes are freed in list order");
}

void AppendFailureKeepsLinks() {
    Runtime runtime;
    BaseRuntime base_runtime;
    pal4::GomTaskBase base(base_runtime);
    pal4::GomTaskContainer container(runtime, base);
    pal4::GomTaskContainerState state{};
    (void)container.Initialize(state, 0u);
    pal4::GomTaskBaseState first{}, second{};
    runtime.tasks[0x5000u] = &first;
    runtime.tasks[0x6000u] = &second;
    const pal4::GomTaskTimingConfig timing{2u, 4.0f, 10.0f};
    runtime.next_allocation = 0x2000u;
    (void)container.AddTaskAndInitialize(
        state, 0x8000u, first, 0x5000u, "first", timing);
    const auto links = runtime.words;
    runtime.next_allocation = 0u;
    bool no_node = false;
    try {
        (void)container.AddTaskAndInitialize(
            state, 0x8000u, second, 0x6000u, "second", timing);
    } catch (const std::bad_alloc&) { no_node = true; }
    Check(no_node && state.count == 1u && runtime.words == links &&
              first.next == 0u && second.name == 0u,
          "failed node allocation preserves live list and previous task links");
    runtime.next_allocation = 0x3000u;
    base_runtime.next_allocation = 0u;
    bool no_name = false;
    try {
        (void)container.AddTaskAndInitialize(
            state, 0x8000u, second, 0x6000u, "second", timing);
    } catch (const std::bad_alloc&) { no_name = true; }
    Check(no_name && state.count == 1u && runtime.words == links &&
              first.next == 0u && runtime.frees == std::vector<std::uint32_t>{0x3000u},
          "failed task name allocation releases reserved node without publishing links");
}

void UpdateLifecycle() {
    Runtime runtime;
    BaseRuntime base_runtime;
    pal4::GomTaskBase base(base_runtime);
    pal4::GomTaskContainer container(runtime, base);
    pal4::GomTaskContainerState state{};
    Check(container.Update(state, 1.0f) == 0u && runtime.events.empty(),
          "zero count exits before list access");
    static_cast<void>(container.Initialize(state, 0u));
    pal4::GomTaskBaseState task{};
    runtime.tasks[0x5000u] = &task;
    runtime.next_allocation = 0x2000u;
    static_cast<void>(container.AddTaskAndInitialize(
        state, 0x8000u, task, 0x5000u, "run",
        pal4::GomTaskTimingConfig{2u, 2.0f, 2.0f}));
    static_cast<void>(container.BeginWaiting(state));
    runtime.events.clear();
    Check(container.Update(state, 0.5f) == 1u &&
              task.phase == 2u && task.total_elapsed == 0.5f &&
              task.wait_elapsed == 0.5f && task.run_elapsed == 0.0f &&
              state.ready == 0u &&
              std::find(runtime.events.begin(), runtime.events.end(),
                        "wait") != runtime.events.end(),
          "waiting task accumulates total and wait time before callback");
    runtime.wait_complete = true;
    runtime.events.clear();
    Check(container.Update(state, 0.25f) == 1u && task.phase == 3u &&
              task.total_elapsed == 0.75f &&
              task.wait_elapsed == 0.75f &&
              std::find(runtime.events.begin(), runtime.events.end(),
                        "begin") != runtime.events.end(),
          "completed wait calls begin callback and enters running phase");
    runtime.events.clear();
    Check(container.Update(state, 1.0f) == 1u && task.phase == 3u &&
              task.total_elapsed == 1.75f && task.run_elapsed == 1.0f &&
              std::find(runtime.events.begin(), runtime.events.end(),
                        "tick") <
                  std::find(runtime.events.begin(), runtime.events.end(),
                            "duration"),
          "running task ticks before checking completion");
    runtime.duration_complete = true;
    runtime.events.clear();
    Check(container.Update(state, 1.0f) == 1u && task.phase == 4u &&
              task.total_elapsed == 2.75f && task.run_elapsed == 2.0f &&
              task.finish_latch == 2.75f && state.ready == 0u &&
              std::find(runtime.events.begin(), runtime.events.end(),
                        "finish") != runtime.events.end(),
          "completed run latches total time, finishes, and enters done phase");
    runtime.events.clear();
    Check(container.Update(state, 1.0f) == 0x33u && state.ready == 1u &&
              runtime.events.back() == "free" &&
              runtime.deleted_tasks ==
                  std::vector<std::uint32_t>{0x5000u} &&
              task.total_elapsed == 2.75f && state.count == 0u &&
              state.sentinel == 0x1000u,
          "done task is counted on next frame and triggers cleanup");
}

void CleanupAndIgnoredTasks() {
    Runtime runtime;
    BaseRuntime base_runtime;
    pal4::GomTaskBase base(base_runtime);
    pal4::GomTaskContainer container(runtime, base);
    pal4::GomTaskContainerState state{};
    state.count = 0xffffffffu;
    Check(container.Update(state, 1.0f) == 0xffffffffu &&
              runtime.events.empty(),
          "signed nonpositive count skips update traversal");
    state.count = 0u;
    static_cast<void>(container.Initialize(state, 0u));
    pal4::GomTaskBaseState task{};
    runtime.tasks[0x5000u] = &task;
    runtime.next_allocation = 0x2000u;
    static_cast<void>(container.AddTaskAndInitialize(
        state, 0x8000u, task, 0x5000u, "idle",
        pal4::GomTaskTimingConfig{}));
    task.phase = 4u;
    task.active = 0u;
    runtime.events.clear();
    Check(container.Update(state, 2.0f) == 1u && state.count == 1u &&
              task.total_elapsed == 0.0f && runtime.deleted_tasks.empty(),
          "inactive done task is excluded from completed count");
    task.active = 1u;
    task.phase = 7u;
    Check(container.Update(state, 2.0f) == 1u && task.phase == 7u &&
              task.total_elapsed == 0.0f,
          "unknown task phase is untouched");
    task.phase = 4u;
    Check(container.Update(state, 2.0f) == 0x33u &&
              state.count == 0u && state.ready == 1u &&
              state.sentinel == 0x1000u &&
              runtime.deleted_tasks == std::vector<std::uint32_t>{0x5000u},
          "done active task causes object cleanup but retains sentinel");
    state.ready = 0u;
    Check(container.CleanupAndReady(state) == 0x1000u &&
              state.ready == 1u && state.count == 0u,
          "cleanup wrapper preserves empty sentinel and sets ready");
}

}  // namespace

int main() {
    try {
        EmptyContainer();
        PopulatedContainer();
        AllocationFailure();
        AppendTasks();
        AppendFailureKeepsLinks();
        UpdateLifecycle();
        CleanupAndIgnoredTasks();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
