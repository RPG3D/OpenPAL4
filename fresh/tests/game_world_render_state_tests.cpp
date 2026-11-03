#include "pal4/game_world_render_state.h"

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

struct Runtime final : pal4::IGameWorldRenderStateRuntime {
    std::map<std::uint32_t, std::uint32_t> words{
        {0x51f8u, 0x7000u}, {0x7004u, 13u}, {0x7008u, 3u},
        {0x700cu, 17u}, {0x7010u, 19u}, {0x7014u, 23u}};
    std::map<std::uint32_t, std::uint8_t> bytes{
        {0x7000u, 1u}, {0x7001u, 1u}};
    std::vector<std::string> events;
    std::uint32_t device = 0x9000u;

    std::uint32_t ReadWord(std::uint32_t address) override {
        events.push_back("word:" + std::to_string(address));
        return words[address];
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        events.push_back("byte:" + std::to_string(address));
        return bytes[address];
    }
    std::uint32_t GetD3DDevice() override {
        events.emplace_back("device");
        return device;
    }
    void SetRenderState(std::uint32_t handle, std::uint32_t state,
                        std::uint32_t value) override {
        Check(handle == device, "render call uses current device");
        events.push_back("set:" + std::to_string(state) + ":" +
                         std::to_string(value));
    }
};

using Events = std::vector<std::string>;

void Gates() {
    Runtime runtime;
    runtime.words[0x51f8u] = 0u;
    pal4::GameWorldRenderState::Apply(0x5000u, runtime);
    Check(runtime.events == Events{"word:20984"}, "null settings gate");

    runtime = Runtime{};
    runtime.bytes[0x7000u] = 0u;
    pal4::GameWorldRenderState::Clear(0x5000u, runtime);
    Check(runtime.events == Events{"word:20984", "byte:28672"},
          "inactive settings gate");

    runtime = Runtime{};
    runtime.device = 0u;
    pal4::GameWorldRenderState::Apply(0x5000u, runtime);
    Check(runtime.events == Events{"word:20984", "byte:28672", "device"},
          "missing device gate");

    runtime = Runtime{};
    runtime.bytes[0x7001u] = 0u;
    pal4::GameWorldRenderState::Apply(0x5000u, runtime);
    Check(runtime.events == Events{"word:20984", "byte:28672", "device",
                                   "byte:28673", "set:28:0"},
          "disabled mode only clears alpha state");
}

void ApplyModes() {
    Runtime runtime;
    pal4::GameWorldRenderState::Apply(0x5000u, runtime);
    Check(runtime.events == Events{
              "word:20984", "byte:28672", "device", "byte:28673",
              "word:28680", "set:35:3", "set:140:0", "set:48:0",
              "word:28676", "set:34:13", "set:28:1", "word:28680",
              "word:28684", "set:36:17", "word:28688", "set:37:19"},
          "mode 3 sets both blend states in order");

    runtime = Runtime{};
    runtime.words[0x7008u] = 2u;
    pal4::GameWorldRenderState::Apply(0x5000u, runtime);
    Check(runtime.events == Events{
              "word:20984", "byte:28672", "device", "byte:28673",
              "word:28680", "set:35:2", "set:140:0", "set:48:0",
              "word:28676", "set:34:13", "set:28:1", "word:28680",
              "word:28692", "set:38:23"},
          "other modes set state 38 only");
}

void Clear() {
    Runtime runtime;
    pal4::GameWorldRenderState::Clear(0x5000u, runtime);
    Check(runtime.events == Events{
              "word:20984", "byte:28672", "device", "set:28:0",
              "set:35:0", "set:140:0", "set:48:0"},
          "cleanup clears four states in IDA order");

    pal4::GameWorldRenderStateRuntimeStub stub;
    pal4::GameWorldRenderState::Apply(0x5000u, stub);
    pal4::GameWorldRenderState::Clear(0x5000u, stub);
}

}  // namespace

int main() {
    try {
        Gates();
        ApplyModes();
        Clear();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
