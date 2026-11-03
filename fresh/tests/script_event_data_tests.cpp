#include "pal4/script_event_data.h"

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

struct Runtime final : pal4::IScriptEventDataRuntime {
    pal4::ScriptEventDataState* state = nullptr;
    std::vector<std::string> calls;
    std::uint8_t count = 3u;
    std::uint8_t written = 0u;

    void DestroyXmlNode(std::uint32_t address) override {
        calls.push_back("destroy:" + std::to_string(address));
        Check(state->words[9] == 0u || state->words[9] == address,
              "owned node is cleared after its destructor and free");
    }

    std::uint32_t Free(std::uint32_t address) override {
        calls.push_back("free:" + std::to_string(address));
        return address ^ 0x55u;
    }

    std::uint8_t ReadByte(std::uint32_t address) override {
        calls.push_back("read:" + std::to_string(address));
        Check(state->words[6] == 0u && state->words[7] == 0u &&
                  state->words[8] == 0u,
              "collection fields are cleared before string reference release");
        return count;
    }

    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        calls.push_back("write:" + std::to_string(address));
        written = value;
        Check(state->words[1] == address + 1u,
              "string fields are cleared after reference decrement");
    }
};

struct StringRuntime final : pal4::IScriptEventStringRuntime {
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> calls;
    std::uint32_t copied_length = 0u;

    void ThrowLength() override {
        throw std::length_error("length");
    }
    std::uint8_t SetErrorFlag(pal4::ScriptEventDataState& state,
                              std::uint8_t flag) override {
        calls.emplace_back("reset");
        Check(flag == 1u, "string reset flag");
        state.words[1] = 0u;
        state.words[2] = 0u;
        state.words[3] = 0u;
        return 0x5au;
    }
    void Reallocate(pal4::ScriptEventDataState& state,
                    std::uint32_t length) override {
        calls.emplace_back("reallocate");
        state.words[1] = 0x7000u;
        state.words[3] = length;
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        calls.emplace_back("read");
        return bytes.at(address);
    }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        calls.emplace_back("write");
        bytes[address] = value;
    }
    void Copy(std::uint32_t destination, const char* source,
              std::uint32_t length) override {
        calls.emplace_back("copy");
        copied_length = length;
        for (std::uint32_t index = 0; index < length; ++index)
            bytes[destination + index] =
                static_cast<std::uint8_t>(source[index]);
    }
};

struct DocumentRuntime final : pal4::IScriptEventDocumentRuntime {
    std::uint32_t allocated = 0x9000u;
    std::uint32_t initialized = 0x9100u;
    bool throw_on_initialize = false;
    std::vector<std::string> calls;

    std::uint32_t Allocate(std::uint32_t size) override {
        calls.emplace_back("allocate");
        Check(size == 60u, "condition document allocation size");
        return allocated;
    }
    std::uint32_t InitializeDocument(
        std::uint32_t address, const char* name,
        std::uint32_t option) override {
        calls.emplace_back("initialize");
        Check(address == allocated && option == 0u &&
                  std::string(name) == "behAIAtomic-condition-attribute",
              "condition document constructor arguments");
        if (throw_on_initialize) throw std::runtime_error("constructor");
        return initialized;
    }
    void Free(std::uint32_t address) override {
        calls.emplace_back("free");
        Check(address == allocated, "constructor failure frees allocation");
    }
};

void TestInitializePreservesUnwrittenBytes() {
    pal4::ScriptEventDataState state;
    state.words.fill(0xabcdef12u);
    Check(&pal4::ScriptEventData::Initialize(state, 0x89123456u) == &state &&
              state.words[0] == 0xabcdef89u &&
              state.words[5] == 0xabcdef89u && state.words[4] == 3u,
          "initializer uses high byte of saved object pointer");
    for (const auto index : {1u, 2u, 3u, 6u, 7u, 8u, 9u})
        Check(state.words[index] == 0u, "initializer clears observed words");
}

void TestOwnedNodeCollectionAndSharedString() {
    pal4::ScriptEventDataState state;
    state.words = {0x22u, 0x2001u, 13u, 20u, 3u,
                   0x33u, 0x1000u, 0x1078u, 0x1100u, 0x9000u};
    Runtime runtime;
    runtime.state = &state;
    Check(pal4::ScriptEventData::Cleanup(state, runtime) == 0x2000u &&
              state.words == std::array<std::uint32_t, 10>{
                  0x22u, 0u, 0u, 0u, 3u, 0x33u, 0u, 0u, 0u, 0u} &&
              runtime.written == 2u &&
              runtime.calls == std::vector<std::string>{
                  "destroy:36864", "free:36864", "destroy:4096",
                  "destroy:4156", "free:4096", "read:8192", "write:8192"},
          "cleanup order, 60-byte stride and shared string decrement");
}

void TestZeroAndSentinelReferenceCounts() {
    for (const auto count : {std::uint8_t{0u}, std::uint8_t{0xffu}}) {
        pal4::ScriptEventDataState state;
        state.words[1] = 0x3001u;
        Runtime runtime;
        runtime.state = &state;
        runtime.count = count;
        Check(pal4::ScriptEventData::Cleanup(state, runtime) ==
                  (0x3000u ^ 0x55u) &&
                  state.words[1] == 0u &&
                  runtime.calls == std::vector<std::string>{
                      "free:0", "read:12288", "free:12288"},
              "zero and sentinel reference headers are freed");
    }
}

void TestEmptyCollectionAndNullString() {
    pal4::ScriptEventDataState state;
    state.words[6] = 0x5000u;
    state.words[7] = 0x5000u;
    state.words[8] = 0x6000u;
    Runtime runtime;
    runtime.state = &state;
    Check(pal4::ScriptEventData::Cleanup(state, runtime) == 0u &&
              runtime.calls == std::vector<std::string>{"free:20480"},
          "empty collection still frees its begin pointer");
    runtime.calls.clear();
    Check(pal4::ScriptEventData::Cleanup(state, runtime) == 0u &&
              runtime.calls == std::vector<std::string>{"free:0"},
          "repeated cleanup still calls free on null collection pointer");
}

void TestAssignSharedBuffer() {
    pal4::ScriptEventDataState state;
    state.words[0] = 0x12345678u;
    state.words[1] = 0x5101u;
    state.words[3] = 20u;
    StringRuntime runtime;
    runtime.bytes[0x5100u] = 3u;
    Check(pal4::ScriptEventData::AssignCString(state, "abc", runtime) == 3u &&
              state.words[0] == 0x12345678u &&
              state.words[1] == 0x7000u && state.words[2] == 3u &&
              runtime.bytes[0x7000u] == 'a' &&
              runtime.bytes[0x7003u] == 0u &&
              runtime.calls == std::vector<std::string>{
                  "read", "reallocate", "copy", "write"},
          "shared string always reallocates for nonempty assignment");

    state.words[1] = 0x5101u;
    state.words[2] = 5u;
    runtime.calls.clear();
    Check(pal4::ScriptEventData::AssignCString(state, "", runtime) == 1u &&
              state.words[1] == 0u && state.words[2] == 0u &&
              state.words[3] == 0u && runtime.bytes[0x5100u] == 2u &&
              runtime.calls == std::vector<std::string>{"read", "write"},
          "empty assignment decrements shared header and returns pointer low byte");
}

void TestAssignUniqueAndCapacityPaths() {
    pal4::ScriptEventDataState state;
    state.words[1] = 0x6001u;
    state.words[3] = 12u;
    StringRuntime runtime;
    runtime.bytes[0x6000u] = 0u;
    Check(pal4::ScriptEventData::AssignCString(state, "hello", runtime) == 5u &&
              state.words[1] == 0x6001u && state.words[2] == 5u &&
              state.words[3] == 12u && runtime.bytes[0x6006u] == 0u &&
              runtime.calls == std::vector<std::string>{"read", "copy", "write"},
          "small unique buffer with enough capacity reuses storage");

    runtime.calls.clear();
    state.words[3] = 32u;
    Check(pal4::ScriptEventData::AssignCString(state, "x", runtime) == 1u &&
              state.words[1] == 0x7000u &&
              runtime.calls == std::vector<std::string>{
                  "read", "reset", "reallocate", "copy", "write"},
          "capacity above 31 triggers reset and reallocation");

    runtime.calls.clear();
    state.words[1] = 0x6001u;
    state.words[3] = 1u;
    runtime.bytes[0x6000u] = 0xffu;
    Check(pal4::ScriptEventData::AssignCString(state, "long", runtime) == 4u &&
              runtime.calls == std::vector<std::string>{
                  "read", "reset", "reallocate", "copy", "write"},
          "sentinel header with insufficient capacity reallocates");
}

void TestAssignEmptyUniqueBuffer() {
    pal4::ScriptEventDataState state;
    state.words[1] = 0x8001u;
    state.words[2] = 4u;
    state.words[3] = 8u;
    StringRuntime runtime;
    runtime.bytes[0x8000u] = 0xffu;
    Check(pal4::ScriptEventData::AssignCString(state, "", runtime) == 0x5au &&
              state.words[1] == 0u && state.words[2] == 0u &&
              state.words[3] == 0u &&
              runtime.calls == std::vector<std::string>{"read", "reset"},
          "empty unique assignment returns reset callback low byte");
}

void TestGetOrCreateConditionDocument() {
    pal4::ScriptEventDataState state;
    DocumentRuntime runtime;
    state.words[9] = 0x8000u;
    Check(pal4::ScriptEventData::GetOrCreateConditionDocument(
              state, runtime) == 0x8000u && runtime.calls.empty(),
          "existing condition document is returned without allocation");

    state.words[9] = 0u;
    runtime.allocated = 0u;
    Check(pal4::ScriptEventData::GetOrCreateConditionDocument(
              state, runtime) == 0u &&
              runtime.calls == std::vector<std::string>{"allocate"},
          "failed allocation does not call document constructor");

    runtime.calls.clear();
    runtime.allocated = 0x9000u;
    Check(pal4::ScriptEventData::GetOrCreateConditionDocument(
              state, runtime) == 0x9100u && state.words[9] == 0x9100u &&
              runtime.calls == std::vector<std::string>{
                  "allocate", "initialize"},
          "new condition document is stored and returned");
}

void TestConditionDocumentConstructorFailure() {
    pal4::ScriptEventDataState state;
    DocumentRuntime runtime;
    runtime.throw_on_initialize = true;
    bool caught = false;
    try {
        static_cast<void>(pal4::ScriptEventData::GetOrCreateConditionDocument(
            state, runtime));
    } catch (const std::runtime_error&) {
        caught = true;
    }
    Check(caught && state.words[9] == 0u &&
              runtime.calls == std::vector<std::string>{
                  "allocate", "initialize", "free"},
          "constructor exception frees allocation before propagation");
}

struct CompareRuntime final : pal4::IScriptEventCompareRuntime {
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::uint32_t> reads;
    std::uint8_t ReadByte(std::uint32_t address) override {
        reads.push_back(address);
        return bytes.at(address);
    }
};

void TestCompareCString() {
    pal4::ScriptEventDataState state;
    state.words[1] = 0x1000u;
    state.words[2] = 3u;
    CompareRuntime runtime;
    runtime.bytes = {{0x1000u, 'a'}, {0x1001u, 'b'},
                     {0x1002u, 'c'}};
    Check(pal4::ScriptEventData::CompareCString(
              state, "abc", runtime) == 0 &&
              runtime.reads == std::vector<std::uint32_t>{
                  0x1000u, 0x1001u, 0x1002u},
          "identical strings compare equal over exact stored length");
    runtime.reads.clear();
    Check(pal4::ScriptEventData::CompareCString(
              state, "abd", runtime) == -1 &&
              runtime.reads.size() == 3u &&
              pal4::ScriptEventData::CompareCString(
                  state, "abb", runtime) == 1,
          "different bytes produce normalized unsigned comparison signs");
    state.words[2] = 2u;
    Check(pal4::ScriptEventData::CompareCString(
              state, "abc", runtime) == -1,
          "shorter stored prefix sorts before longer input");
    state.words[2] = 3u;
    Check(pal4::ScriptEventData::CompareCString(
              state, "ab", runtime) == 1,
          "longer stored prefix sorts after shorter input");
    state.words[2] = 0u;
    state.words[1] = 0u;
    runtime.reads.clear();
    Check(pal4::ScriptEventData::CompareCString(
              state, "", runtime) == 0 && runtime.reads.empty(),
          "empty strings do not dereference a null data pointer");
    runtime.bytes[0x1000u] = 0xffu;
    state.words[1] = 0x1000u;
    state.words[2] = 1u;
    const char high_byte[] = {static_cast<char>(0x80), '\0'};
    Check(pal4::ScriptEventData::CompareCString(
              state, high_byte, runtime) == 1,
          "comparison treats bytes above 127 as unsigned");
}

}  // namespace

int main() {
    TestInitializePreservesUnwrittenBytes();
    TestOwnedNodeCollectionAndSharedString();
    TestZeroAndSentinelReferenceCounts();
    TestEmptyCollectionAndNullString();
    TestAssignSharedBuffer();
    TestAssignUniqueAndCapacityPaths();
    TestAssignEmptyUniqueBuffer();
    TestGetOrCreateConditionDocument();
    TestConditionDocumentConstructorFailure();
    TestCompareCString();
    pal4::ScriptEventDataState state;
    pal4::ScriptEventDataRuntimeStub stub;
    Check(pal4::ScriptEventData::Cleanup(state, stub) == 0u,
          "default runtime links");
    pal4::ScriptEventStringRuntimeStub string_stub;
    Check(pal4::ScriptEventData::AssignCString(state, "", string_stub) == 0u,
          "default string runtime links");
    pal4::ScriptEventDocumentRuntimeStub document_stub;
    Check(pal4::ScriptEventData::GetOrCreateConditionDocument(
              state, document_stub) == 0u,
          "default document runtime links");
    pal4::ScriptEventCompareRuntimeStub compare_stub;
    Check(pal4::ScriptEventData::CompareCString(
              state, "", compare_stub) == 0,
          "default comparison runtime links");
    std::cout << "10 script event data cases passed\n";
}
