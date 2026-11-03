#include "pal4/script_event_processor.h"

#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IScriptEventSchedulerRuntime {
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint32_t> condition_results;
    std::vector<std::string> calls;
    std::int32_t random_value = 0;
    std::uint32_t dispatch_result = 0u;
    std::uint32_t update_result = 0x99u;
    float last_elapsed = 0.0f;

    std::uint8_t ReadByte(std::uint32_t address) override {
        calls.push_back("byte:" + std::to_string(address));
        return bytes.at(address);
    }
    std::uint32_t ReadDword(std::uint32_t address) override {
        calls.push_back("word:" + std::to_string(address));
        return words.at(address);
    }
    std::int32_t Random() override {
        calls.push_back("random");
        return random_value;
    }
    std::uint32_t GetConditionDocument(std::uint32_t event) override {
        calls.push_back("document:" + std::to_string(event));
        return event + 0x100u;
    }
    std::uint32_t CallCondition(std::uint32_t callback,
                                std::uint32_t document) override {
        calls.push_back("condition:" + std::to_string(callback) + ":" +
                        std::to_string(document));
        return condition_results.at(document);
    }
    void InsertCandidate(pal4::ScriptEventProcessorState& state,
                         std::uint32_t position,
                         std::uint32_t event) override {
        calls.push_back("insert:" + std::to_string(position) + ":" +
                        std::to_string(event));
        if (state.words[9] == 0u) {
            state.words[9] = 0x2000u;
            state.words[10] = 0x2000u;
            state.words[11] = 0x2100u;
        }
        words[state.words[10]] = event;
        state.words[10] += 4u;
    }
    std::uint32_t DispatchEvent(std::uint32_t event,
                                std::uint32_t task_context,
                                std::uint32_t argument_two,
                                std::uint32_t argument_three) override {
        calls.push_back("dispatch:" + std::to_string(event) + ":" +
                        std::to_string(task_context) + ":" +
                        std::to_string(argument_two) + ":" +
                        std::to_string(argument_three));
        return dispatch_result;
    }
    std::uint32_t UpdateContainer(std::uint32_t task_context,
                                  float elapsed) override {
        calls.push_back("update:" + std::to_string(task_context));
        last_elapsed = elapsed;
        return update_result;
    }
};

void TestGates() {
    pal4::ScriptEventProcessorState state;
    Runtime runtime;
    Check(pal4::ScriptEventProcessor::HandleEventProcessing(
              state, 0.5f, 0x9000u, 1u, 2u, 0x12345678u,
              runtime) == 0x12345600u && runtime.calls.empty(),
          "disabled processor clears only incoming EAX low byte");
    state.words[6] = 0x12340001u;
    runtime.bytes[0x900cu] = 0u;
    Check(pal4::ScriptEventProcessor::HandleEventProcessing(
              state, 0.5f, 0x9000u, 1u, 2u, 0u,
              runtime) == 0x99u && runtime.last_elapsed == 0.5f &&
              runtime.calls == std::vector<std::string>{
                  "byte:36876", "update:36864"},
          "inactive task context advances the task container directly");
    runtime.calls.clear();
    runtime.bytes[0x900cu] = 1u;
    Check(pal4::ScriptEventProcessor::HandleEventProcessing(
              state, 0.5f, 0x9000u, 1u, 2u, 0u,
              runtime) == 0u &&
              runtime.calls == std::vector<std::string>{"byte:36876"},
          "missing callback stops before scanning event records");
    runtime.calls.clear();
    state.words[7] = 0x8000u;
    Check(pal4::ScriptEventProcessor::HandleEventProcessing(
              state, 0.5f, 0x9000u, 1u, 2u, 0u,
              runtime) == 0u && state.words[10] == 0u &&
              runtime.calls == std::vector<std::string>{"byte:36876"},
          "empty source and candidate arrays yield no selected event");
}

void TestDispatch() {
    pal4::ScriptEventProcessorState state;
    state.words[1] = 0x1000u;
    state.words[2] = 0x1008u;
    state.words[6] = 1u;
    state.words[7] = 0x8000u;
    state.words[9] = 0x2000u;
    state.words[10] = 0x2000u;
    Runtime runtime;
    runtime.bytes[0x900cu] = 1u;
    runtime.words = {{0x1000u, 0x5000u}, {0x1004u, 0x6000u}};
    runtime.condition_results = {{0x5100u, 0x101u},
                                 {0x6100u, 0x100u}};
    runtime.dispatch_result = 0x12340000u;
    Check(pal4::ScriptEventProcessor::HandleEventProcessing(
              state, 0.75f, 0x9000u, 3u, 4u, 0u,
              runtime) == 0x12340000u &&
              state.words[10] == state.words[9] &&
              runtime.calls == std::vector<std::string>{
                  "byte:36876", "word:4096", "document:20480",
                  "condition:32768:20736", "insert:8192:20480",
                  "word:4100", "document:24576",
                  "condition:32768:24832", "word:8192",
                  "dispatch:20480:36864:3:4"},
          "condition filter inserts one candidate and false dispatch skips update");

    runtime.calls.clear();
    state.words[10] = state.words[9];
    runtime.condition_results[0x6100u] = 1u;
    runtime.random_value = 1;
    runtime.dispatch_result = 0xdead0001u;
    Check(pal4::ScriptEventProcessor::HandleEventProcessing(
              state, 0.75f, 0x9000u, 3u, 4u, 0u,
              runtime) == 0x99u &&
              state.words[10] == state.words[9] &&
              runtime.last_elapsed == 0.75f &&
              runtime.calls == std::vector<std::string>{
                  "byte:36876", "word:4096", "document:20480",
                  "condition:32768:20736", "insert:8192:20480",
                  "word:4100", "document:24576",
                  "condition:32768:24832", "insert:8196:24576",
                  "random", "word:8196",
                  "dispatch:24576:36864:3:4", "update:36864"},
          "two candidates use CRT random selection and successful dispatch updates");
}

void TestSelectionAndFirst() {
    pal4::ScriptEventProcessorState state;
    state.words[1] = 0x1000u;
    Runtime runtime;
    Check(pal4::ScriptEventProcessor::FirstEvent(state) == 0x1000u &&
              pal4::ScriptEventProcessor::SelectRandomEvent(
                  state, runtime) == 0u && runtime.calls.empty(),
          "first-event getter returns begin and null candidate array selects none");
    state.words[9] = 0x2000u;
    state.words[10] = 0x1ffcu;
    Check(pal4::ScriptEventProcessor::SelectRandomEvent(
              state, runtime) == 0u && runtime.calls.empty(),
          "negative signed candidate span selects none");
    state.words[10] = 0x2004u;
    runtime.words[0x2000u] = 0x5000u;
    Check(pal4::ScriptEventProcessor::SelectRandomEvent(
              state, runtime) == 0x5000u &&
              runtime.calls == std::vector<std::string>{"word:8192"},
          "single candidate avoids the random call");
    runtime.calls.clear();
    state.words[10] = 0x2008u;
    runtime.words[0x2004u] = 0x6000u;
    runtime.random_value = 3;
    Check(pal4::ScriptEventProcessor::SelectRandomEvent(
              state, runtime) == 0x6000u &&
              runtime.calls == std::vector<std::string>{
                  "random", "word:8196"},
          "multiple candidates use signed remainder as an index");
}

}  // namespace

int main() {
    TestGates();
    TestDispatch();
    TestSelectionAndFirst();
    pal4::ScriptEventProcessorState state;
    pal4::ScriptEventSchedulerRuntimeStub stub;
    Check(pal4::ScriptEventProcessor::HandleEventProcessing(
              state, 0.0f, 0u, 0u, 0u, 0u, stub) == 0u,
          "scheduler runtime stub links");
    std::cout << "11 script event scheduler cases passed\n";
}
