#include "pal4/game_save_writer.h"

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Entry {
    std::uint32_t index;
    std::uint32_t value;
    bool operator==(const Entry&) const = default;
};

struct Runtime final : pal4::IGameSaveWriterRuntime {
    std::vector<std::string> calls;
    std::vector<Entry> entries;
    pal4::GameSaveHeader saved_header;
    std::uint32_t game_queries = 0;
    std::uint32_t player_manager_queries = 0;
    std::uint32_t arena_queries = 0;
    std::uint32_t opened_file = 0x1010;
    std::uint32_t arena_record = 0x5000;
    bool save_directory_exists = true;
    bool players_present = true;
    std::uint8_t actor_result = 0xa5;

    void Log(const char* call) { calls.emplace_back(call); }
    std::string CurrentDirectory(std::uint32_t capacity) override {
        Check(capacity == 256, "original directory buffer capacity");
        Log("get directory");
        return "C:\\PAL4";
    }
    bool SetCurrentDirectory(std::string_view path) override {
        if (path == "save") {
            Log("set save directory");
            return save_directory_exists;
        }
        Check(path == "C:\\PAL4", "restore original directory");
        Log("restore directory");
        return true;
    }
    void CreateDirectory(std::string_view path) override {
        Check(path == "save", "create save directory");
        Log("create directory");
    }
    std::uint32_t OpenWrite(std::string_view path) override {
        Check(path == "save\\-7.dat", "signed slot save filename");
        Log("open write");
        return opened_file;
    }
    std::uint32_t InitializeFileStream(std::uint32_t file) override {
        Check(file == opened_file, "file stream handle");
        Log("file stream");
        return 0x2020;
    }
    std::uint32_t InitializeSaveArchive(std::uint32_t stream) override {
        Check(stream == 0x2020, "save archive stream");
        Log("archive");
        return 0x3030;
    }
    void SetArchiveErrorFlag(std::uint32_t archive,
                             bool value) override {
        Check(archive == 0x3030 && !value, "archive error flag");
        Log("error flag");
    }
    void SetArchiveText(std::uint32_t archive,
                        std::string_view text) override {
        Check(archive == 0x3030, "archive text object");
        if (text == "FOR_TEQUILA") Log("version text");
        else {
            Check(text == "scene-2", "arena scene name");
            Log("scene text");
        }
    }
    std::uint32_t PlayerManager() override {
        Log("player manager");
        ++player_manager_queries;
        return 0xb000;
    }
    std::uint32_t IsPlayer102Active(std::uint32_t manager) override {
        Check(manager == 0xb000, "active player manager");
        Log("player 102 flag");
        return 0x101;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        if (address >= 0x90dec0 && address <= 0x90df7c &&
            (address & 3u) == 0u) {
            Log("global word");
            return address;
        }
        if (address == 0x2004) {
            Log("game key word");
            return 0x40;
        }
        if (address == 0x4108) {
            Log("arena record pointer");
            return arena_record;
        }
        if (address == 0x5000) {
            Log("arena record id");
            return 0x77;
        }
        if (address >= 0x6b08 && address <= 0x9b08 &&
            (address - 0x6b08) % 0x1000 == 0) {
            Log("player model");
            return 0x100 + (address - 0x6b08) / 0x1000;
        }
        if (address == 0x40e0) {
            Log("arena scene");
            return 0x9000;
        }
        if (address == 0x4008) {
            Log("arena world path");
            return 0x8000;
        }
        if (address == 0xa654) {
            Log("pet value");
            return 0x55;
        }
        throw std::runtime_error("unexpected save word address");
    }
    std::string ReadString(std::uint32_t address) override {
        Check(address == 0x5024, "arena record text offset");
        Log("scene name");
        return "scene-2";
    }
    std::uint32_t GameInstance() override {
        Log("game");
        ++game_queries;
        return 0x2000;
    }
    std::uint32_t ObjectInternalId(std::uint32_t object) override {
        Check(object == 0x2000, "game internal ID object");
        Log("game internal ID");
        return 0x30;
    }
    std::uint32_t ArenaManager() override {
        Log("arena manager");
        ++arena_queries;
        return 0x4000;
    }
    std::uint32_t PlayerByIndex(std::uint32_t manager,
                                std::uint32_t index) override {
        Check(manager == 0xb000 && index < 4, "indexed player lookup");
        Log("player by index");
        return players_present ? 0x6000 + index * 0x1000 : 0;
    }
    std::uint32_t PlayerProperty(std::uint32_t property,
                                 std::uint32_t category) override {
        Check(property >= 0x6654 && property <= 0x9654 &&
                  (property - 0x6654) % 0x1000 == 0 && category == 4,
              "player property virtual slot and category");
        Log("player property");
        return 0xabc000 + (property - 0x6654) / 0x1000;
    }
    void PrepareSaveData(std::uint32_t archive) override {
        Check(archive == 0x3030, "prepare save archive");
        Log("prepare data");
    }
    void WriteHeader(std::uint32_t archive,
                     const pal4::GameSaveHeader& header) override {
        Check(archive == 0x3030, "header archive");
        saved_header = header;
        Log("write header");
    }
    void PlayMusic(std::string_view path,
                   std::uint32_t loop) override {
        Check(path == "wa11.wav" && loop == 1, "post-header music");
        Log("play music");
    }
    void SetGameDataEntry(std::uint32_t game, std::uint32_t index,
                          std::uint32_t value) override {
        Check(game == 0x2000, "game-data owner");
        entries.push_back({index, value});
        Log("set game data");
    }
    void SerializeGameData(std::uint32_t game,
                           std::uint32_t archive) override {
        Check(game == 0x2000 && archive == 0x3030,
              "game-data serializer arguments");
        Log("serialize game data");
    }
    std::uint32_t Inventory() override {
        Log("inventory");
        return 0xc000;
    }
    void SerializeInventory(std::uint32_t inventory,
                            std::uint32_t archive) override {
        Check(inventory == 0xc000 && archive == 0x3030,
              "inventory serializer arguments");
        Log("serialize inventory");
    }
    void SerializePlayers(std::uint32_t manager,
                          std::uint32_t archive) override {
        Check(manager == 0xb000 && archive == 0x3030,
              "player serializer arguments");
        Log("serialize players");
    }
    std::uint32_t ArenaCache() override {
        Log("arena cache");
        return 0xd000;
    }
    void SaveArenaCache(std::uint32_t cache, std::uint32_t world_path,
                        std::uint32_t scene,
                        std::uint32_t archive) override {
        Check(cache == 0xd000 && world_path == 0x8000 &&
                  scene == 0x9000 && archive == 0x3030,
              "arena cache save arguments");
        Log("save arena cache");
    }
    void SerializeArenaConfig(std::uint32_t manager,
                              std::uint32_t archive) override {
        Check(manager == 0x4000 && archive == 0x3030,
              "arena config serializer arguments");
        Log("serialize arena config");
    }
    std::uint32_t CameraManager() override {
        Log("camera manager");
        return 0xe000;
    }
    void SaveCameraConfig(std::uint32_t camera,
                          std::uint32_t archive) override {
        Check(camera == 0xe000 && archive == 0x3030,
              "camera config serializer arguments");
        Log("save camera config");
    }
    std::uint32_t ActorControl() override {
        Log("actor control");
        return 0xf000;
    }
    std::uint8_t SaveActorControlConfig(std::uint32_t control,
                                        std::uint32_t archive) override {
        Check(control == 0xf000 && archive == 0x3030,
              "actor config serializer arguments");
        Log("save actor config");
        return actor_result;
    }
    void CloseWrite(std::uint32_t file) override {
        Check(file == opened_file, "close opened save file");
        Log("close file");
    }
    void CaptureThumbnail(std::int32_t slot) override {
        Check(slot == -7, "thumbnail uses signed save slot");
        Log("thumbnail");
    }
    std::uint32_t GlobalData() override {
        Log("global data");
        return 0x1111;
    }
    void SaveDialogSettings(std::uint32_t global_data) override {
        Check(global_data == 0x1111, "dialog settings owner");
        Log("save settings");
    }
    void ReleaseSaveArchive(std::uint32_t archive) override {
        Check(archive == 0x3030, "release save archive");
        Log("release archive");
    }
    std::uint32_t PetManager() override {
        Log("pet manager");
        return 0xa000;
    }
};

void InOrder(const Runtime& runtime,
             std::initializer_list<const char*> expected) {
    auto it = runtime.calls.begin();
    for (const auto* call : expected) {
        it = std::find(it, runtime.calls.end(), call);
        Check(it != runtime.calls.end(), "save stage order");
        ++it;
    }
}

void TestSuccessfulSave() {
    Runtime runtime;
    Check(pal4::GameSaveWriter::SaveSlot(-7, runtime) == 0xa5,
          "save returns low byte from actor config writer");
    Check(runtime.saved_header.player_102_active == 1 &&
              runtime.saved_header.game_global == 0x90dec0 &&
              runtime.saved_header.game_key == 0x70 &&
              runtime.saved_header.arena_record_id == 0x77 &&
              runtime.saved_header.version == 1,
          "52-byte header fixed fields");
    for (std::uint32_t i = 0; i < 4; ++i)
        Check(runtime.saved_header.player_models[i] == 0x100 + i &&
                  runtime.saved_header.player_properties[i] == 0xabc000 + i,
              "all four player model and property fields");
    Check(runtime.entries.size() == 48, "47 global entries and pet entry");
    for (std::uint32_t i = 0; i < 47; ++i) {
        const auto slot = i < 5 ? 21 + i : 22 + i;
        Check(runtime.entries[i] == Entry{slot, 0x90dec4 + i * 4},
              "exact ordered game-data slot and source address");
    }
    Check(runtime.entries.back() == Entry{69, 0x55},
          "pet value occupies game-data slot 69");
    Check(runtime.game_queries == 51 &&
              runtime.player_manager_queries == 6 &&
              runtime.arena_queries == 4,
          "native singletons reacquired at every original call site");
    InOrder(runtime, {"get directory", "set save directory",
                      "restore directory", "open write", "file stream",
                      "archive", "error flag", "version text",
                      "player 102 flag", "game key word",
                      "game internal ID", "arena record pointer",
                      "arena record id", "scene text", "player by index",
                      "prepare data", "write header", "play music",
                      "set game data", "pet manager",
                      "serialize game data", "serialize inventory",
                      "serialize players", "save arena cache",
                      "serialize arena config", "save camera config",
                      "save actor config", "close file", "thumbnail",
                      "global data", "save settings", "release archive"});
}

void TestOpenFailureAndDirectoryCreation() {
    Runtime runtime;
    runtime.save_directory_exists = false;
    runtime.opened_file = 0;
    Check(pal4::GameSaveWriter::SaveSlot(-7, runtime) == 0 &&
              runtime.calls == std::vector<std::string>{
                                   "get directory", "set save directory",
                                   "create directory", "restore directory",
                                   "open write"},
          "open failure returns zero after directory repair");
}

void TestAbsentArenaAndPlayers() {
    Runtime runtime;
    runtime.arena_record = 0;
    runtime.players_present = false;
    runtime.actor_result = 0;
    Check(pal4::GameSaveWriter::SaveSlot(-7, runtime) == 0 &&
              runtime.saved_header.arena_record_id == 0xffffffffu,
          "missing arena record uses minus-one sentinel");
    for (std::uint32_t i = 0; i < 4; ++i)
        Check(runtime.saved_header.player_models[i] == 0 &&
                  runtime.saved_header.player_properties[i] == 0,
              "missing players leave both header arrays zero");
    Check(std::find(runtime.calls.begin(), runtime.calls.end(),
                    "scene text") == runtime.calls.end() &&
              std::find(runtime.calls.begin(), runtime.calls.end(),
                        "player property") == runtime.calls.end(),
          "absent native records skip optional reads");
}

}  // namespace

int main() {
    try {
        TestSuccessfulSave();
        TestOpenFailureAndDirectoryCreation();
        TestAbsentArenaAndPlayers();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
