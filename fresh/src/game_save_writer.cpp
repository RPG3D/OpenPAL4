#include "pal4/game_save_writer.h"

namespace pal4 {
namespace {

GameSaveHeader BuildHeader(const std::uint32_t archive,
                           IGameSaveWriterRuntime& runtime) {
    GameSaveHeader header;
    const auto active_manager = runtime.PlayerManager();
    header.player_102_active = runtime.IsPlayer102Active(active_manager) & 0xffu;
    header.game_global = runtime.ReadWord(GameSaveWriter::kGlobalHeaderWord);

    const auto first_game = runtime.GameInstance();
    const auto first_key = runtime.ReadWord(first_game + 4u);
    const auto second_game = runtime.GameInstance();
    header.game_key = first_key + runtime.ObjectInternalId(second_game);

    const auto arena = runtime.ArenaManager();
    const auto record = runtime.ReadWord(arena + 0x108u);
    if (record != 0u) {
        header.arena_record_id = runtime.ReadWord(record);
        const auto name = runtime.ReadString(record + 0x24u);
        runtime.SetArchiveText(archive, name);
    }

    for (std::uint32_t index = 0; index < 4u; ++index) {
        const auto manager = runtime.PlayerManager();
        const auto player = runtime.PlayerByIndex(manager, index);
        if (player == 0u) continue;
        header.player_models[index] = runtime.ReadWord(player + 0xb08u);
        header.player_properties[index] = runtime.PlayerProperty(
            player + 0x654u, 4u);
    }
    return header;
}

void UpdateGameDataEntries(IGameSaveWriterRuntime& runtime) {
    for (std::uint32_t index = 0; index < GameSaveWriter::kGameDataWordCount;
         ++index) {
        const auto game = runtime.GameInstance();
        const auto value = runtime.ReadWord(
            GameSaveWriter::kFirstGameDataWord + index * 4u);
        const auto entry = index < 5u ? 21u + index : 22u + index;
        runtime.SetGameDataEntry(game, entry, value);
    }
    const auto pet = runtime.PetManager();
    const auto pet_value = runtime.ReadWord(pet + 0x654u);
    const auto game = runtime.GameInstance();
    runtime.SetGameDataEntry(game, 69u, pet_value);
}

void SerializeSubsystems(const std::uint32_t archive,
                         IGameSaveWriterRuntime& runtime) {
    const auto game = runtime.GameInstance();
    runtime.SerializeGameData(game, archive);
    const auto inventory = runtime.Inventory();
    runtime.SerializeInventory(inventory, archive);
    const auto players = runtime.PlayerManager();
    runtime.SerializePlayers(players, archive);

    const auto first_arena = runtime.ArenaManager();
    const auto scene = runtime.ReadWord(first_arena + 0xe0u);
    const auto second_arena = runtime.ArenaManager();
    const auto world_path = runtime.ReadWord(second_arena + 8u);
    const auto cache = runtime.ArenaCache();
    runtime.SaveArenaCache(cache, world_path, scene, archive);

    const auto config_arena = runtime.ArenaManager();
    runtime.SerializeArenaConfig(config_arena, archive);
    const auto camera = runtime.CameraManager();
    runtime.SaveCameraConfig(camera, archive);
}

}  // namespace

std::uint8_t GameSaveWriter::SaveSlot(
    const std::int32_t slot, IGameSaveWriterRuntime& runtime) {
    const auto original_directory = runtime.CurrentDirectory(256u);
    if (!runtime.SetCurrentDirectory("save"))
        runtime.CreateDirectory("save");
    static_cast<void>(runtime.SetCurrentDirectory(original_directory));

    const auto filename = "save\\" + std::to_string(slot) + ".dat";
    const auto file = runtime.OpenWrite(filename);
    if (file == 0u) return 0u;

    const auto stream = runtime.InitializeFileStream(file);
    const auto archive = runtime.InitializeSaveArchive(stream);
    runtime.SetArchiveErrorFlag(archive, false);
    runtime.SetArchiveText(archive, "FOR_TEQUILA");
    const auto header = BuildHeader(archive, runtime);
    runtime.PrepareSaveData(archive);
    runtime.WriteHeader(archive, header);
    runtime.PlayMusic("wa11.wav", 1u);

    UpdateGameDataEntries(runtime);
    SerializeSubsystems(archive, runtime);
    const auto control = runtime.ActorControl();
    const auto result = runtime.SaveActorControlConfig(control, archive);
    runtime.CloseWrite(file);
    runtime.CaptureThumbnail(slot);
    const auto global_data = runtime.GlobalData();
    runtime.SaveDialogSettings(global_data);
    runtime.ReleaseSaveArchive(archive);
    return result;
}

}  // namespace pal4
