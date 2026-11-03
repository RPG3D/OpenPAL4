#include "pal4/game_world_update_processor.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace {

using Op = pal4::GameWorldUpdateOperation;

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct UpdateCall {
    Op operation;
    std::uint32_t object = 0u;
    float delta = 0.0f;
    std::uint32_t argument = 0u;
    bool operator==(const UpdateCall&) const = default;
};

std::uint32_t Handle(Op operation) {
    return 0x1000u + static_cast<std::uint32_t>(operation);
}

struct Runtime final : pal4::IGameWorldUpdateRuntime {
    std::vector<UpdateCall> calls;
    std::vector<std::uint32_t> contexts;
    std::vector<std::uint32_t> reads;

    std::uint32_t ReadWord(std::uint32_t address) override {
        reads.push_back(address);
        const auto index = reads.size() - 1u;
        return index < contexts.size() ? contexts[index] : 0u;
    }
    std::uint32_t Call(Op operation, std::uint32_t object,
                       float delta, std::uint32_t argument) override {
        calls.push_back({operation, object, delta, argument});
        return Handle(operation);
    }
};

std::vector<UpdateCall> CommonPrefix() {
    return {
        {Op::UpdateRenderParameters, 0u, 0.25f},
        {Op::UpdateClumpMatrices, 0x5140u, 0.25f},
        {Op::GetTemperatureManager, 0u, 0.25f},
        {Op::UpdateTemperature, Handle(Op::GetTemperatureManager)},
    };
}

std::vector<UpdateCall> CommonTail() {
    return {
        {Op::AdvanceFades, 0x5000u, 0.25f},
        {Op::AdvanceFadeIn, 0x5000u, 0.25f},
        {Op::AdvanceSecondaryFade, 0x5000u, 0.25f},
        {Op::NoOpWorld, 0x5000u, 0.25f},
        {Op::NoOpWorld, 0x5000u, 0.25f},
        {Op::GetBackgroundMusicPlayer},
        {Op::NoOpBackgroundMusic, Handle(Op::GetBackgroundMusicPlayer)},
    };
}

void FullUpdate() {
    Runtime runtime;
    runtime.contexts = {1u, 2u, 3u, 4u, 5u, 6u};
    const auto result = pal4::GameWorldUpdateProcessor::Update(
        0x5000u, 0.25f, 0x55u, 0u, runtime);
    auto expected = CommonPrefix();
    const std::vector<UpdateCall> body = {
        {Op::GetDialogManager, 0u, 0.25f, 1u},
        {Op::UpdateDialogs, Handle(Op::GetDialogManager), 0.25f, 1u},
        {Op::GetPlayerManager, 0u, 0.25f, 2u},
        {Op::ProcessPlayerElements, Handle(Op::GetPlayerManager), 0.25f, 2u},
        {Op::GetNpcManager, 0u, 0.25f, 3u},
        {Op::ProcessNpcObjects, Handle(Op::GetNpcManager), 0.25f, 3u},
        {Op::GetMonsterManager, 0u, 0.25f, 4u},
        {Op::ProcessMonsterObjects, Handle(Op::GetMonsterManager), 0.25f,
         4u},
        {Op::GetPetManager},
        {Op::ProcessPetObjects, Handle(Op::GetPetManager), 0.25f, 5u},
        {Op::GetObjectManager, 0u, 0.25f, 6u},
        {Op::ProcessObjects, Handle(Op::GetObjectManager), 0.25f, 6u},
        {Op::GetArenaManager, 0u, 0.25f},
        {Op::UpdateArenaDialogs, Handle(Op::GetArenaManager)},
        {Op::GetEffectManager, 0u, 0.25f},
        {Op::UpdateEffects, Handle(Op::GetEffectManager)},
        {Op::SyncWeatherCamera, 0x5000u, 0.25f, 0x55u},
        {Op::UpdateArenaCache, 0x51c0u, 0.25f},
    };
    expected.insert(expected.end(), body.begin(), body.end());
    const auto tail = CommonTail();
    expected.insert(expected.end(), tail.begin(), tail.end());
    Check(result == Handle(Op::GetBackgroundMusicPlayer) &&
              runtime.calls == expected &&
              runtime.reads == std::vector<std::uint32_t>(6u, 0x5010u),
          "full world update preserves manager order, repeated context reads, "
          "mode, delta and BGM return");
}

void SkipWorldObjects() {
    Runtime runtime;
    const auto result = pal4::GameWorldUpdateProcessor::Update(
        0x5000u, 0.25f, 0x55u, 1u, runtime);
    auto expected = CommonPrefix();
    const auto tail = CommonTail();
    expected.insert(expected.end(), tail.begin(), tail.end());
    Check(result == Handle(Op::GetBackgroundMusicPlayer) &&
              runtime.calls == expected && runtime.reads.empty(),
          "skip flag bypasses object managers but retains shared update tail");

    pal4::GameWorldUpdateRuntimeStub stub;
    Check(pal4::GameWorldUpdateProcessor::Update(
              0x5000u, 0.25f, 0x55u, 1u, stub) == 0u,
          "native-service stub links and returns a stable null music handle");
}

}  // namespace

int main() {
    try {
        FullUpdate();
        SkipWorldObjects();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
