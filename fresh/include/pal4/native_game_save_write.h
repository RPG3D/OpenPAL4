#pragma once
#include "pal4/native_arena_cache_save_section.h"
#include <filesystem>
#include <span>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace pal4 {
class NativePalGameIVTextureStorage;
class DialogManagerState;
class NativeInventorySession;
struct PlayerManagerObjectState;
struct PalActorControlState;
class NativeArenaCacheOwner;
class NativeCameraManagerOwner;

// Live scene/UI effects stay with their creating owners. No default callbacks:
// a caller must bind every effect before a confirmed save can succeed.
class INativeGameSaveWriteHost {
public:
    virtual ~INativeGameSaveWriteHost()=default;
    virtual void PlaySaveSound(std::string_view,std::uint32_t)=0;
    [[nodiscard]] virtual NativeArenaCacheEntry CaptureCurrentArena()=0;
    [[nodiscard]] virtual std::uint32_t TimerFrameDuration(std::uint32_t manager)=0;
    virtual void CaptureThumbnail(std::int32_t slot)=0;
    virtual void SaveDialogSettings()=0;
};
struct NativeGameSaveWriteServices {
    NativePalGameIVTextureStorage& game;
    DialogManagerState& globals;
    std::span<std::byte> pet_image;
    NativeInventorySession& inventory;
    PlayerManagerObjectState& players;
    NativeArenaCacheOwner& cache;
    std::uint32_t arena;
    NativeCameraManagerOwner& cameras;
    const PalActorControlState& control;
    std::uint32_t ui_manager;
    INativeGameSaveWriteHost& host;
};
class NativeGameSaveWrite final {
public:
    // Reuses5CBEA0 and native section serializers. Open failure returns0;
    // rejected sections/failed writes throw; final actor status is preserved.
    // Earlier game effects and an opened/truncated file are not rolled back.
    [[nodiscard]] static std::uint8_t SaveSlot(const std::filesystem::path& root,
        std::int32_t slot,NativeGameSaveWriteServices services);
};
} // namespace pal4
