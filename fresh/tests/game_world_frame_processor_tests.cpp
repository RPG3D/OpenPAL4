#include "pal4/game_world_frame_processor.h"

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <vector>

namespace {

using Op = pal4::GameWorldFrameOperation;

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct FrameCall {
    Op operation;
    std::uint32_t first;
    std::uint32_t second;
    bool operator==(const FrameCall&) const = default;
};

struct Runtime final : pal4::IGameWorldFrameRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::uint32_t> word_reads;
    std::vector<std::uint32_t> byte_reads;
    std::vector<FrameCall> calls;
    std::uint32_t render_result = 1u;

    std::uint32_t ReadWord(std::uint32_t address) override {
        word_reads.push_back(address);
        return words[address];
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        byte_reads.push_back(address);
        return bytes[address];
    }
    std::uint32_t Call(Op operation, std::uint32_t first,
                       std::uint32_t second) override {
        calls.push_back({operation, first, second});
        return operation == Op::RenderWorld
                   ? render_result
                   : 0x1000u + static_cast<std::uint32_t>(operation);
    }
};

std::uint32_t Manager(Op operation) {
    return 0x1000u + static_cast<std::uint32_t>(operation);
}

std::vector<FrameCall> FullFrame() {
    constexpr auto world = 0x5000u;
    constexpr auto frame = 0x777u;
    return {
        {Op::BeginFrame, world, 0u},
        {Op::RenderWorld, world, frame},
        {Op::GetGameObjectManager, frame, 0u},
        {Op::RenderGameObjects, Manager(Op::GetGameObjectManager), frame},
        {Op::GetPlayerManager, frame, 0u},
        {Op::UpdateAndRenderPlayers, Manager(Op::GetPlayerManager), frame},
        {Op::GetNpcManager, frame, 0u},
        {Op::ProcessNpcObjects, Manager(Op::GetNpcManager), frame},
        {Op::GetMonsterManager, frame, 0u},
        {Op::ProcessMonsterObjects, Manager(Op::GetMonsterManager), frame},
        {Op::GetPetManager, 0u, 0u},
        {Op::ProcessPetObjects, Manager(Op::GetPetManager), frame},
        {Op::GetPalObjectManager, frame, 0u},
        {Op::RenderPalObjects, Manager(Op::GetPalObjectManager), frame},
        {Op::GetArenaSkillManager, frame, 0u},
        {Op::RenderArenaSkills, Manager(Op::GetArenaSkillManager), frame},
        {Op::ProcessScenePass, world, frame},
        {Op::EndFrame, world, 0u},
        {Op::GetPlayerManager, frame, 0u},
        {Op::ProcessPlayerUpdates, Manager(Op::GetPlayerManager), frame},
        {Op::GetNpcManager, frame, 0u},
        {Op::ProcessNpcAlternative, Manager(Op::GetNpcManager), frame},
        {Op::GetEffectManager, frame, 0u},
        {Op::ProcessEffects, Manager(Op::GetEffectManager), frame},
        {Op::GetPlayerManager, frame, 0u},
        {Op::ProcessPlayerRelatedData, Manager(Op::GetPlayerManager), frame},
    };
}

void FullFrameWithoutEarlyPass() {
    Runtime runtime;
    pal4::GameWorldFrameProcessor::Process(0x5000u, 0x777u, runtime);
    Check(runtime.calls == FullFrame() &&
              runtime.word_reads == std::vector<std::uint32_t>{0x51f8u} &&
              runtime.byte_reads.empty(),
          "normal frame preserves every manager call and skips null state");
}

void EarlyPlayerAndNpcPass() {
    Runtime runtime;
    runtime.words[0x51f8u] = 0x6000u;
    runtime.bytes[0x6001u] = 1u;
    runtime.words[0x6008u] = 3u;
    pal4::GameWorldFrameProcessor::Process(0x5000u, 0x777u, runtime);
    auto expected = FullFrame();
    expected.insert(expected.begin(), {
        {Op::GetPlayerManager, 0x777u, 0u},
        {Op::ProcessPlayerUpdates, Manager(Op::GetPlayerManager), 0x777u},
        {Op::GetNpcManager, 0x777u, 0u},
        {Op::ProcessNpcAlternative, Manager(Op::GetNpcManager), 0x777u},
    });
    Check(runtime.calls == expected &&
              runtime.word_reads ==
                  std::vector<std::uint32_t>{0x51f8u, 0x6008u} &&
              runtime.byte_reads == std::vector<std::uint32_t>{0x6001u},
          "active type-three state adds player and NPC pass before render");
}

void EarlyPassGates() {
    Runtime disabled;
    disabled.words[0x51f8u] = 0x6000u;
    pal4::GameWorldFrameProcessor::Process(0x5000u, 0x777u, disabled);
    Check(disabled.calls == FullFrame() &&
              disabled.word_reads ==
                  std::vector<std::uint32_t>{0x51f8u} &&
              disabled.byte_reads == std::vector<std::uint32_t>{0x6001u},
          "disabled state skips type lookup and early pass");

    Runtime wrong_type;
    wrong_type.words[0x51f8u] = 0x6000u;
    wrong_type.bytes[0x6001u] = 1u;
    wrong_type.words[0x6008u] = 2u;
    pal4::GameWorldFrameProcessor::Process(0x5000u, 0x777u, wrong_type);
    Check(wrong_type.calls == FullFrame() &&
              wrong_type.word_reads ==
                  std::vector<std::uint32_t>{0x51f8u, 0x6008u},
          "non-three state skips early pass");
}

void RenderGateAndStub() {
    Runtime runtime;
    runtime.words[0x51f8u] = 0x6000u;
    runtime.bytes[0x6001u] = 1u;
    runtime.words[0x6008u] = 3u;
    runtime.render_result = 0u;
    pal4::GameWorldFrameProcessor::Process(0x5000u, 0x777u, runtime);
    Check(runtime.calls == std::vector<FrameCall>{
              {Op::GetPlayerManager, 0x777u, 0u},
              {Op::ProcessPlayerUpdates, Manager(Op::GetPlayerManager),
               0x777u},
              {Op::GetNpcManager, 0x777u, 0u},
              {Op::ProcessNpcAlternative, Manager(Op::GetNpcManager),
               0x777u},
              {Op::BeginFrame, 0x5000u, 0u},
              {Op::RenderWorld, 0x5000u, 0x777u},
              {Op::EndFrame, 0x5000u, 0u},
          },
          "failed render still executes end hook and skips remaining managers");

    pal4::GameWorldFrameRuntimeStub stub;
    pal4::GameWorldFrameProcessor::Process(0x5000u, 0x777u, stub);
}

}  // namespace

int main() {
    try {
        FullFrameWithoutEarlyPass();
        EarlyPlayerAndNpcPass();
        EarlyPassGates();
        RenderGateAndStub();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
