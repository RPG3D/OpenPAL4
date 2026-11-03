#include "pal4/game_save_reader.h"

#include <string>

namespace pal4 {
namespace {

void AdvanceRandomState(IGameSaveReaderRuntime& runtime) {
    const auto seed = runtime.Random();
    runtime.SeedRandom(seed);
    static_cast<void>(runtime.Random());
}

}  // namespace

std::uint8_t GameSaveReader::LoadSlot(
    const std::int32_t slot, const bool quick_load,
    GameSaveHeader* const header_output,
    std::string* const marker_output,
    IGameSaveReaderRuntime& runtime) {
    const auto script = runtime.ScriptExecutor();
    runtime.CleanupScriptCache(script + 0x54u);
    const auto file = runtime.OpenRead(
        "save\\" + std::to_string(slot) + ".dat");
    if (file == 0u) return 0u;

    const auto stream = runtime.InitializeFileStream(file);
    const auto archive = runtime.InitializeLoadArchive(stream);
    runtime.SetArchiveErrorFlag(archive, false);
    const auto marker = runtime.ReadMarker(archive, quick_load);
    if (marker_output != nullptr) *marker_output = marker;
    GameSaveHeader header;
    const auto read_result = runtime.ReadHeader(archive, header);
    if (quick_load) {
        if (header_output != nullptr) *header_output = header;
        runtime.CloseRead(file);
        runtime.SetArchiveErrorFlag(archive, true);
        runtime.ReleaseLoadArchive(archive);
        return read_result;
    }

    const auto music = runtime.BackgroundMusicPlayer();
    runtime.WriteWord(music + 0x34u, 0u);
    runtime.PlayMusic("wa10.wav", 1u);
    AdvanceRandomState(runtime);
    const auto game = runtime.GameInstance();
    runtime.DeserializeGameData(game, archive);
    AdvanceRandomState(runtime);
    const auto inventory = runtime.Inventory();
    runtime.DeserializeInventory(inventory, archive);
    AdvanceRandomState(runtime);
    const auto players = runtime.PlayerManager();
    runtime.DeserializePlayers(players, archive);
    const auto cache = runtime.ArenaCache();
    runtime.LoadArenaCache(cache, archive);
    const auto mode_control = runtime.ActorControl();
    runtime.SetActorControlMode(mode_control, 1u);
    const auto arena = runtime.ArenaManager();
    runtime.DeserializeScene(arena, archive);
    AdvanceRandomState(runtime);
    const auto camera = runtime.CameraManager();
    runtime.LoadCameraConfig(camera, archive);
    AdvanceRandomState(runtime);
    const auto final_control = runtime.ActorControl();
    const auto result = runtime.LoadActorControlConfig(final_control, archive);
    runtime.CloseRead(file);
    runtime.ReleaseLoadArchive(archive);
    return result;
}

}  // namespace pal4
