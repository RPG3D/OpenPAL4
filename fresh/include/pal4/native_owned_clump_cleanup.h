#pragma once
#include "pal4/game_resource_manager.h"
namespace pal4 {
class NativeBaseObjectMemory;
class NativePalGameIVTextureStorage;
class NativeGobClumpAsset;
// Shared cleanup boundary for native GOB and ArenaActor resources. Caller
// preflights empty shadow/camera/Im3D fields before invoking CleanupClump.
class NativeOwnedClumpCleanup final : public GameResourceManagerRuntimeStub {
public:
    NativeOwnedClumpCleanup(NativeBaseObjectMemory&, NativePalGameIVTextureStorage&, NativeGobClumpAsset&);
    std::uint32_t Allocate(std::uint32_t) override;
    std::uint32_t Free(std::uint32_t) override;
    std::string ReadString(std::uint32_t) override;
    void StoreString(std::uint32_t, std::string_view) override;
    void CleanupCamera() override;
    std::uint32_t ClumpWorld(std::uint32_t) override;
    void RemoveClump(std::uint32_t, std::uint32_t) override;
    void DestroyClump(std::uint32_t) override;
    void RemoveTextureByName(std::uint32_t, std::string_view) override;
private:
    void Require(std::uint32_t);
    NativeBaseObjectMemory& memory_;
    NativePalGameIVTextureStorage& game_;
    NativeGobClumpAsset& model_;
};
}
