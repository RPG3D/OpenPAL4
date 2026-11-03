#include "pal4/message_task.h"

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(const bool ok, const char* message) {
    if (!ok) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IMessageStringRuntime {
    pal4::MessageStringTemporary temporary{};
    std::map<std::uint32_t, std::uint8_t> counts;
    std::vector<std::string> calls;
    std::vector<std::uint32_t> freed;
    std::string reset_text;
    std::uint32_t copied_source = 0u;
    std::uint32_t copied_length = 0u;
    bool throw_on_copy = false;

    pal4::MessageStringTemporary ReadString(
        const std::uint32_t config, const std::string_view key,
        const std::uint32_t fallback) override {
        Check(config == 0x5000u && key == "Attr_Task_Message_What" &&
                  fallback == pal4::MessageTask::kEmptyConfigAddress,
              "XML reader key and default address");
        calls.emplace_back("read");
        return temporary;
    }
    std::uint8_t RefCount(const std::uint32_t data) override {
        return counts[data];
    }
    void SetRefCount(const std::uint32_t data,
                     const std::uint8_t value) override {
        counts[data] = value;
        calls.emplace_back("ref");
    }
    std::uint32_t ResetDestination(
        pal4::MessageTaskState& state, const std::uint32_t offset,
        const std::uint32_t flag) override {
        Check(offset == 72u && flag == 1u,
              "string destination reset arguments");
        calls.emplace_back("reset");
        state.words[19u] = 0u;
        state.words[20u] = 0u;
        state.words[21u] = 0u;
        return 0xabu;
    }
    void EraseDestination(pal4::MessageTaskState&,
                          const std::uint32_t start,
                          const std::uint32_t count) override {
        calls.emplace_back("erase:" + std::to_string(start) + ":" +
                           std::to_string(count));
    }
    bool AllocateDestination(pal4::MessageTaskState& state,
                             const std::uint32_t length,
                             const std::uint32_t mode) override {
        Check(mode == 1u, "allocation mode");
        calls.emplace_back("alloc");
        state.words[19u] = 0x9000u;
        state.words[21u] = length;
        return true;
    }
    void ReallocateDestination(pal4::MessageTaskState& state,
                               const std::uint32_t length) override {
        calls.emplace_back("realloc");
        state.words[19u] = 0xa000u;
        state.words[21u] = length;
    }
    void CopyBytes(const std::uint32_t destination,
                   const std::uint32_t source,
                   const std::uint32_t count) override {
        Check(destination == 0x9000u, "copy uses allocated destination");
        calls.emplace_back("copy");
        copied_source = source;
        copied_length = count;
        if (throw_on_copy)
            throw std::runtime_error("copy");
    }
    void CopyLiteral(const std::uint32_t destination,
                     const std::string_view text) override {
        Check(destination == 0xa000u && text == reset_text,
              "reset literal copied into resized destination");
        calls.emplace_back("literal");
    }
    void WriteNull(const std::uint32_t address) override {
        calls.emplace_back("null:" + std::to_string(address));
    }
    void Free(const std::uint32_t address) override {
        calls.emplace_back("free");
        freed.push_back(address);
    }
    std::string_view ResetText() override { return reset_text; }
};

void TestLoad() {
    pal4::MessageTaskState state{};
    Runtime runtime;
    state.words[19u] = 0x7000u;
    Check(pal4::MessageTask::LoadString(state, 0u, runtime) == 0u &&
              state.words[19u] == 0x7000u && runtime.calls.empty(),
          "null configuration leaves string intact");

    runtime.temporary = {0x8000u, 3u, 7u, false};
    runtime.counts[0x8000u] = 1u;
    Check(pal4::MessageTask::LoadString(state, 0x5000u, runtime) == 1u &&
              state.words[19u] == 0x8000u && state.words[20u] == 3u &&
              state.words[21u] == 7u && runtime.counts[0x8000u] == 1u &&
              runtime.calls == std::vector<std::string>{
                                   "read", "reset", "ref", "ref"},
          "shareable buffer increments owner ref then releases temporary");

    runtime.calls.clear();
    runtime.temporary = {0x8100u, 2u, 2u, false};
    runtime.counts[0x8100u] = 0xfeu;
    Check(pal4::MessageTask::LoadString(state, 0x5000u, runtime) == 1u &&
              state.words[19u] == 0x9000u && state.words[20u] == 2u &&
              runtime.copied_source == 0x8100u &&
              runtime.copied_length == 2u &&
              runtime.counts[0x8100u] == 0xfdu,
          "saturated source reference count forces allocation and copy");

    runtime.calls.clear();
    runtime.temporary = {0u, 0u, 0u, false};
    Check(pal4::MessageTask::LoadString(state, 0x5000u, runtime) == 1u &&
              runtime.copied_source == pal4::MessageTask::kEmptyTextAddress &&
              runtime.copied_length == 0u && state.words[20u] == 0u,
          "empty temporary copies native static empty string");

    runtime.calls.clear();
    runtime.temporary = {0u, 2u, 2u, true};
    Check(pal4::MessageTask::LoadString(state, 0x5000u, runtime) == 1u &&
              runtime.calls == std::vector<std::string>{
                                   "read", "erase:2:4294967295", "erase:0:0"},
          "self alias uses the two native erase operations");

    runtime.calls.clear();
    runtime.temporary = {0x8200u, 2u, 2u, false};
    runtime.counts[0x8200u] = 0xffu;
    runtime.throw_on_copy = true;
    bool caught = false;
    try {
        (void)pal4::MessageTask::LoadString(state, 0x5000u, runtime);
    } catch (const std::runtime_error&) {
        caught = true;
    }
    Check(caught && runtime.freed.back() == 0x81ffu,
          "copy exception releases temporary with sentinel reference count");
}

void TestReset() {
    pal4::MessageTaskState state{};
    Runtime runtime;
    state.words[19u] = 0x8012u;
    state.words[20u] = 4u;
    state.words[21u] = 8u;
    state.words[22u] = 0x6000u;
    runtime.counts[0x8012u] = 3u;
    Check(pal4::MessageTask::ResetString(state, runtime) == 0x12u &&
              runtime.counts[0x8012u] == 2u &&
              state.words[19u] == 0u && state.words[20u] == 0u &&
              state.words[21u] == 0u && state.words[22u] == 0u,
          "shared string decrements reference and clears four words");

    state.words[19u] = 0x9000u;
    state.words[22u] = 0x6000u;
    runtime.counts[0x9000u] = 0u;
    runtime.calls.clear();
    Check(pal4::MessageTask::ResetString(state, runtime) == 0xabu &&
              state.words[19u] == 0u && state.words[22u] == 0u &&
              runtime.calls == std::vector<std::string>{"reset"},
          "unique string uses native destination reset");

    runtime.reset_text = "abc";
    state.words[19u] = 0x9000u;
    state.words[21u] = 3u;
    state.words[22u] = 0x6000u;
    runtime.counts[0x9000u] = 2u;
    runtime.calls.clear();
    Check(pal4::MessageTask::ResetString(state, runtime) == 3u &&
              state.words[19u] == 0xa000u && state.words[20u] == 3u &&
              state.words[22u] == 0u &&
              runtime.calls == std::vector<std::string>{
                                   "realloc", "literal", "null:40963"},
          "nonempty reset text reallocates, copies and terminates");

    state.words[19u] = 0xa000u;
    state.words[21u] = 8u;
    state.words[22u] = 0x6000u;
    runtime.counts[0xa000u] = 0u;
    runtime.calls.clear();
    Check(pal4::MessageTask::ResetString(state, runtime) == 3u &&
              runtime.calls == std::vector<std::string>{
                                   "literal", "null:40963"} &&
              state.words[22u] == 0u,
          "short unique buffer reused without allocation");

    state.words[21u] = 1u;
    runtime.calls.clear();
    Check(pal4::MessageTask::ResetString(state, runtime) == 3u &&
              runtime.calls == std::vector<std::string>{
                                   "reset", "realloc", "literal", "null:40963"},
          "undersized unique buffer reset before reallocation");
}

}  // namespace

int main() {
    TestLoad();
    TestReset();
}
