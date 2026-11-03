#include "pal4/game_save_reader.h"

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IGameSaveReaderRuntime {
    std::vector<std::string> calls;
    std::vector<std::uint32_t> seeds;
    std::uint32_t file = 0x1000;
    std::uint32_t random_calls = 0;
    std::uint32_t actor_queries = 0;
    std::uint8_t header_result = 0x3c;
    std::uint8_t actor_result = 0xa7;

    void Log(const char* call) { calls.emplace_back(call); }
    std::uint32_t ScriptExecutor() override {
        Log("script executor");
        return 0x2000;
    }
    void CleanupScriptCache(std::uint32_t cache) override {
        Check(cache == 0x2054, "script cache member offset");
        Log("clear script cache");
    }
    std::uint32_t OpenRead(std::string_view path) override {
        Check(path == "save\\-7.dat", "signed save-file name");
        Log("open read");
        return file;
    }
    std::uint32_t InitializeFileStream(std::uint32_t value) override {
        Check(value == file, "file stream handle");
        Log("file stream");
        return 0x3000;
    }
    std::uint32_t InitializeLoadArchive(std::uint32_t stream) override {
        Check(stream == 0x3000, "archive stream and read mode");
        Log("load archive");
        return 0x4000;
    }
    void SetArchiveErrorFlag(std::uint32_t archive,
                             bool value) override {
        Check(archive == 0x4000, "archive error-flag object");
        Log(value ? "error flag true" : "error flag false");
    }
    std::string ReadMarker(std::uint32_t archive,
                           bool quick_load) override {
        Check(archive == 0x4000 && quick_load == expected_quick,
              "archive marker mode");
        Log("read marker");
        return "scene from save";
    }
    std::uint8_t ReadHeader(std::uint32_t archive,
                            pal4::GameSaveHeader& header) override {
        Check(archive == 0x4000 && sizeof(header) == 0x34,
              "52-byte save header");
        header.player_102_active = 1;
        header.game_key = 0x11223344;
        header.arena_record_id = 0x77;
        Log("read header");
        return header_result;
    }
    void CloseRead(std::uint32_t value) override {
        Check(value == file, "close opened save file");
        Log("close file");
    }
    void ReleaseLoadArchive(std::uint32_t archive) override {
        Check(archive == 0x4000, "release load archive");
        Log("release archive");
    }
    std::uint32_t BackgroundMusicPlayer() override {
        Log("background music");
        return 0x5000;
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        Check(address == 0x5034 && value == 0,
              "reset background music field +0x34");
        Log("reset background music");
    }
    void PlayMusic(std::string_view path,
                   std::uint32_t loop) override {
        Check(path == "wa10.wav" && loop == 1,
              "load transition music");
        Log("play music");
    }
    std::uint32_t Random() override {
        Log("random");
        return ++random_calls;
    }
    void SeedRandom(std::uint32_t value) override {
        Check(value == random_calls, "seed from immediate prior random");
        seeds.push_back(value);
        Log("seed random");
    }
    std::uint32_t GameInstance() override {
        Log("game");
        return 0x6000;
    }
    void DeserializeGameData(std::uint32_t game,
                             std::uint32_t archive) override {
        Check(game == 0x6000 && archive == 0x4000,
              "game-data load arguments");
        Log("load game data");
    }
    std::uint32_t Inventory() override {
        Log("inventory");
        return 0x7000;
    }
    void DeserializeInventory(std::uint32_t inventory,
                              std::uint32_t archive) override {
        Check(inventory == 0x7000 && archive == 0x4000,
              "inventory load arguments");
        Log("load inventory");
    }
    std::uint32_t PlayerManager() override {
        Log("player manager");
        return 0x8000;
    }
    void DeserializePlayers(std::uint32_t players,
                            std::uint32_t archive) override {
        Check(players == 0x8000 && archive == 0x4000,
              "player load arguments");
        Log("load players");
    }
    std::uint32_t ArenaCache() override {
        Log("arena cache");
        return 0x9000;
    }
    void LoadArenaCache(std::uint32_t cache,
                        std::uint32_t archive) override {
        Check(cache == 0x9000 && archive == 0x4000,
              "arena cache load arguments");
        Log("load arena cache");
    }
    std::uint32_t ActorControl() override {
        Log("actor control");
        return 0xa000 + ++actor_queries * 0x100;
    }
    void SetActorControlMode(std::uint32_t control,
                             std::uint32_t mode) override {
        Check(control == 0xa100 && mode == 1,
              "actor-control mode before scene load");
        Log("set actor mode");
    }
    std::uint32_t ArenaManager() override {
        Log("arena manager");
        return 0xb000;
    }
    void DeserializeScene(std::uint32_t arena,
                          std::uint32_t archive) override {
        Check(arena == 0xb000 && archive == 0x4000,
              "scene load arguments");
        Log("load scene");
    }
    std::uint32_t CameraManager() override {
        Log("camera manager");
        return 0xc000;
    }
    void LoadCameraConfig(std::uint32_t camera,
                          std::uint32_t archive) override {
        Check(camera == 0xc000 && archive == 0x4000,
              "camera config load arguments");
        Log("load camera");
    }
    std::uint8_t LoadActorControlConfig(
        std::uint32_t control, std::uint32_t archive) override {
        Check(control == 0xa200 && archive == 0x4000,
              "final actor-control load arguments");
        Log("load actor config");
        return actor_result;
    }
    bool expected_quick = false;
};

void InOrder(const Runtime& runtime,
             std::initializer_list<const char*> expected) {
    auto it = runtime.calls.begin();
    for (const auto* call : expected) {
        it = std::find(it, runtime.calls.end(), call);
        Check(it != runtime.calls.end(), "load stage order");
        ++it;
    }
}

void TestOpenFailure() {
    Runtime runtime;
    runtime.file = 0;
    pal4::GameSaveHeader output;
    output.game_key = 0xdeadbeef;
    std::string marker = "unchanged";
    Check(pal4::GameSaveReader::LoadSlot(-7, true, &output, &marker,
                                         runtime) == 0 &&
              runtime.calls == std::vector<std::string>{
                                   "script executor", "clear script cache",
                                   "open read"} &&
              output.game_key == 0xdeadbeef && marker == "unchanged",
          "missing save returns zero without touching outputs");
}

void TestQuickLoad() {
    Runtime runtime;
    runtime.expected_quick = true;
    pal4::GameSaveHeader output;
    std::string marker;
    Check(pal4::GameSaveReader::LoadSlot(-7, true, &output, &marker,
                                         runtime) == 0x3c &&
              output.game_key == 0x11223344 &&
              output.arena_record_id == 0x77 &&
              marker == "scene from save",
          "quick load copies 52-byte header and metadata");
    Check(runtime.calls == std::vector<std::string>{
                               "script executor", "clear script cache",
                               "open read", "file stream", "load archive",
                               "error flag false", "read marker",
                               "read header", "close file", "error flag true",
                               "release archive"} &&
              runtime.random_calls == 0,
          "quick path closes file and releases archive without game load");
    runtime = Runtime{};
    runtime.expected_quick = true;
    runtime.header_result = 0;
    Check(pal4::GameSaveReader::LoadSlot(-7, true, nullptr, nullptr,
                                         runtime) == 0 &&
              runtime.calls.back() == "release archive",
          "quick path with null outputs forwards zero header result");
}

void TestFullLoad() {
    Runtime runtime;
    runtime.header_result = 0;
    pal4::GameSaveHeader output;
    output.game_key = 0xdeadbeef;
    std::string marker;
    Check(pal4::GameSaveReader::LoadSlot(-7, false, &output, &marker,
                                         runtime) == 0xa7 &&
              marker == "scene from save" &&
              output.game_key == 0xdeadbeef,
          "full load returns final actor result and does not copy quick header");
    Check(runtime.random_calls == 10 &&
              runtime.seeds == std::vector<std::uint32_t>{1, 3, 5, 7, 9} &&
              runtime.actor_queries == 2,
          "five seed pairs and two actor-control lookups");
    InOrder(runtime, {"read header", "background music",
                      "reset background music", "play music",
                      "random", "seed random", "random", "game",
                      "load game data", "random", "seed random",
                      "random", "inventory", "load inventory",
                      "random", "seed random", "random",
                      "player manager", "load players", "arena cache",
                      "load arena cache", "actor control",
                      "set actor mode", "arena manager", "load scene",
                      "random", "seed random", "random",
                      "camera manager", "load camera", "random",
                      "seed random", "random", "actor control",
                      "load actor config", "close file", "release archive"});
    Check(std::find(runtime.calls.begin(), runtime.calls.end(),
                    "error flag true") == runtime.calls.end(),
          "full load does not set quick-branch cleanup flag");
}

}  // namespace

int main() {
    try {
        TestOpenFailure();
        TestQuickLoad();
        TestFullLoad();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
