#pragma once

#include "pal4/game_save_reader.h"
#include "pal4/native_player_manager_load_runtime.h"
#include <filesystem>
#include <span>

namespace pal4 {
class NativePalGameIVTextureStorage;
class DialogManagerState;
class NativeInventorySession;
class NativeArenaCacheOwner;
class NativeCameraManagerOwner;
class INativeArenaSceneLoadServices;
class NativeGameSaveArchiveReader;

class INativeGameSaveLoadHost {
public:
    virtual ~INativeGameSaveLoadHost() = default;
    virtual void CleanupScriptCache() = 0;
    virtual void ResetBackgroundMusic() = 0;
    virtual void PlayMusic(std::string_view path, std::uint32_t loop) = 0;
    virtual void SetActorControlMode(std::uint32_t mode) = 0;
    // Called after ARENA and CAM_MGR. Resolve current scene owners here:
    // loading ARENA can destroy the previous movement/floor services.
    [[nodiscard]] virtual std::uint8_t LoadActorControlConfig(
        NativeGameSaveArchiveReader& archive) = 0;
};

// Borrowed persistent owners and required game effects. No implicit host stubs.
struct NativeGameSaveLoadServices {
    NativePalGameIVTextureStorage& game;
    DialogManagerState& globals;
    std::span<std::byte> pet_image;
    NativeInventorySession& inventory;
    PlayerManagerObjectState& players;
    NativeBaseObjectMemory& memory;
    NativePlayerSlotCopyRuntime& player_text;
    NativePlayerManagerLoadServices player_services;
    NativeArenaCacheOwner& cache;
    std::uint32_t arena;
    INativeArenaSceneLoadServices& scene;
    NativeCameraManagerOwner& cameras;
    INativeGameSaveLoadHost& host;
};

class NativeGameSaveLoad final {
public:
    // Reuses IDA5CCAE0 sequencing with one RAII archive from byte zero.
    // Section rejection throws; already applied game effects are not rolled back.
    [[nodiscard]] static std::uint8_t LoadSlot(
        const std::filesystem::path& root, std::int32_t slot, bool quick,
        GameSaveHeader* header, std::string* marker,
        NativeGameSaveLoadServices services);
};
}  // namespace pal4
