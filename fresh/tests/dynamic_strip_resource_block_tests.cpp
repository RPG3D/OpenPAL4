#include "pal4/dynamic_strip_resource_block.h"

#include <cstdint>
#include <array>
#include <cstdlib>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IDynamicStripResourceBlockRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> memory;
    std::vector<std::string> calls;
    std::vector<std::uint32_t> freed;
    bool drain_in_base = false;

    void CleanupVisual(pal4::DynamicStripObjectState& state) override {
        calls.push_back("visual");
        Check(state.words[44u] == 0x2000u && state.words[41u] == 0x3000u,
              "native visual cleanup runs before owned storage is released");
        if (drain_in_base) {
            memory[0x3000u] = 0x3000u;
            memory[0x3004u] = 0x3000u;
            state.words[42u] = 0u;
        }
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        Check(memory.contains(address), "list link exists");
        calls.push_back("read");
        return memory.at(address);
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        calls.push_back("write");
        memory[address] = value;
    }
    void Free(std::uint32_t address) override {
        calls.push_back("free");
        freed.push_back(address);
    }
};

void TestNodes() {
    pal4::DynamicStripObjectState state{};
    state.words[44u] = 0x2000u;
    state.words[45u] = 0x2008u;
    state.words[46u] = 0x2010u;
    state.words[41u] = 0x3000u;
    state.words[42u] = 2u;
    Runtime runtime;
    runtime.memory = {
        {0x3000u, 0x3100u}, {0x3004u, 0x3200u},
        {0x3100u, 0x3200u}, {0x3104u, 0x3000u},
        {0x3200u, 0x3000u}, {0x3204u, 0x3100u}};
    pal4::DynamicStripResourceBlock::Cleanup(state, runtime);
    Check(runtime.freed ==
              std::vector<std::uint32_t>{
                  0x2000u, 0x3100u, 0x3200u, 0x3000u} &&
              state.words[44u] == 0u && state.words[45u] == 0u &&
              state.words[46u] == 0u && state.words[41u] == 0u &&
              state.words[42u] == 0u &&
              runtime.memory[0x3000u] == 0x3000u &&
              runtime.memory[0x3004u] == 0x3000u &&
              runtime.calls.front() == "visual",
          "cleanup unlinks nodes and frees buffer before sentinel");
}

void TestAlreadyDrained() {
    pal4::DynamicStripObjectState state{};
    state.words[44u] = 0x2000u;
    state.words[41u] = 0x3000u;
    state.words[42u] = 1u;
    Runtime runtime;
    runtime.drain_in_base = true;
    runtime.memory[0x3000u] = 0x3100u;
    runtime.memory[0x3004u] = 0x3100u;
    pal4::DynamicStripResourceBlock::Cleanup(state, runtime);
    Check(runtime.freed ==
              std::vector<std::uint32_t>{0x2000u, 0x3000u} &&
              state.words[42u] == 0u,
          "already emptied visual list only frees its sentinel");
}

void TestEmbeddedStorage() {
    struct Guarded {
        std::uint32_t before=0x12345678u;
        std::array<std::uint32_t,34> words{};
        std::uint32_t after=0x87654321u;
    } state;
    state.words[31u]=0x2000u;
    state.words[28u]=0x3000u;
    state.words[29u]=2u;
    Runtime runtime;
    runtime.memory = {
        {0x3000u, 0x3100u}, {0x3004u, 0x3200u},
        {0x3100u, 0x3200u}, {0x3104u, 0x3000u},
        {0x3200u, 0x3000u}, {0x3204u, 0x3100u}};
    const pal4::StripVisualBlockView view{state.words};
    pal4::DynamicStripResourceBlock::ReleaseStorage(view,runtime);
    Check(runtime.freed == std::vector<std::uint32_t>{0x2000u,0x3100u,0x3200u,0x3000u} &&
              !state.words[31u] && !state.words[28u] && !state.words[29u],
          "136-byte storage release retires buffer, retained inactive nodes and sentinel");
    pal4::DynamicStripResourceBlock::ReleaseStorage(view,runtime);
    Check(runtime.freed.back()==0u && state.before==0x12345678u && state.after==0x87654321u,
          "repeated empty embedded release does not read null sentinel or touch neighbors");
}

}  // namespace

int main() {
    TestNodes();
    TestAlreadyDrained();
    TestEmbeddedStorage();
}
