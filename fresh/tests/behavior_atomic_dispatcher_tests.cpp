#include "pal4/behavior_atomic_dispatcher.h"

#include <cstdlib>
#include <iostream>
#include <map>
#include <stdexcept>
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

struct Runtime final : pal4::IBehaviorAtomicDispatchRuntime {
    std::map<std::pair<std::uint32_t, std::string>, std::uint32_t> attributes;
    std::map<std::uint32_t, std::int32_t> integers;
    std::map<std::uint32_t, float> floats;
    std::vector<std::uint32_t> randoms;
    std::vector<std::uint32_t> notified_records;
    std::vector<std::int32_t> processed_classes;
    std::vector<std::string> calls;
    pal4::BehaviorAtomicWaitOptions last_options{};
    std::uint32_t task_result = 0x9000u;
    std::size_t random_index = 0u;

    void DefineInt(std::uint32_t record, const char* name,
                   std::uint32_t token, std::int32_t value) {
        attributes[{record, name}] = token;
        integers[token] = value;
    }
    void DefineFloat(std::uint32_t record, const char* name,
                     std::uint32_t token, float value) {
        attributes[{record, name}] = token;
        floats[token] = value;
    }
    void CleanupCollection(std::uint32_t context) override {
        calls.emplace_back("cleanup");
        Check(context == 0xaaaa0001u, "cleanup context");
    }
    std::uint32_t Rand() override {
        calls.emplace_back("rand");
        Check(random_index < randoms.size(), "expected random sample");
        return randoms[random_index++];
    }
    void DivideByZero() override {
        calls.emplace_back("divide_zero");
        throw std::domain_error("zero record count");
    }
    std::uint32_t FindAttribute(
        std::uint32_t record, const char* name) override {
        calls.emplace_back(std::string("find:") + name);
        const auto it = attributes.find({record, name});
        return it == attributes.end() ? 0u : it->second;
    }
    std::int32_t ToInt(std::uint32_t attribute,
                       std::int32_t fallback) override {
        calls.emplace_back("int");
        Check(fallback == 0 || fallback == 3, "integer fallback");
        return integers.at(attribute);
    }
    float ToFloat(std::uint32_t attribute, float fallback) override {
        calls.emplace_back("float");
        Check(fallback == 0.0f, "float fallback");
        return floats.at(attribute);
    }
    std::uint32_t ProcessEvent(
        std::uint32_t context, std::int32_t class_id,
        std::uint32_t task_container, const char* atomic_name,
        const pal4::BehaviorAtomicWaitOptions& options) override {
        calls.emplace_back("process");
        Check(context == 0xaaaa0001u && task_container == 0xbbbb0002u &&
                  std::string(atomic_name) == "beh-vtm-atomic",
              "process-event arguments");
        processed_classes.push_back(class_id);
        last_options = options;
        return task_result;
    }
    void NotifyTask(std::uint32_t task,
                    std::uint32_t notification_argument,
                    std::uint32_t record) override {
        calls.emplace_back("notify");
        Check(task == task_result && notification_argument == 0xcccc0003u,
              "task callback arguments");
        notified_records.push_back(record);
    }
    void MarkProcessed(std::uint32_t context) override {
        calls.emplace_back("mark");
        Check(context == 0xaaaa0001u, "processed context");
    }
};

std::uint8_t Dispatch(pal4::ScriptEventDataState& state, Runtime& runtime) {
    return pal4::BehaviorAtomicDispatcher::Dispatch(
        state, 0xaaaa0001u, 0xbbbb0002u, 0xcccc0003u, runtime);
}

void TestIterateAll() {
    pal4::ScriptEventDataState state;
    state.words[4] = 0u;
    state.words[6] = 0x1000u;
    state.words[7] = 0x10b4u;
    Runtime runtime;
    runtime.DefineInt(0x1000u, "behAIAtomic-classID", 1u, 7);
    runtime.DefineInt(0x1000u, "beh-vtm-waitMode", 2u, 5);
    runtime.DefineFloat(0x1000u, "beh-vtm-waitTime", 3u, 1.5f);
    runtime.DefineInt(0x1078u, "behAIAtomic-classID", 4u, -1);
    Check(Dispatch(state, runtime) == 1u &&
              runtime.processed_classes == std::vector<std::int32_t>{7} &&
              runtime.notified_records == std::vector<std::uint32_t>{0x1000u} &&
              runtime.last_options.mode == 5 &&
              runtime.last_options.wait_time == 1.5f &&
              runtime.last_options.expect_time == 0.0f &&
              runtime.calls.front() == "cleanup" &&
              runtime.calls.back() == "mark" && runtime.random_index == 0u,
          "mode zero dispatches valid records and always marks processed");
}

void TestWeightedSelectionAndEarlyReturns() {
    pal4::ScriptEventDataState state;
    state.words[4] = 1u;
    state.words[6] = 0x1000u;
    state.words[7] = 0x1078u;
    Runtime runtime;
    runtime.randoms = {16384u};
    runtime.task_result = 0u;
    runtime.DefineInt(0x1000u, "behAIAtomic-proportion", 1u, 30);
    runtime.DefineInt(0x103cu, "behAIAtomic-proportion", 2u, 70);
    runtime.DefineInt(0x103cu, "behAIAtomic-classID", 3u, 8);
    Check(Dispatch(state, runtime) == 1u &&
              runtime.processed_classes == std::vector<std::int32_t>{8} &&
              runtime.notified_records.empty() &&
              runtime.last_options.mode == 3 &&
              runtime.last_options.wait_time == 0.0f &&
              runtime.last_options.expect_time == 0.0f &&
              runtime.calls.back() == "mark" && runtime.random_index == 1u,
          "weighted mode selects cumulative proportion and marks after null task");

    runtime.calls.clear();
    runtime.random_index = 0u;
    runtime.attributes.erase({0x103cu, "behAIAtomic-proportion"});
    Check(Dispatch(state, runtime) == 0u &&
              runtime.calls.back() == "find:behAIAtomic-proportion",
          "missing proportion returns zero without processed flag");

    runtime.calls.clear();
    runtime.random_index = 0u;
    state.words[7] = 0x103cu;
    Check(Dispatch(state, runtime) == 0u &&
              runtime.calls.back() == "int",
          "insufficient total proportion returns zero without processed flag");
}

void TestSingleAndUniformSelection() {
    pal4::ScriptEventDataState state;
    state.words[4] = 2u;
    state.words[6] = 0x1000u;
    state.words[7] = 0x103cu;
    Runtime runtime;
    runtime.DefineInt(0x1000u, "behAIAtomic-classID", 1u, 4);
    Check(Dispatch(state, runtime) == 1u && runtime.random_index == 0u &&
              runtime.notified_records == std::vector<std::uint32_t>{0x1000u},
          "single atomic is selected without random draw");

    state.words[7] = 0x1078u;
    runtime.calls.clear();
    runtime.notified_records.clear();
    runtime.randoms = {1u};
    runtime.DefineInt(0x103cu, "behAIAtomic-classID", 2u, 9);
    Check(Dispatch(state, runtime) == 1u && runtime.random_index == 1u &&
              runtime.notified_records == std::vector<std::uint32_t>{0x103cu} &&
              runtime.processed_classes.back() == 9,
          "multiple atomics use rand modulo record count");
}

void TestEmptyAndUnknownModes() {
    pal4::ScriptEventDataState state;
    Runtime runtime;
    state.words[4] = 3u;
    Check(Dispatch(state, runtime) == 0u &&
              runtime.calls == std::vector<std::string>{"cleanup"},
          "unknown mode exits after collection cleanup");

    runtime.calls.clear();
    state.words[4] = 0u;
    Check(Dispatch(state, runtime) == 1u &&
              runtime.calls == std::vector<std::string>{"cleanup", "mark"},
          "empty iterate-all mode marks processed");

    runtime.calls.clear();
    state.words[4] = 1u;
    runtime.randoms = {10u};
    Check(Dispatch(state, runtime) == 0u &&
              runtime.calls == std::vector<std::string>{"cleanup", "rand"},
          "empty weighted mode consumes random sample but does not mark");

    runtime.calls.clear();
    state.words[4] = 2u;
    runtime.randoms = {10u, 10u};
    bool caught = false;
    try {
        static_cast<void>(Dispatch(state, runtime));
    } catch (const std::domain_error&) {
        caught = true;
    }
    Check(caught && runtime.calls == std::vector<std::string>{
                  "cleanup", "rand", "divide_zero"},
          "empty uniform mode reaches observed divide-by-zero boundary");
}

}  // namespace

int main() {
    TestIterateAll();
    TestWeightedSelectionAndEarlyReturns();
    TestSingleAndUniformSelection();
    TestEmptyAndUnknownModes();
    pal4::ScriptEventDataState state;
    pal4::BehaviorAtomicDispatchRuntimeStub stub;
    Check(pal4::BehaviorAtomicDispatcher::Dispatch(
              state, 0u, 0u, 0u, stub) == 1u,
          "default runtime links");
    std::cout << "4 behavior atomic dispatch cases passed\n";
}
