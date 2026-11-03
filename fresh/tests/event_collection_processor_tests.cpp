#include "pal4/event_collection_processor.h"

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IEventCollectionBindRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint32_t> visited;
    std::vector<std::string> calls;
    std::uint32_t next_object{0x5000u};
    std::uint32_t next_array{0x8000u};
    std::uint32_t allocations{};
    std::uint32_t fail_allocation{};
    std::uint32_t throw_on_visit{};
    std::uint32_t throw_on_append{};
    std::uint32_t temp_off_calls{};
    std::uint32_t throw_on_bind{};

    std::uint32_t Allocate(const std::uint32_t bytes) override {
        calls.push_back("allocate:" + std::to_string(bytes));
        ++allocations;
        if (allocations == fail_allocation) return 0u;
        const auto address = next_object;
        next_object += 0x100u;
        return address;
    }
    std::uint32_t ReadWord(const std::uint32_t address) override {
        return words.at(address);
    }
    void WriteWord(const std::uint32_t address,
                   const std::uint32_t value) override {
        words[address] = value;
    }
    void Free(const std::uint32_t address) override {
        calls.push_back("free:" + std::to_string(address));
    }
    bool AtEnd(const std::uint32_t iterator,
               const std::uint32_t vtable_offset) override {
        Check(vtable_offset == 24u, "end predicate uses virtual slot 24");
        const auto owner = ReadWord(iterator + 8u);
        return ReadWord(iterator + 4u) == ReadWord(owner + 8u);
    }
    void Advance(const std::uint32_t iterator,
                 const std::uint32_t vtable_offset) override {
        Check(vtable_offset == 36u, "advance uses virtual slot 36");
        calls.push_back("advance:" + std::to_string(iterator));
        WriteWord(iterator + 4u, ReadWord(iterator + 4u) + 4u);
    }
    void Visit(const std::uint32_t element,
               pal4::EventVisitorState& visitor,
               const std::uint32_t vtable_offset) override {
        Check(vtable_offset == 12u && visitor.vtable == 0x8498a8u &&
                  visitor.output == 0u,
              "visitor has original vtable and cleared output");
        calls.push_back("visit:" + std::to_string(element));
        if (element == throw_on_visit) throw std::runtime_error("visitor");
        visitor.output = visited.at(element);
    }
    void Append(pal4::EventOutputState& output,
                const std::uint32_t value) override {
        calls.push_back("append:" + std::to_string(value));
        if (value != 0u && value == throw_on_append)
            throw std::runtime_error("append");
        if (output.begin == 0u) {
            output.begin = next_array;
            output.end = output.begin;
            output.capacity = output.begin + 64u;
            next_array += 0x100u;
        }
        Check(output.end < output.capacity, "test output has spare capacity");
        WriteWord(output.end, value);
        output.end += 4u;
    }
    void IncrementTempOff() override {
        calls.push_back("temp_off");
        ++temp_off_calls;
    }
    void BindEffect(const std::uint32_t target,
                    const std::uint32_t effect) override {
        calls.push_back("bind:" + std::to_string(target) + ":" +
                        std::to_string(effect));
        if (effect == throw_on_bind)
            throw std::runtime_error("effect binding");
    }
};

void SeedEmptyCollection(Runtime& runtime, const std::uint32_t group,
                         const std::uint32_t first,
                         const std::uint32_t third) {
    runtime.words[group + 4u] = first;
    runtime.words[group + 8u] = first;
    runtime.words[group + 36u] = third;
    runtime.words[group + 40u] = third;
}

void SeedSingleChild(Runtime& runtime) {
    SeedEmptyCollection(runtime, 0x1000u, 0x2000u, 0x2100u);
    SeedEmptyCollection(runtime, 0x1100u, 0x2200u, 0x2300u);
    runtime.words[0x1028u] = 0x2104u;
    runtime.words[0x2100u] = 0x3100u;
    runtime.words[0x3104u] = 0x1100u;
    runtime.words[0x1108u] = 0x2204u;
    runtime.words[0x2200u] = 0x3200u;
    runtime.visited[0x3200u] = 0xb0u;
}

void TestEmpty() {
    Runtime runtime;
    SeedEmptyCollection(runtime, 0x1000u, 0x2000u, 0x2100u);
    pal4::EventOutputState output{};
    Check(pal4::EventCollectionProcessor::Process(
              output, 0x1000u, runtime) == 0x5000u &&
              output.begin == 0u && output.end == 0u &&
              runtime.calls == std::vector<std::string>{
                  "allocate:12", "allocate:12", "free:20736",
                  "free:20480"},
          "empty collection creates and releases both iterators");
}

void TestRecursiveWrapping() {
    Runtime runtime;
    SeedEmptyCollection(runtime, 0x1000u, 0x2000u, 0x2100u);
    SeedEmptyCollection(runtime, 0x1100u, 0x2200u, 0x2300u);
    runtime.words[0x1008u] = 0x2008u;
    runtime.words[0x2000u] = 0x3000u;
    runtime.words[0x2004u] = 0x3004u;
    runtime.visited[0x3000u] = 0xa0u;
    runtime.visited[0x3004u] = 0xa1u;
    runtime.words[0x1028u] = 0x2104u;
    runtime.words[0x2100u] = 0x3100u;
    runtime.words[0x3104u] = 0x1100u;
    runtime.words[0x1108u] = 0x2204u;
    runtime.words[0x2200u] = 0x3200u;
    runtime.visited[0x3200u] = 0xb0u;

    pal4::EventOutputState output{};
    Check(pal4::EventCollectionProcessor::Process(
              output, 0x1000u, runtime) == 0x5000u &&
              output.begin == 0x8000u && output.end == 0x800cu &&
              runtime.words.at(0x8000u) == 0xa0u &&
              runtime.words.at(0x8004u) == 0xa1u &&
              runtime.words.at(0x8008u) == 0x5400u &&
              runtime.words.at(0x5400u) == 0x849888u &&
              runtime.words.at(0x5404u) == 0x3100u &&
              runtime.words.at(0x5408u) == 0xb0u &&
              runtime.temp_off_calls == 1u,
          "direct values precede wrapped recursive child value");
    const auto visit_first = std::find(runtime.calls.begin(),
                                       runtime.calls.end(), "visit:12288");
    const auto visit_child = std::find(runtime.calls.begin(),
                                       runtime.calls.end(), "visit:12800");
    const auto free_child_buffer = std::find(runtime.calls.begin(),
                                             runtime.calls.end(), "free:33024");
    Check(visit_first < visit_child &&
              free_child_buffer != runtime.calls.end() &&
              runtime.calls[runtime.calls.size() - 2u] == "free:20736" &&
              runtime.calls.back() == "free:20480",
          "recursive traversal releases child buffer before outer iterators");
}

void TestFailures() {
    Runtime runtime;
    SeedEmptyCollection(runtime, 0x1000u, 0x2000u, 0x2100u);
    pal4::EventOutputState output{};
    runtime.fail_allocation = 1u;
    bool caught = false;
    try {
        (void)pal4::EventCollectionProcessor::Process(
            output, 0x1000u, runtime);
    } catch (const std::bad_alloc&) {
        caught = true;
    }
    Check(caught && runtime.calls ==
              std::vector<std::string>{"allocate:12"},
          "failed first iterator allocation stops before traversal");
    runtime.fail_allocation = 2u;
    runtime.allocations = 0u;
    runtime.calls.clear();
    caught = false;
    try {
        (void)pal4::EventCollectionProcessor::Process(
            output, 0x1000u, runtime);
    } catch (const std::bad_alloc&) {
        caught = true;
    }
    Check(caught && runtime.calls == std::vector<std::string>{
              "allocate:12", "allocate:12", "free:20480"},
          "failed second iterator releases the first");

    Runtime exceptional;
    SeedEmptyCollection(exceptional, 0x1000u, 0x2000u, 0x2100u);
    SeedEmptyCollection(exceptional, 0x1100u, 0x2200u, 0x2300u);
    exceptional.words[0x1028u] = 0x2104u;
    exceptional.words[0x2100u] = 0x3100u;
    exceptional.words[0x3104u] = 0x1100u;
    exceptional.words[0x1108u] = 0x2204u;
    exceptional.words[0x2200u] = 0x3200u;
    exceptional.throw_on_visit = 0x3200u;
    caught = false;
    try {
        (void)pal4::EventCollectionProcessor::Process(
            output, 0x1000u, exceptional);
    } catch (const std::runtime_error&) {
        caught = true;
    }
    Check(caught && exceptional.calls.size() >= 5u &&
              exceptional.calls[exceptional.calls.size() - 4u] ==
                  "free:20992" &&
              exceptional.calls[exceptional.calls.size() - 3u] ==
                  "free:0" &&
              exceptional.calls[exceptional.calls.size() - 2u] ==
                  "free:20736" &&
              exceptional.calls.back() == "free:20480",
          "visitor exception releases child, buffer and outer iterators");

    Runtime no_wrapper;
    SeedSingleChild(no_wrapper);
    no_wrapper.fail_allocation = 5u;
    pal4::EventOutputState missing{};
    Check(pal4::EventCollectionProcessor::Process(
              missing, 0x1000u, no_wrapper) == 0x5000u &&
              missing.end == missing.begin + 4u &&
              no_wrapper.words.at(missing.begin) == 0u &&
              no_wrapper.temp_off_calls == 0u,
          "failed wrapper allocation still appends a null result");

    Runtime append_error;
    SeedSingleChild(append_error);
    append_error.throw_on_append = 0x5400u;
    pal4::EventOutputState rejected{};
    caught = false;
    try {
        (void)pal4::EventCollectionProcessor::Process(
            rejected, 0x1000u, append_error);
    } catch (const std::runtime_error&) {
        caught = true;
    }
    Check(caught && rejected.begin == 0u &&
              append_error.temp_off_calls == 1u &&
              append_error.calls[append_error.calls.size() - 4u] ==
                  "free:21504" &&
              append_error.calls[append_error.calls.size() - 3u] ==
                  "free:32768" &&
              append_error.calls[append_error.calls.size() - 2u] ==
                  "free:20736" &&
              append_error.calls.back() == "free:20480",
          "append exception frees wrapper, child buffer and outer iterators");
}

void TestProcessAndBind() {
    Runtime empty;
    SeedEmptyCollection(empty, 0x1000u, 0x2000u, 0x2100u);
    Check(pal4::EventCollectionProcessor::ProcessAndBind(
              0x9000u, 0x1000u, empty) == 1u &&
              empty.calls.back() == "free:0" &&
              std::none_of(empty.calls.begin(), empty.calls.end(),
                           [](const auto& call) {
                               return call.starts_with("bind:");
                           }),
          "empty processing frees null buffer and returns byte one");

    Runtime runtime;
    SeedSingleChild(runtime);
    runtime.words[0x1008u] = 0x2004u;
    runtime.words[0x2000u] = 0x3000u;
    runtime.visited[0x3000u] = 0xa0u;
    Check(pal4::EventCollectionProcessor::ProcessAndBind(
              0x9000u, 0x1000u, runtime) == 1u,
          "collected effects bind successfully");
    std::vector<std::string> bindings;
    for (const auto& call : runtime.calls)
        if (call.starts_with("bind:")) bindings.push_back(call);
    Check(bindings == std::vector<std::string>{
              "bind:36864:160", "bind:36864:21504"} &&
              runtime.calls.back() == "free:32768",
          "direct effect binds before wrapped child and buffer frees last");

    Runtime failure;
    SeedSingleChild(failure);
    failure.words[0x1008u] = 0x2004u;
    failure.words[0x2000u] = 0x3000u;
    failure.visited[0x3000u] = 0xa0u;
    failure.throw_on_bind = 0xa0u;
    bool caught = false;
    try {
        (void)pal4::EventCollectionProcessor::ProcessAndBind(
            0x9000u, 0x1000u, failure);
    } catch (const std::runtime_error&) {
        caught = true;
    }
    Check(caught && failure.calls.back() == "free:32768",
          "failed effect binding still frees collected pointer buffer");
}

}  // namespace

int main() {
    TestEmpty();
    TestRecursiveWrapping();
    TestFailures();
    TestProcessAndBind();
    pal4::EventCollectionRuntimeStub stub;
    Check(stub.Allocate(12u) == 0u && stub.ReadWord(0u) == 0u,
          "native event collection boundary has a linkable stub");
}
