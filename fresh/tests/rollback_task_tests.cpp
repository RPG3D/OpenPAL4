#include "pal4/rollback_task.h"

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IRollbackTaskRuntime {
    static constexpr std::uint32_t object = 0x1000u;
    static constexpr std::uint32_t owner = 0x5000u;
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, bool> completion;
    std::vector<std::string> calls;
    std::uint32_t cleaned = 0u;
    std::uint32_t notified = 0u;
    unsigned wait_calls = 0u;
    bool wait_result = true;

    void BaseConstruct(std::uint32_t address) override {
        Check(address == object, "rollback base constructor address");
        calls.emplace_back("base construct");
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words.at(address);
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
        calls.emplace_back("write word");
    }
    void CleanupCollection(std::uint32_t collection) override {
        calls.emplace_back("cleanup");
        cleaned = collection;
    }
    void NotifyPrevious(std::uint32_t task) override {
        calls.emplace_back("previous");
        notified = task;
    }
    bool SiblingComplete(std::uint32_t task) override {
        calls.emplace_back("sibling");
        return completion.at(task);
    }
    bool WaitComplete(std::uint32_t address) override {
        Check(address == object, "rollback wait object");
        ++wait_calls;
        calls.emplace_back("wait");
        return wait_result;
    }
};

void CheckLifecycle() {
    Runtime runtime;
    Check(pal4::RollbackTask::Construct(
              Runtime::object, runtime) == Runtime::object &&
              runtime.words.at(Runtime::object) == 0x8488f4u &&
              runtime.words.at(Runtime::object + 0x3cu) == 0u &&
              runtime.words.at(Runtime::object + 0x40u) == 0u &&
              runtime.calls ==
                  (std::vector<std::string>{"base construct", "write word",
                                            "write word", "write word"}),
          "rollback constructor layout and order");
    Check(pal4::RollbackTask::ReturnZeroWithArgument(0xffffffffu) ==
              0u && pal4::RollbackTask::ReturnZero() == 0u,
          "rollback empty virtual methods");
}

void CheckEnter() {
    Runtime runtime;
    runtime.words[Runtime::object + 0x40u] = 0x7000u;
    runtime.words[Runtime::object + 0x3cu] = Runtime::owner;
    runtime.words[Runtime::object + 0x30u] = 0x6000u;
    runtime.words[Runtime::owner + 0x2a4u] = 0xa000u;
    runtime.words[Runtime::owner + 0x2a8u] = 0xa008u;
    Check(pal4::RollbackTask::Enter(Runtime::object, runtime) ==
              1u && runtime.cleaned == 0x7000u &&
              runtime.notified == 0x6000u &&
              runtime.words.at(Runtime::owner + 0x2a8u) == 0xa000u &&
              runtime.calls ==
                  (std::vector<std::string>{"cleanup", "write word",
                                            "previous"}),
          "rollback clears task range before previous callback");
    runtime.calls.clear();
    runtime.words[Runtime::object + 0x40u] = 0u;
    runtime.words[Runtime::object + 0x30u] = 0u;
    Check(pal4::RollbackTask::Enter(Runtime::object, runtime) ==
              1u && runtime.calls ==
                  (std::vector<std::string>{"write word"}),
          "rollback optional cleanup and callback gates");
}

void CheckWait() {
    Runtime runtime;
    runtime.words[Runtime::object + 0x28u] = 1u;
    Check(!pal4::RollbackTask::WaitUntilSiblingsDone(
              Runtime::object, runtime) && runtime.wait_calls == 0u,
          "rollback state gate");
    runtime.words[Runtime::object + 0x28u] = 2u;
    runtime.words[Runtime::object + 0x38u] = 0u;
    Check(!pal4::RollbackTask::WaitUntilSiblingsDone(
              Runtime::object, runtime) && runtime.wait_calls == 0u,
          "rollback missing group gate");
    runtime.words[Runtime::object + 0x38u] = 0x7000u;
    runtime.words[0x7008u] = 4u;
    runtime.words[Runtime::object + 0x30u] = 0x6000u;
    runtime.words[0x6030u] = 0x6100u;
    runtime.words[0x6130u] = 0u;
    runtime.words[Runtime::object + 0x34u] = 0x6200u;
    runtime.words[0x6234u] = 0u;
    runtime.completion[0x6000u] = true;
    runtime.completion[0x6100u] = false;
    runtime.completion[0x6200u] = true;
    Check(!pal4::RollbackTask::WaitUntilSiblingsDone(
              Runtime::object, runtime) && runtime.wait_calls == 0u &&
              std::count(runtime.calls.begin(), runtime.calls.end(),
                         "sibling") == 3,
          "rollback traverses both chains and counts completed siblings");
    runtime.completion[0x6100u] = true;
    runtime.calls.clear();
    runtime.wait_result = false;
    Check(!pal4::RollbackTask::WaitUntilSiblingsDone(
              Runtime::object, runtime) && runtime.wait_calls == 1u &&
              runtime.calls.back() == "wait",
          "rollback defers to common wait predicate");
    runtime.wait_result = true;
    Check(pal4::RollbackTask::WaitUntilSiblingsDone(
              Runtime::object, runtime) && runtime.wait_calls == 2u,
          "rollback completes with one remaining task");
}

}  // namespace

int main() {
    try {
        CheckLifecycle();
        CheckEnter();
        CheckWait();
        std::cout << "rollback_task_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
