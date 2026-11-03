#include "pal4/stored_transform_task.h"

#include <array>
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

struct Runtime final : pal4::IStoredTransformTaskRuntime {
    static constexpr std::uint32_t object = 0x1000u;
    static constexpr std::uint32_t owner = 0x5000u;
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> calls;
    std::uint32_t applied_owner = 0u;
    std::array<std::uint32_t, 3> applied_vector{1u, 1u, 1u};
    bool wait_result = false;

    void BaseConstruct(std::uint32_t address) override {
        Check(address == object, "stored task base constructor address");
        calls.emplace_back("base construct");
    }
    std::uint32_t BaseDestruct(std::uint32_t address) override {
        Check(address == object, "stored task base destructor address");
        calls.emplace_back("base destruct");
        return 0x76543210u;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.emplace_back("read owner");
        return words.at(address);
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        calls.emplace_back("write word");
        words[address] = value;
    }
    void ApplyStoredTransform(
        std::uint32_t task_owner,
        const std::array<std::uint32_t, 3>& zero_vector) override {
        calls.emplace_back("apply transform");
        applied_owner = task_owner;
        applied_vector = zero_vector;
    }
    bool WaitComplete(std::uint32_t address) override {
        Check(address == object, "stored task wait object");
        calls.emplace_back("wait complete");
        return wait_result;
    }
};

void CheckLifecycleAndApply() {
    Runtime runtime;
    Check(pal4::StoredTransformTask::Construct(
              Runtime::object, runtime) == Runtime::object &&
              runtime.words.at(Runtime::object) == 0x8488d0u &&
              runtime.words.at(Runtime::object + 0x3cu) == 0u &&
              runtime.calls ==
                  (std::vector<std::string>{"base construct", "write word",
                                            "write word"}),
          "stored task constructor layout and order");
    runtime.calls.clear();
    Check(pal4::StoredTransformTask::SetOwner(
              Runtime::object, Runtime::owner, runtime) ==
              Runtime::owner &&
              runtime.words.at(Runtime::object + 0x3cu) ==
                  Runtime::owner,
          "stored task owner setter");
    runtime.calls.clear();
    Check(pal4::StoredTransformTask::Apply(
              Runtime::object, runtime) == 1u &&
              runtime.applied_owner == Runtime::owner &&
              runtime.applied_vector ==
                  (std::array<std::uint32_t, 3>{0u, 0u, 0u}) &&
              runtime.calls ==
                  (std::vector<std::string>{"read owner", "apply transform"}),
          "stored task applies owner transform with zero vector");
    runtime.calls.clear();
    Check(pal4::StoredTransformTask::Destruct(
              Runtime::object, runtime) == 0x76543210u &&
              runtime.words.at(Runtime::object + 0x3cu) == 0u &&
              runtime.words.at(Runtime::object) == 0x8486fcu &&
              runtime.calls ==
                  (std::vector<std::string>{"write word", "write word",
                                            "base destruct"}),
          "stored task destructor order and return");
}

void CheckWaitThunk() {
    Runtime runtime;
    for (const bool ready : {false, true}) {
        runtime.wait_result = ready;
        runtime.calls.clear();
        Check(pal4::StoredTransformTask::IsWaitComplete(
                  Runtime::object, runtime) == ready &&
                  runtime.calls ==
                      (std::vector<std::string>{"wait complete"}),
              "stored task wait thunk forwards predicate");
    }
}

}  // namespace

int main() {
    try {
        CheckLifecycleAndApply();
        CheckWaitThunk();
        std::cout << "stored_transform_task_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
