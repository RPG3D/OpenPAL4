#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <string_view>

namespace pal4 {

struct GameSaveHeader {
    std::uint32_t player_102_active = 0;
    std::array<std::uint32_t, 4> player_models{};
    std::array<std::uint32_t, 4> player_properties{};
    std::uint32_t game_global = 0;
    std::uint32_t game_key = 0;
    std::uint32_t arena_record_id = 0xffffffffu;
    std::uint32_t version = 1;
};

static_assert(sizeof(GameSaveHeader) == 0x34);

class IGameSaveWriterRuntime {
public:
    virtual ~IGameSaveWriterRuntime() = default;
    [[nodiscard]] virtual std::string CurrentDirectory(
        std::uint32_t capacity) = 0;
    [[nodiscard]] virtual bool SetCurrentDirectory(std::string_view path) = 0;
    virtual void CreateDirectory(std::string_view path) = 0;
    [[nodiscard]] virtual std::uint32_t OpenWrite(std::string_view path) = 0;
    [[nodiscard]] virtual std::uint32_t InitializeFileStream(
        std::uint32_t file) = 0;
    [[nodiscard]] virtual std::uint32_t InitializeSaveArchive(
        std::uint32_t stream) = 0;
    virtual void SetArchiveErrorFlag(std::uint32_t archive,
                                     bool value) = 0;
    virtual void SetArchiveText(std::uint32_t archive,
                                std::string_view text) = 0;
    [[nodiscard]] virtual std::uint32_t PlayerManager() = 0;
    [[nodiscard]] virtual std::uint32_t IsPlayer102Active(
        std::uint32_t manager) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::string ReadString(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t GameInstance() = 0;
    [[nodiscard]] virtual std::uint32_t ObjectInternalId(
        std::uint32_t object) = 0;
    [[nodiscard]] virtual std::uint32_t ArenaManager() = 0;
    [[nodiscard]] virtual std::uint32_t PlayerByIndex(
        std::uint32_t manager, std::uint32_t index) = 0;
    [[nodiscard]] virtual std::uint32_t PlayerProperty(
        std::uint32_t property, std::uint32_t category) = 0;
    virtual void PrepareSaveData(std::uint32_t archive) = 0;
    virtual void WriteHeader(std::uint32_t archive,
                             const GameSaveHeader& header) = 0;
    virtual void PlayMusic(std::string_view path,
                           std::uint32_t loop) = 0;
    virtual void SetGameDataEntry(std::uint32_t game,
                                  std::uint32_t index,
                                  std::uint32_t value) = 0;
    virtual void SerializeGameData(std::uint32_t game,
                                   std::uint32_t archive) = 0;
    [[nodiscard]] virtual std::uint32_t Inventory() = 0;
    virtual void SerializeInventory(std::uint32_t inventory,
                                    std::uint32_t archive) = 0;
    virtual void SerializePlayers(std::uint32_t manager,
                                  std::uint32_t archive) = 0;
    [[nodiscard]] virtual std::uint32_t ArenaCache() = 0;
    virtual void SaveArenaCache(std::uint32_t cache,
                                std::uint32_t world_path,
                                std::uint32_t scene,
                                std::uint32_t archive) = 0;
    virtual void SerializeArenaConfig(std::uint32_t manager,
                                      std::uint32_t archive) = 0;
    [[nodiscard]] virtual std::uint32_t CameraManager() = 0;
    virtual void SaveCameraConfig(std::uint32_t camera,
                                  std::uint32_t archive) = 0;
    [[nodiscard]] virtual std::uint32_t ActorControl() = 0;
    [[nodiscard]] virtual std::uint8_t SaveActorControlConfig(
        std::uint32_t control, std::uint32_t archive) = 0;
    virtual void CloseWrite(std::uint32_t file) = 0;
    virtual void CaptureThumbnail(std::int32_t slot) = 0;
    [[nodiscard]] virtual std::uint32_t GlobalData() = 0;
    virtual void SaveDialogSettings(std::uint32_t global_data) = 0;
    virtual void ReleaseSaveArchive(std::uint32_t archive) = 0;
    [[nodiscard]] virtual std::uint32_t PetManager() = 0;
};

class GameSaveWriter final {
public:
    static constexpr std::uint32_t kGlobalHeaderWord = 0x90dec0u;
    static constexpr std::uint32_t kFirstGameDataWord = 0x90dec4u;
    static constexpr std::uint32_t kGameDataWordCount = 47u;

    // IDA 0x5cbea0.
    [[nodiscard]] static std::uint8_t SaveSlot(
        std::int32_t slot, IGameSaveWriterRuntime& runtime);
};

}  // namespace pal4
