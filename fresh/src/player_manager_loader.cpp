#include "pal4/player_manager_loader.h"

namespace pal4 {

std::uint32_t PlayerManagerLoadRuntimeStub::Log(std::uint32_t) { return 0u; }
void PlayerManagerLoadRuntimeStub::ProcessScripts() {}
std::uint32_t PlayerManagerLoadRuntimeStub::GameDB() { return 0u; }
std::uint32_t PlayerManagerLoadRuntimeStub::FindRole(
    std::uint32_t, std::uint32_t) { return 0u; }
void PlayerManagerLoadRuntimeStub::CopySlotData(
    std::uint32_t, std::uint32_t) {}
std::uint32_t PlayerManagerLoadRuntimeStub::ReadDword(
    std::uint32_t) { return 0u; }
void PlayerManagerLoadRuntimeStub::CallLoadActor(
    std::uint32_t, std::uint32_t, std::uint32_t,
    std::uint32_t, std::uint32_t) {}
void PlayerManagerLoadRuntimeStub::CallVectorMethod(
    std::uint32_t, std::uint32_t, PlayerManagerVector&,
    std::uint32_t) {}
void PlayerManagerLoadRuntimeStub::SetStateFlag(
    std::uint32_t, std::uint32_t) {}
void PlayerManagerLoadRuntimeStub::CallScriptIndex(
    std::uint32_t, std::uint32_t, std::uint32_t) {}
void PlayerManagerLoadRuntimeStub::ProcessLoadedData(std::uint32_t) {}
void PlayerManagerLoadRuntimeStub::CleanupScripts() {}

std::uint32_t PlayerManagerLoader::Load(
    std::uint32_t manager, IPlayerManagerLoadRuntime& runtime) {
    static_cast<void>(runtime.Log(0x2bu));
    runtime.ProcessScripts();
    PlayerManagerVector first_vector{};
    PlayerManagerVector second_vector{};
    for (std::uint32_t index = 0u; index < kSlotCount; ++index) {
        const auto slot = manager + index * kSlotBytes;
        const auto game_db = runtime.GameDB();
        const auto record = runtime.FindRole(game_db + 0x20u, index);
        if (record == 0u)
            continue;
        runtime.CopySlotData(slot, record);
        auto model = runtime.ReadDword(slot + kModelOffset);
        if (model == 0u)
            model = kEmptyString;
        runtime.CallLoadActor(slot, 0x30u, kNames[index], model, 1u);
        runtime.CallVectorMethod(slot, 0x4u, first_vector, 0u);
        runtime.CallVectorMethod(slot, 0x8u, second_vector, 0u);
        runtime.SetStateFlag(slot, 0u);
        runtime.CallScriptIndex(slot + kScriptOffset, 0x18u, index);
    }
    runtime.ProcessLoadedData(manager);
    runtime.CleanupScripts();
    return runtime.Log(0x49u);
}

}  // namespace pal4
