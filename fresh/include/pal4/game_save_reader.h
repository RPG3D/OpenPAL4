#pragma once

#include "pal4/game_save_writer.h"

#include <cstdint>
#include <string>
#include <string_view>

namespace pal4 {

class IGameSaveReaderRuntime {
public:
    virtual ~IGameSaveReaderRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ScriptExecutor() = 0;
    virtual void CleanupScriptCache(std::uint32_t cache) = 0;
    [[nodiscard]] virtual std::uint32_t OpenRead(std::string_view path) = 0;
    [[nodiscard]] virtual std::uint32_t InitializeFileStream(
        std::uint32_t file) = 0;
    [[nodiscard]] virtual std::uint32_t InitializeLoadArchive(
        std::uint32_t stream) = 0;
    virtual void SetArchiveErrorFlag(std::uint32_t archive,
                                     bool value) = 0;
    [[nodiscard]] virtual std::string ReadMarker(
        std::uint32_t archive, bool quick_load) = 0;
    [[nodiscard]] virtual std::uint8_t ReadHeader(
        std::uint32_t archive, GameSaveHeader& header) = 0;
    virtual void CloseRead(std::uint32_t file) = 0;
    virtual void ReleaseLoadArchive(std::uint32_t archive) = 0;
    [[nodiscard]] virtual std::uint32_t BackgroundMusicPlayer() = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    virtual void PlayMusic(std::string_view path,
                           std::uint32_t loop) = 0;
    [[nodiscard]] virtual std::uint32_t Random() = 0;
    virtual void SeedRandom(std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t GameInstance() = 0;
    virtual void DeserializeGameData(std::uint32_t game,
                                     std::uint32_t archive) = 0;
    [[nodiscard]] virtual std::uint32_t Inventory() = 0;
    virtual void DeserializeInventory(std::uint32_t inventory,
                                      std::uint32_t archive) = 0;
    [[nodiscard]] virtual std::uint32_t PlayerManager() = 0;
    virtual void DeserializePlayers(std::uint32_t players,
                                    std::uint32_t archive) = 0;
    [[nodiscard]] virtual std::uint32_t ArenaCache() = 0;
    virtual void LoadArenaCache(std::uint32_t cache,
                                std::uint32_t archive) = 0;
    [[nodiscard]] virtual std::uint32_t ActorControl() = 0;
    virtual void SetActorControlMode(std::uint32_t control,
                                     std::uint32_t mode) = 0;
    [[nodiscard]] virtual std::uint32_t ArenaManager() = 0;
    virtual void DeserializeScene(std::uint32_t arena,
                                  std::uint32_t archive) = 0;
    [[nodiscard]] virtual std::uint32_t CameraManager() = 0;
    virtual void LoadCameraConfig(std::uint32_t camera,
                                  std::uint32_t archive) = 0;
    [[nodiscard]] virtual std::uint8_t LoadActorControlConfig(
        std::uint32_t control, std::uint32_t archive) = 0;
};

class GameSaveReader final {
public:
    // IDA 0x5ccae0.
    [[nodiscard]] static std::uint8_t LoadSlot(
        std::int32_t slot, bool quick_load,
        GameSaveHeader* header_output,
        std::string* marker_output,
        IGameSaveReaderRuntime& runtime);
};

}  // namespace pal4
