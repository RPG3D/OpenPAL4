#include "pal4/script_event_processor.h"

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

struct Runtime final : pal4::IScriptEventInsertRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::map<std::uint32_t, std::string> text;
    std::map<std::uint32_t, std::string> event_names;
    std::vector<std::string> calls;
    bool allow_text = true;
    bool throw_on_event_init = false;
    std::uint32_t event_allocation = 0x7000u;

    Runtime() { text[0x841964u] = ""; }

    bool AllocateText(pal4::StringBufferState& state,
                      std::uint32_t length, std::uint8_t mode) override {
        calls.push_back("textalloc:" + std::to_string(length) + ":" +
                        std::to_string(mode));
        if (!allow_text) return false;
        state = {0x4001u, 0u, length};
        bytes[0x4000u] = 0u;
        return true;
    }
    void CopyText(std::uint32_t destination, const char* source,
                  std::uint32_t length) override {
        calls.push_back("copy:" + std::to_string(destination));
        text[destination] = std::string(source, length);
    }
    void SetTextSize(pal4::StringBufferState& state,
                     std::uint32_t length) override {
        calls.push_back("size:" + std::to_string(length));
        state.length = length;
    }
    void AssignName(pal4::StringBufferState& destination,
                    const pal4::StringBufferState& source) override {
        calls.push_back("assignname:" + std::to_string(source.data));
        destination = source;
        if (source.data != 0u) ++bytes.at(source.data - 1u);
    }
    std::uint32_t EmptyStringAddress() override {
        calls.push_back("empty");
        return 0x841964u;
    }
    std::int32_t CompareEventString(std::uint32_t event,
                                    std::uint32_t query) override {
        calls.push_back("compare:" + std::to_string(event) + ":" +
                        std::to_string(query));
        return event_names.at(event) == text.at(query) ? 0 : 1;
    }
    std::uint32_t Allocate(std::uint32_t bytes_count) override {
        calls.push_back("alloc:" + std::to_string(bytes_count));
        return event_allocation;
    }
    std::uint32_t InitializeEventData(std::uint32_t address) override {
        calls.push_back("initevent:" + std::to_string(address));
        if (throw_on_event_init)
            throw std::runtime_error("event constructor failed");
        return address;
    }
    void AssignEventCString(std::uint32_t event,
                            const char* source) override {
        calls.push_back("assignevent:" + std::to_string(event));
        event_names[event] = source;
    }
    void InsertEventPointer(pal4::ScriptEventProcessorState& state,
                            std::uint32_t position,
                            std::uint32_t event) override {
        calls.push_back("insert:" + std::to_string(position) + ":" +
                        std::to_string(event));
        if (state.words[1] == 0u) {
            state.words[1] = 0x1000u;
            state.words[2] = 0x1000u;
            state.words[3] = 0x1100u;
        }
        words[state.words[2]] = event;
        state.words[2] += 4u;
    }
    std::uint32_t ReadDword(std::uint32_t address) override {
        calls.push_back("read:" + std::to_string(address));
        return words.at(address);
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        calls.push_back("readbyte:" + std::to_string(address));
        return bytes.at(address);
    }
    void WriteByte(std::uint32_t address,
                   std::uint8_t value) override {
        calls.push_back("writebyte:" + std::to_string(address) + ":" +
                        std::to_string(value));
        bytes[address] = value;
    }
    void Free(std::uint32_t address) override {
        calls.push_back("free:" + std::to_string(address));
    }
};

bool HasCall(const Runtime& runtime, const std::string& call) {
    for (const auto& item : runtime.calls)
        if (item == call) return true;
    return false;
}

void TestExistingEvent() {
    pal4::ScriptEventProcessorState state;
    state.words[1] = 0x1000u;
    state.words[2] = 0x1008u;
    state.words[5] = 0x5555u;
    Runtime runtime;
    runtime.words = {{0x1000u, 0x5000u}, {0x1004u, 0x6000u}};
    runtime.event_names = {{0x5000u, "other"}, {0x6000u, "target"}};
    Check(pal4::ScriptEventProcessor::ProcessAndAddEvent(
              state, "target", runtime) == 0u &&
              state.words[2] == 0x1008u && state.words[5] == 0x5555u &&
              runtime.calls == std::vector<std::string>{
                  "textalloc:6:1", "copy:16385", "size:6",
                  "assignname:16385", "readbyte:16384",
                  "writebyte:16384:0", "assignname:16385",
                  "read:4096", "compare:20480:16385",
                  "read:4100", "compare:24576:16385",
                  "readbyte:16384", "writebyte:16384:0",
                  "readbyte:16384", "free:16384"},
          "duplicate event scans in order, releases names and leaves array intact");
}

void TestNewEvent() {
    pal4::ScriptEventProcessorState state;
    state.words[1] = 0x1000u;
    state.words[2] = 0x1004u;
    Runtime runtime;
    runtime.words[0x1000u] = 0x5000u;
    runtime.event_names[0x5000u] = "other";
    Check(pal4::ScriptEventProcessor::ProcessAndAddEvent(
              state, "new", runtime) == 0x7000u &&
              state.words[2] == 0x1008u &&
              state.words[5] == 0x7000u &&
              runtime.words.at(0x1004u) == 0x7000u &&
              HasCall(runtime, "alloc:40") &&
              HasCall(runtime, "initevent:28672") &&
              HasCall(runtime, "assignevent:28672") &&
              HasCall(runtime, "insert:4100:28672") &&
              runtime.calls.back() == "free:16384",
          "new event is assigned, appended and returned from last array slot");
}

void TestEmptyFallback() {
    pal4::ScriptEventProcessorState state;
    state.words[1] = 0x1000u;
    state.words[2] = 0x1004u;
    Runtime runtime;
    runtime.allow_text = false;
    runtime.words[0x1000u] = 0x5000u;
    runtime.event_names[0x5000u] = "";
    Check(pal4::ScriptEventProcessor::ProcessAndAddEvent(
              state, "input", runtime) == 0u &&
              HasCall(runtime, "empty") &&
              HasCall(runtime, "compare:20480:8657252") &&
              !HasCall(runtime, "alloc:40"),
          "missing temporary text uses the global empty string during scan");
}

void TestEventConstructorFailure() {
    pal4::ScriptEventProcessorState state;
    Runtime runtime;
    runtime.throw_on_event_init = true;
    bool threw = false;
    try {
        (void)pal4::ScriptEventProcessor::ProcessAndAddEvent(
            state, "new", runtime);
    } catch (const std::runtime_error&) {
        threw = true;
    }
    Check(threw && state.words[2] == 0u &&
              HasCall(runtime, "alloc:40") &&
              HasCall(runtime, "free:28672") &&
              runtime.calls.back() == "free:16384" &&
              !HasCall(runtime, "insert:0:28672"),
          "constructor failure frees allocated event and remaining name");
}

void TestNullEventAllocation() {
    pal4::ScriptEventProcessorState state;
    Runtime runtime;
    runtime.event_allocation = 0u;
    Check(pal4::ScriptEventProcessor::ProcessAndAddEvent(
              state, "new", runtime) == 0u &&
              state.words[5] == 0u &&
              HasCall(runtime, "assignevent:0") &&
              HasCall(runtime, "insert:0:0"),
          "null allocation still reaches unguarded event assignment and insert");
}

}  // namespace

int main() {
    TestExistingEvent();
    TestNewEvent();
    TestEmptyFallback();
    TestEventConstructorFailure();
    TestNullEventAllocation();
    pal4::ScriptEventProcessorState state;
    pal4::ScriptEventInsertRuntimeStub stub;
    Check(pal4::ScriptEventProcessor::ProcessAndAddEvent(
              state, "stub", stub) == 0u,
          "linkable insert runtime runs without external services");
    std::cout << "6 script event insertion cases passed\n";
}
