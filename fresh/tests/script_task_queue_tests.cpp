#include "pal4/script_task_queue.h"

#include <algorithm>
#include <cstdint>
#include <deque>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

constexpr std::uint32_t kQueue = 0x1000u;
constexpr std::uint32_t kHead = 0x2000u;

struct Runtime final : pal4::IScriptTaskQueueRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::map<std::uint32_t, std::uint32_t> identities;
    std::map<std::uint32_t, bool> processed;
    std::map<std::uint32_t, std::uint32_t> results;
    std::deque<std::uint32_t> allocations;
    std::vector<std::string> calls;

    Runtime() {
        words[kQueue + 4u] = kHead;
        words[kQueue + 8u] = 0u;
        words[kHead] = kHead;
        words[kHead + 4u] = kHead;
    }

    void Add(std::uint32_t node, std::uint32_t task, bool owned = true) {
        const auto tail = words[kHead + 4u];
        words[node] = kHead;
        words[node + 4u] = tail;
        words[tail] = node;
        words[kHead + 4u] = node;
        words[node + 12u] = task;
        bytes[node + 8u] = owned ? 1u : 0u;
        ++words[kQueue + 8u];
    }

    std::uint32_t ReadWord(std::uint32_t address) override {
        return words.at(address);
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        calls.push_back("byte:" + std::to_string(address));
        return bytes[address];
    }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        bytes[address] = value;
        calls.push_back("set:" + std::to_string(address));
    }
    std::uint32_t Allocate(std::uint32_t size) override {
        Check(size == 16u, "queue node allocation size");
        calls.push_back("allocate");
        if (allocations.empty()) return 0u;
        const auto value = allocations.front();
        allocations.pop_front();
        return value;
    }
    std::uint32_t Free(std::uint32_t address) override {
        calls.push_back("free:" + std::to_string(address));
        return address + 1u;
    }
    std::uint32_t EventIdentity(std::uint32_t event) override {
        calls.push_back("identity:" + std::to_string(event));
        return identities[event];
    }
    bool EventProcessed(std::uint32_t event) override {
        calls.push_back("processed:" + std::to_string(event));
        return processed[event];
    }
    void PrepareEvent(std::uint32_t event) override {
        calls.push_back("prepare:" + std::to_string(event));
    }
    void SetProcessedFlag(std::uint32_t event_data) override {
        calls.push_back("flag:" + std::to_string(event_data));
    }
    void GetSkillSystem() override { calls.push_back("skill"); }
    void RunNoOp() override { calls.push_back("noop"); }
    void StartIdentityAction(std::uint32_t identity) override {
        calls.push_back("action:" + std::to_string(identity));
    }
    void UpdateContainer(std::uint32_t container, float delta) override {
        Check(delta == 0.25f, "queue delta is forwarded");
        calls.push_back("update:" + std::to_string(container));
    }
    void StopTask(std::uint32_t task) override {
        calls.push_back("stop:" + std::to_string(task));
    }
    std::uint32_t ResultEvent(std::uint32_t task) override {
        calls.push_back("result:" + std::to_string(task));
        return results[task];
    }
    void DeleteTask(std::uint32_t task, std::uint32_t flags) override {
        Check(flags == 1u, "queue deletes with scalar flag");
        calls.push_back("delete:" + std::to_string(task));
    }
};

std::size_t Count(const Runtime& runtime, const std::string& call) {
    return static_cast<std::size_t>(std::count(runtime.calls.begin(),
                                               runtime.calls.end(), call));
}

std::size_t Index(const Runtime& runtime, const std::string& call) {
    const auto it = std::find(runtime.calls.begin(), runtime.calls.end(), call);
    Check(it != runtime.calls.end(), "expected runtime call");
    return static_cast<std::size_t>(it - runtime.calls.begin());
}

void RegisterAndDeduplicate() {
    Runtime runtime;
    constexpr std::uint32_t old_event = 0x3000u;
    constexpr std::uint32_t new_event = 0x4000u;
    runtime.Add(0x5000u, old_event);
    runtime.bytes[old_event + 40u] = 1u;
    runtime.identities[old_event] = 77u;
    runtime.identities[new_event] = 77u;
    runtime.processed[old_event] = true;
    runtime.allocations.push_back(0x6000u);
    Check(pal4::ScriptTaskQueue::RegisterEvent(kQueue, new_event, runtime) ==
              new_event &&
          runtime.bytes[old_event + 40u] == 0u &&
          runtime.words[kHead] == 0x5000u &&
          runtime.words[0x5000u] == 0x6000u &&
          runtime.words[0x6000u] == kHead &&
          runtime.words[kHead + 4u] == 0x6000u &&
          runtime.words[0x6000u + 12u] == new_event &&
          runtime.bytes[0x6000u + 8u] == 1u &&
          runtime.words[kQueue + 8u] == 2u,
          "registration deactivates matching entry and tail-inserts owned node");
    Check(Count(runtime, "identity:" + std::to_string(new_event)) == 4u &&
          Count(runtime, "identity:" + std::to_string(old_event)) == 1u &&
          Index(runtime, "skill") < Index(runtime, "processed:" +
                                              std::to_string(new_event)) &&
          Index(runtime, "action:77") < Index(runtime, "prepare:" +
                                                 std::to_string(new_event)) &&
          Index(runtime, "prepare:" + std::to_string(new_event)) <
              Index(runtime, "flag:" + std::to_string(new_event + 4u)),
          "matching identity executes virtual calls in IDA order");
    runtime.calls.clear();
    Check(pal4::ScriptTaskQueue::RegisterEvent(kQueue, 0u, runtime) == 0u &&
          runtime.calls.empty(), "null registration does nothing");
}

void RegistrationBranches() {
    Runtime runtime;
    runtime.Add(0x5000u, 0x3000u);
    runtime.bytes[0x3000u + 40u] = 1u;
    runtime.identities[0x3000u] = 10u;
    runtime.identities[0x4000u] = 11u;
    runtime.allocations.push_back(0x6000u);
    Check(pal4::ScriptTaskQueue::RegisterEvent(kQueue, 0x4000u, runtime) ==
              0x4000u && runtime.bytes[0x3000u + 40u] == 1u &&
          Count(runtime, "skill") == 0u,
          "different identity retains previous event");

    Runtime failed;
    Check(pal4::ScriptTaskQueue::RegisterEvent(kQueue, 0x4000u, failed) ==
              0u && failed.words[kQueue + 8u] == 0u &&
          Count(failed, "delete:16384") == 1u &&
          Index(failed, "prepare:16384") < Index(failed, "allocate"),
          "allocation failure releases prepared event without linking");
}

void UpdateAndRequeue() {
    Runtime runtime;
    runtime.Add(0x5000u, 0x3000u);
    runtime.Add(0x5200u, 0x3200u, false);
    runtime.Add(0x5100u, 0x3100u);
    runtime.bytes[0x3000u + 40u] = 7u;
    runtime.bytes[0x3100u + 16u] = 1u;
    runtime.bytes[0x3100u + 40u] = 1u;
    runtime.results[0x3100u] = 0x4000u;
    runtime.allocations.push_back(0x6000u);
    Check(pal4::ScriptTaskQueue::Update(kQueue, 0.25f, runtime) ==
              0x4000u && runtime.words[kHead] == 0x5000u &&
          runtime.words[0x5000u] == 0x6000u &&
          runtime.words[kHead + 4u] == 0x6000u &&
          runtime.words[kQueue + 8u] == 2u,
          "second pass removes ready and disabled tasks then requeues result");
    Check(Count(runtime, "update:12292") == 1u &&
          Count(runtime, "stop:12544") == 1u &&
          Count(runtime, "result:12544") == 1u &&
          Count(runtime, "delete:12544") == 1u &&
          Count(runtime, "delete:12800") == 0u &&
          Count(runtime, "byte:12840") == 3u &&
          Index(runtime, "stop:12544") < Index(runtime, "result:12544") &&
          Index(runtime, "result:12544") < Index(runtime, "free:20736") &&
          Index(runtime, "free:20736") < Index(runtime, "prepare:16384"),
          "two-pass scan preserves virtual and ownership order");
}

void ActiveReturnAndReset() {
    Runtime runtime;
    runtime.Add(0x5000u, 0x3000u);
    runtime.bytes[0x3000u + 40u] = 0xa5u;
    Check(pal4::ScriptTaskQueue::Update(kQueue, 0.25f, runtime) ==
              ((kHead & ~0xffu) | 0xa5u) &&
          runtime.words[kQueue + 8u] == 1u &&
          Count(runtime, "update:12292") == 1u,
          "active task sets return low byte and remains queued");
    Check(pal4::ScriptTaskQueue::ResetFlags(kQueue, runtime) == kHead &&
          runtime.bytes[0x3000u + 40u] == 0u,
          "reset clears active flag");
    runtime.bytes[0x3000u + 16u] = 1u;
    runtime.bytes[0x3000u + 40u] = 1u;
    Check(pal4::ScriptTaskQueue::ResetFlags(kQueue, runtime) == kHead &&
          runtime.bytes[0x3000u + 40u] == 1u,
          "reset leaves ready task untouched");
}

void CleanupOwnership() {
    Runtime runtime;
    runtime.Add(0x5000u, 0x3000u);
    runtime.Add(0x5100u, 0x3100u, false);
    Check(pal4::ScriptTaskQueue::Cleanup(kQueue, runtime) == 0x5101u &&
          runtime.words[kHead] == kHead &&
          runtime.words[kHead + 4u] == kHead &&
          runtime.words[kQueue + 8u] == 0u &&
          Count(runtime, "delete:12288") == 1u &&
          Count(runtime, "delete:12544") == 0u &&
          Index(runtime, "delete:12288") < Index(runtime, "free:20480") &&
          Index(runtime, "free:20480") < Index(runtime, "free:20736"),
          "cleanup unlinks all nodes and deletes owned objects first");
    Check(pal4::ScriptTaskQueue::Cleanup(kQueue, runtime) == 0u,
          "empty cleanup defines otherwise indeterminate native result");
}

}  // namespace

int main() {
    RegisterAndDeduplicate();
    RegistrationBranches();
    UpdateAndRequeue();
    ActiveReturnAndReset();
    CleanupOwnership();
    std::cout << "Script task queue tests passed\n";
}
