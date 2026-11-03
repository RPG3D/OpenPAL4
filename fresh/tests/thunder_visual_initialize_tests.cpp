#include "pal4/thunder_visual_storage.h"

#include <bit>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace {

void Check(bool good, const char* message) {
    if (!good) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IThunderVisualInitializationRuntime {
    std::map<std::uint32_t, std::uint32_t> memory;
    std::vector<std::string> calls;
    std::uint32_t next_frame = 0x900u;
    void ResetProperties(pal4::ThunderObjectState& state) override {
        calls.push_back("reset");
        state.words[15u] = 0xaabbcc00u;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.push_back("read");
        return memory.at(address);
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        calls.push_back("write");
        memory[address] = value;
    }
    std::uint32_t CreateFrame() override {
        calls.push_back("create");
        const auto result = next_frame;
        next_frame += 0x10u;
        return result;
    }
    void SetupAndUpdateFrame(pal4::ThunderObjectState& state) override {
        Check(state.words[28u] == 0x900u && state.words[34u] == 0u,
              "first frame created before setup and second after");
        calls.push_back("setup");
    }
    void SetFrameIdentity(std::uint32_t frame) override {
        Check(frame == 0x910u, "identity on second frame");
        calls.push_back("identity");
    }
};

void MissingTexture() {
    Runtime runtime;
    pal4::ThunderObjectState state;
    state.words[0u] = 0xdeadbeefu;
    Check(pal4::ThunderVisualStorage::InitializeWithTexture(
              state, 0u, 30.0f, 7u, runtime) == 0u,
          "null texture rejected");
    Check(runtime.calls == std::vector<std::string>{"reset"} &&
              state.words[15u] == 0xaabbcc00u &&
              state.words[0u] == 0xdeadbeefu,
          "visual properties reset before null texture gate");
}

void Initialized(float fps, std::uint32_t expected_reciprocal) {
    Runtime runtime;
    runtime.memory[0x854u] = 4u;
    pal4::ThunderObjectState state;
    state.words[0u] = 0xdeadbeefu;
    state.words[36u] = 0x12345678u;
    Check(pal4::ThunderVisualStorage::InitializeWithTexture(
              state, 0x800u, fps, 7u, runtime) == 1u,
          "non-null texture initializes visual");
    Check(runtime.memory[0x854u] == 5u && state.words[29u] == 0x800u,
          "texture reference incremented");
    Check(state.words[35u] == expected_reciprocal && state.words[36u] == 0u,
          "fps reciprocal and timer reset");
    Check(state.words[28u] == 0x900u && state.words[34u] == 0x910u &&
              state.words[15u] == 0xaa07cc01u &&
              state.words[0u] == 0xdeadbeefu,
          "new frame handles and active/mode bytes");
    Check(runtime.calls == std::vector<std::string>{
              "reset", "read", "write", "create", "setup", "create",
              "identity"}, "native initialization order");
}

}  // namespace

int main() {
    MissingTexture();
    Initialized(4.0f, std::bit_cast<std::uint32_t>(0.25f));
    Initialized(0.0f, 0u);
    Initialized(-3.0f, 0u);
}
