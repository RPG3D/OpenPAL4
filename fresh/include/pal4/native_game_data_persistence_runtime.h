#pragma once

#include "pal4/game_data_persistence.h"
#include "pal4/native_game_save_archive_buffer.h"
#include "pal4/native_game_save_archive_reader.h"

#include <cstdint>
#include <functional>

namespace pal4 {

class DialogManagerState;
class NativePalGameIVTextureStorage;

// Reads/writes the live PALGameIV image through shared GameDataPersistence.
// Game, global data, archive and pet image are borrowed and must outlive this runtime.
class NativeGameDataPersistenceRuntime final : public IGameDataPersistenceRuntime {
public:
    static constexpr std::uint32_t kArchiveHandle = 1u;

    NativeGameDataPersistenceRuntime(
        NativePalGameIVTextureStorage& game, DialogManagerState& global_data,
        NativeGameSaveArchiveBuffer& archive, std::uint32_t ui_manager,
        std::function<std::uint32_t(std::uint32_t)> timer_duration);

    NativeGameDataPersistenceRuntime(
        NativePalGameIVTextureStorage& game, DialogManagerState& global_data,
        NativeGameSaveArchiveReader& archive, std::span<std::byte> pet_image);
    [[nodiscard]] std::uint8_t Serialize();
    [[nodiscard]] bool Deserialize();

    void PrepareTarget(std::uint32_t target, std::string_view marker) override;
    [[nodiscard]] std::optional<std::string> ReadMarker(
        std::uint32_t source) override;
    void ProcessNativeRecords(std::uint32_t stream, bool reading) override;
    [[nodiscard]] std::uint32_t CreateNode(std::string_view name) override;
    [[nodiscard]] std::uint32_t ParseNode(
        std::uint32_t source, std::string_view name) override;
    [[nodiscard]] bool CheckVersionAndType(
        std::uint32_t node, std::uint32_t source) override;
    [[nodiscard]] bool HasAttribute(
        std::uint32_t node, std::string_view key) override;
    void SetInt(std::uint32_t node, std::string_view key,
                std::int32_t value) override;
    void AddInt(std::uint32_t node, std::string_view key,
                std::int32_t value) override;
    void SetString(std::uint32_t node, std::string_view key,
                   std::string_view value) override;
    void AddString(std::uint32_t node, std::string_view key,
                   std::string_view value) override;
    [[nodiscard]] std::int32_t ReadInt(
        std::uint32_t node, std::string_view key,
        std::int32_t default_value) override;
    [[nodiscard]] std::string ReadString(
        std::uint32_t node, std::string_view key,
        std::string_view default_value) override;
    [[nodiscard]] std::uint8_t Write(
        std::uint32_t node, std::uint32_t target) override;
    void DestroyNode(std::uint32_t node) override;
    void ReleaseNativeRecords(std::uint32_t stream) override;
    void ReleaseMarker() override;
    [[nodiscard]] std::uint32_t ReadDword(std::uint32_t address) override;
    void WriteDword(std::uint32_t address, std::uint32_t value) override;
    [[nodiscard]] std::string_view ReadCString(
        std::uint32_t address) override;
    [[nodiscard]] std::uint32_t ObjectInternalId(
        std::uint32_t object) override;
    [[nodiscard]] std::uint32_t UiFrameManager() override;
    [[nodiscard]] std::uint32_t TimerFrameDuration(
        std::uint32_t manager) override;
    [[nodiscard]] std::uint32_t GlobalDataSubsystem() override;
    [[nodiscard]] std::uint32_t PetManager() override;

private:
    void RequireArchive(std::uint32_t handle) const;
    void RequireNode(std::uint32_t handle) const;
    void AddAttribute(std::uint32_t node, BinaryXmlAttribute attribute);

    NativePalGameIVTextureStorage& game_;
    DialogManagerState& global_data_;
    NativeGameSaveArchiveBuffer* output_ = nullptr;
    NativeGameSaveArchiveReader* input_ = nullptr;
    std::span<std::byte> pet_image_;
    BinaryXmlConfig parsed_;
    std::string expected_root_;
    std::uint32_t ui_manager_ = 0u;
    std::function<std::uint32_t(std::uint32_t)> timer_duration_;
    BinaryXmlNode node_;
    bool node_open_{};
};

}  // namespace pal4
