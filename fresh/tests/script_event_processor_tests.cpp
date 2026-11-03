#include "pal4/script_event_processor.h"

#include <array>
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

struct Runtime final : pal4::IScriptEventProcessorRuntime {
    std::uint32_t allocation = 0x2000u;
    bool throw_on_initialize = false;
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> calls;

    std::uint32_t Allocate(std::uint32_t bytes) override {
        calls.push_back("alloc:" + std::to_string(bytes));
        return allocation;
    }
    std::uint32_t InitializeDocument(
        std::uint32_t address, const char* name,
        std::uint32_t option) override {
        calls.push_back("initdoc:" + std::to_string(address) + ":" +
                        name + ":" + std::to_string(option));
        if (throw_on_initialize)
            throw std::runtime_error("XML constructor failed");
        return address + 0x100u;
    }
    void CleanupDocument(std::uint32_t address) override {
        calls.push_back("cleanupdoc:" + std::to_string(address));
    }
    void DestroyDocument(std::uint32_t address) override {
        calls.push_back("destroydoc:" + std::to_string(address));
    }
    std::uint32_t ReadDword(std::uint32_t address) override {
        calls.push_back("read:" + std::to_string(address));
        return words.at(address);
    }
    void CleanupEventData(std::uint32_t address) override {
        calls.push_back("clearevent:" + std::to_string(address));
    }
    std::uint32_t Free(std::uint32_t address) override {
        calls.push_back("free:" + std::to_string(address));
        return address + 1u;
    }
};

void TestInitialize() {
    pal4::ScriptEventProcessorState state;
    state.words.fill(0xa5a5a5a5u);
    Runtime runtime;
    Check(pal4::ScriptEventProcessor::Initialize(
              state, 0x1000u, 0x7bu, runtime) == 0x1000u &&
              state.words == std::array<std::uint32_t, 12>{
                  0xa5a5a57bu, 0u, 0u, 0u, 0x2100u, 0u,
                  0xa5a5a500u, 0u, 0xa5a5a57bu, 0u, 0u, 0u} &&
              runtime.calls == std::vector<std::string>{
                  "alloc:60",
                  "initdoc:8192:behAIAtomicMgr-memory:0"},
          "initializer preserves unwritten bytes and creates XML document");

    state.words.fill(0xa5a5a5a5u);
    runtime.calls.clear();
    runtime.allocation = 0u;
    Check(pal4::ScriptEventProcessor::Initialize(
              state, 0x1000u, 0x22u, runtime) == 0x1000u &&
              state.words[4] == 0u &&
              state.words[0] == 0xa5a5a522u &&
              state.words[8] == 0xa5a5a522u &&
              runtime.calls == std::vector<std::string>{"alloc:60"},
          "allocation failure leaves a null document without constructor call");

    state.words.fill(0xa5a5a5a5u);
    runtime.calls.clear();
    runtime.allocation = 0x2000u;
    runtime.throw_on_initialize = true;
    bool threw = false;
    try {
        (void)pal4::ScriptEventProcessor::Initialize(
            state, 0x1000u, 0x33u, runtime);
    } catch (const std::runtime_error&) {
        threw = true;
    }
    Check(threw && state.words[4] == 0xa5a5a5a5u &&
              runtime.calls == std::vector<std::string>{
                  "alloc:60", "initdoc:8192:behAIAtomicMgr-memory:0",
                  "free:8192"},
          "constructor exception frees the allocated document buffer");
}

void TestClearEvents() {
    pal4::ScriptEventProcessorState state;
    state.words = {0x11u, 0x1000u, 0x100cu, 0x1100u, 0x2000u,
                   0x55u, 0x66u, 0x77u, 0x88u, 0x3000u, 0x3008u,
                   0x3100u};
    Runtime runtime;
    runtime.words = {{0x1000u, 0x5000u}, {0x1004u, 0u},
                     {0x1008u, 0x6000u}};
    Check(pal4::ScriptEventProcessor::ClearEvents(state, runtime) ==
              0x100cu &&
              state.words == std::array<std::uint32_t, 12>{
                  0x11u, 0x1000u, 0x1000u, 0x1100u, 0x2000u,
                  0u, 0x66u, 0x77u, 0x88u, 0x3000u, 0x3000u,
                  0x3100u} &&
              runtime.calls == std::vector<std::string>{
                  "read:4096", "clearevent:20480", "free:20480",
                  "read:4100", "read:4104", "clearevent:24576",
                  "free:24576"},
          "clear walks four-byte pointers, skips null and resets both ends");
}

void TestCleanup() {
    pal4::ScriptEventProcessorState state;
    state.words = {0x11u, 0x1000u, 0x1004u, 0x1100u, 0x2000u,
                   0x55u, 0x66u, 0x77u, 0x88u, 0x3000u, 0x3004u,
                   0x3100u};
    Runtime runtime;
    runtime.words[0x1000u] = 0x5000u;
    Check(pal4::ScriptEventProcessor::Cleanup(state, runtime) ==
              0x1001u &&
              state.words == std::array<std::uint32_t, 12>{
                  0x11u, 0u, 0u, 0u, 0u, 0u,
                  0x66u, 0x77u, 0x88u, 0u, 0u, 0u} &&
              runtime.calls == std::vector<std::string>{
                  "cleanupdoc:8192", "destroydoc:8192",
                  "free:8192", "read:4096", "clearevent:20480",
                  "free:20480", "free:12288", "free:4096"},
          "cleanup releases document, events and both arrays in assembly order");

    runtime.calls.clear();
    Check(pal4::ScriptEventProcessor::Cleanup(state, runtime) == 1u &&
              runtime.calls == std::vector<std::string>{
                  "free:0", "free:0"},
          "empty cleanup still frees both null arrays and returns last result");
}

void TestFlagSetter() {
    pal4::ScriptEventProcessorState state;
    state.words[6] = 0x12345678u;
    Check(pal4::ScriptEventProcessor::SetFlag24(state, 0xabu) == 0xabu &&
              state.words[6] == 0x123456abu,
          "flag setter changes only the byte at offset 24");
}

void TestFirstValue() {
    pal4::ScriptEventProcessorState state;
    Check(pal4::ScriptEventProcessor::SetFirstValue28(state, 0u) == 0u &&
              pal4::ScriptEventProcessor::SetFirstValue28(
                  state, 0x1234u) == 0x1234u &&
              pal4::ScriptEventProcessor::SetFirstValue28(
                  state, 0x5678u) == 0x1234u &&
              state.words[7] == 0x1234u,
          "first-value setter keeps the initial nonzero value");
}

}  // namespace

int main() {
    TestInitialize();
    TestClearEvents();
    TestCleanup();
    TestFlagSetter();
    TestFirstValue();
    pal4::ScriptEventProcessorState state;
    pal4::ScriptEventProcessorRuntimeStub stub;
    Check(pal4::ScriptEventProcessor::Initialize(
              state, 0x1000u, 0u, stub) == 0x1000u &&
              pal4::ScriptEventProcessor::Cleanup(state, stub) == 0u,
          "default processor runtime links");
    std::cout << "9 script event processor cases passed\n";
}
