#include "pal4/time_wait_task.h"

#include <algorithm>
#include <bit>
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

std::size_t IndexOf(const std::vector<std::string>& calls,
                    const std::string& name) {
    const auto found = std::find(calls.begin(), calls.end(), name);
    if (found == calls.end()) throw std::runtime_error("missing " + name);
    return static_cast<std::size_t>(found - calls.begin());
}

struct Runtime : virtual pal4::ITimeWaitTaskRuntime {
    static constexpr std::uint32_t object = 0x1000u;
    static constexpr std::uint32_t owner = 0x5000u;
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> calls;
    bool fail_base_construct = false;

    void BaseConstruct(std::uint32_t address) override {
        Check(address == object || address == 0xe000u,
              "wait base constructor address");
        calls.emplace_back("base construct");
        if (fail_base_construct)
            throw std::runtime_error("base constructor failed");
    }
    std::uint32_t BaseDestruct(std::uint32_t address) override {
        Check(address == object, "wait base destructor address");
        calls.emplace_back("base destruct");
        return 0x76543210u;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words.at(address);
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        return bytes.at(address);
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
        calls.emplace_back("write word");
    }
    void WriteByte(std::uint32_t address,
                   std::uint8_t value) override {
        bytes[address] = value;
        calls.emplace_back("write byte");
    }
};

struct SpawnRuntime final : Runtime, pal4::ITimeWaitTaskSpawnRuntime {
    static constexpr std::uint32_t child = 0xe000u;
    static constexpr std::uint32_t helper = 0xb000u;
    static constexpr std::uint32_t context = 0x7000u;
    static constexpr std::uint32_t collection = owner + 0x2a0u;
    std::deque<std::uint32_t> allocations{child, helper};
    std::vector<std::uint32_t> allocation_sizes;
    std::vector<std::uint32_t> freed;
    std::uint32_t registered = 0u;
    std::uint32_t inserted = 0u;
    std::uint32_t added_helper = 0u;
    std::uint32_t processed = 0u;

    SpawnRuntime() {
        words[object + 0x4cu] = context;
        words[object + 0x48u] = owner;
        words[object + 0x40u] = std::bit_cast<std::uint32_t>(10.0f);
        words[object + 0x30u] = 0u;
        words[collection + 4u] = 0xa000u;
        words[collection + 8u] = 0xa004u;
        words[collection + 12u] = 0xa008u;
        words[0xa000u] = 0xaaaa0000u;
    }
    std::uint32_t AllocateMemory(std::uint32_t size) override {
        allocation_sizes.push_back(size);
        calls.emplace_back("allocate");
        const auto memory = allocations.front();
        allocations.pop_front();
        return memory;
    }
    void FreeMemory(std::uint32_t memory) override {
        freed.push_back(memory);
        calls.emplace_back("free");
    }
    void AddReverseTask(std::uint32_t task_context,
                        std::uint32_t task,
                        std::uint32_t timing) override {
        Check(task_context == context && task == child &&
                  timing == object + 0x0cu,
              "wait reverse timing");
        registered = task;
        calls.emplace_back("add reverse");
    }
    void InsertOwnerTask(std::uint32_t address,
                         std::uint32_t task) override {
        Check(address == collection, "wait owner collection");
        inserted = task;
        calls.emplace_back("insert reverse");
    }
    void NotifyPreviousTask(std::uint32_t previous) override {
        Check(previous == 0x6000u, "wait previous task");
        calls.emplace_back("previous");
    }
    std::uint32_t ConstructHelperTask(std::uint32_t memory) override {
        Check(memory == helper, "wait helper memory");
        calls.emplace_back("construct helper");
        return memory;
    }
    void AddHelperTask(std::uint32_t task_context,
                       std::uint32_t task,
                       std::uint32_t first,
                       std::uint32_t second,
                       std::uint32_t third) override {
        Check(task_context == context && task == helper &&
                  first == 1u && second == 0u && third == 0u,
              "wait helper timing");
        added_helper = task;
        calls.emplace_back("add helper");
    }
    void SetHelperOwner(std::uint32_t task,
                        std::uint32_t task_owner) override {
        Check((task == helper || task == 0u) && task_owner == owner,
              "wait helper owner");
        calls.emplace_back("set helper owner");
    }
    void MarkProcessed(std::uint32_t task_context) override {
        Check(task_context == context, "wait processed context");
        processed = task_context;
        calls.emplace_back("processed");
    }
};

void CheckLifecycleAndElapsed() {
    Runtime runtime;
    Check(pal4::TimeWaitTask::Construct(Runtime::object, runtime) ==
              Runtime::object &&
              runtime.words.at(Runtime::object) == 0x84898cu &&
              runtime.words.at(Runtime::object + 0x48u) == 0u &&
              runtime.words.at(Runtime::object + 0x4cu) == 0u &&
              runtime.calls.front() == "base construct",
          "wait constructor layout");
    Check(!pal4::TimeWaitTask::Enter(Runtime::object, runtime),
          "wait entry rejects missing owner");
    Check(pal4::TimeWaitTask::Initialize(
              Runtime::object, Runtime::owner,
              std::bit_cast<std::uint32_t>(10.0f), 0x7000u,
              runtime) == 0x7000u &&
              runtime.bytes.at(Runtime::object + 0x3cu) == 0u &&
              runtime.words.at(Runtime::object + 0x48u) ==
                  Runtime::owner &&
              runtime.words.at(Runtime::object + 0x44u) == 0u &&
              runtime.words.at(Runtime::object + 0x40u) ==
                  std::bit_cast<std::uint32_t>(10.0f) &&
              pal4::TimeWaitTask::Enter(Runtime::object, runtime),
          "wait initializer and entry");
    Check(pal4::TimeWaitTask::Update(Runtime::object, 5.0f,
                                      runtime) == 1u &&
              std::bit_cast<float>(runtime.words.at(
                  Runtime::object + 0x44u)) == 5.0f &&
              runtime.bytes.at(Runtime::object + 0x3cu) == 0u,
          "wait update before duration");
    Check(pal4::TimeWaitTask::Update(Runtime::object, 5.0f,
                                       runtime) == 1u &&
              std::bit_cast<float>(runtime.words.at(
                  Runtime::object + 0x44u)) == 10.0f &&
              runtime.bytes.at(Runtime::object + 0x3cu) == 0u,
          "wait equality preserves incomplete latch per IDA599936..940");
    Check(pal4::TimeWaitTask::Update(Runtime::object, 1.0f,
                                      runtime) == 1u &&
              std::bit_cast<float>(runtime.words.at(
                  Runtime::object + 0x44u)) == 11.0f &&
              runtime.bytes.at(Runtime::object + 0x3cu) == 1u,
          "wait update completes without clamping elapsed");
    runtime.calls.clear();
    Check(pal4::TimeWaitTask::Update(Runtime::object, 1.0f,
                                      runtime) == 0u &&
              runtime.calls.empty(), "wait completed update gate");
    (void)pal4::TimeWaitTask::Initialize(Runtime::object, Runtime::owner,
        std::bit_cast<std::uint32_t>(1.f), 0u, runtime);
    runtime.words[Runtime::object + 0x44u] = std::bit_cast<std::uint32_t>(1.f);
    Check(pal4::TimeWaitTask::Update(Runtime::object, 0x1p-25f, runtime) == 1u &&
              runtime.words.at(Runtime::object + 0x44u) == std::bit_cast<std::uint32_t>(1.f) &&
              runtime.bytes.at(Runtime::object + 0x3cu) == 0u,
          "wait preserves game PC24 float addition at the duration boundary");
    runtime.words[Runtime::object + 0x48u] = 0u;
    Check(pal4::TimeWaitTask::Update(Runtime::object, 1.0f,
                                      runtime) == 0u &&
              runtime.bytes.at(Runtime::object + 0x3cu) == 0u,
          "wait missing owner clears completion");
    Check(pal4::TimeWaitTask::Destruct(Runtime::object, runtime) ==
              0x76543210u &&
              runtime.words.at(Runtime::object) == 0x8486fcu &&
              runtime.words.at(Runtime::object + 0x4cu) == 0u &&
              runtime.calls.back() == "base destruct",
          "wait destructor layout and return");
}

void CheckFactoryAndFollowup() {
    Runtime descriptor;
    Check(pal4::TimeWaitTaskFactory::ConstructDescriptor(
              0x2000u, descriptor) == 0x2000u &&
              descriptor.words.at(0x2000u) == 0x8489b0u,
          "wait factory descriptor");
    SpawnRuntime factory;
    Check(pal4::TimeWaitTaskFactory::Create(factory) ==
              SpawnRuntime::child &&
              factory.allocation_sizes ==
                  (std::vector<std::uint32_t>{0x50u}) &&
              factory.words.at(SpawnRuntime::child) == 0x84898cu,
          "wait factory creates task");
    SpawnRuntime null_factory;
    null_factory.allocations = {0u};
    Check(pal4::TimeWaitTaskFactory::Create(null_factory) == 0u &&
              null_factory.calls ==
                  (std::vector<std::string>{"allocate"}),
          "wait factory null allocation");
    SpawnRuntime failed_factory;
    failed_factory.fail_base_construct = true;
    bool thrown = false;
    try {
        (void)pal4::TimeWaitTaskFactory::Create(failed_factory);
    } catch (const std::runtime_error&) {
        thrown = true;
    }
    Check(thrown && failed_factory.freed ==
                        (std::vector<std::uint32_t>{SpawnRuntime::child}),
          "wait factory failed construction cleanup");
    SpawnRuntime no_context;
    no_context.words[Runtime::object + 0x4cu] = 0u;
    Check(pal4::TimeWaitTask::SpawnFollowup(
              Runtime::object, no_context) == 0u &&
              no_context.calls.empty(), "wait context gate");
    SpawnRuntime no_owner;
    no_owner.words[Runtime::object + 0x48u] = 0u;
    Check(pal4::TimeWaitTask::SpawnFollowup(
              Runtime::object, no_owner) == 0u &&
              no_owner.calls.empty(), "wait owner gate");
    SpawnRuntime previous;
    previous.words[Runtime::object + 0x30u] = 0x6000u;
    Check(pal4::TimeWaitTask::SpawnFollowup(
              Runtime::object, previous) == 1u &&
              previous.registered == SpawnRuntime::child &&
              previous.inserted == SpawnRuntime::child &&
              previous.words.at(SpawnRuntime::child + 0x40u) ==
                  previous.words.at(Runtime::object + 0x40u) &&
              previous.words.at(SpawnRuntime::child + 0x4cu) == 0u &&
              IndexOf(previous.calls, "add reverse") <
                  IndexOf(previous.calls, "insert reverse") &&
              IndexOf(previous.calls, "insert reverse") <
                  IndexOf(previous.calls, "previous"),
          "wait reverse child before previous callback");
    SpawnRuntime helper;
    Check(pal4::TimeWaitTask::SpawnFollowup(
              Runtime::object, helper) == 1u &&
              helper.added_helper == SpawnRuntime::helper &&
              helper.words.at(0xa004u) == SpawnRuntime::helper &&
              helper.processed == SpawnRuntime::context &&
              helper.allocation_sizes ==
                  (std::vector<std::uint32_t>{0x50u, 0x40u}),
          "wait appends helper");
    SpawnRuntime growth;
    growth.words[SpawnRuntime::collection + 12u] = 0xa004u;
    growth.allocations.push_back(0xf000u);
    Check(pal4::TimeWaitTask::SpawnFollowup(
              Runtime::object, growth) == 1u &&
              growth.allocation_sizes ==
                  (std::vector<std::uint32_t>{0x50u, 0x40u, 8u}) &&
              growth.words.at(0xf000u) == 0xaaaa0000u &&
              growth.words.at(0xf004u) == SpawnRuntime::helper,
          "wait grows helper collection");
}

}  // namespace

int main() {
    try {
        CheckLifecycleAndElapsed();
        CheckFactoryAndFollowup();
        std::cout << "time_wait_task_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
