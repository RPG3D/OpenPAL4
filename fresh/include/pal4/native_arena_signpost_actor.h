#pragma once
#include "pal4/game_resource_manager.h"
#include <filesystem>
#include <memory>
#include <cstdint>
struct RpClump;
namespace pal4 {
class NativeBaseObjectMemory;
class NativePalGameIVTextureStorage;
class NativePkFileManagerRuntime;
class IArenaActorRuntime;
// Owns the original492-byte ArenaActorResource and its M001 model. Borrowed
// allocator/game/package services and RenderWare must outlive this owner.
class NativeArenaSignpostActor final {
public:
    NativeArenaSignpostActor(NativeBaseObjectMemory&, NativePalGameIVTextureStorage&,
        NativePkFileManagerRuntime&, const std::filesystem::path&);
    ~NativeArenaSignpostActor();
    NativeArenaSignpostActor(const NativeArenaSignpostActor&)=delete;
    NativeArenaSignpostActor& operator=(const NativeArenaSignpostActor&)=delete;
    void Load(std::uint32_t serial);
    void SetPosition(const ResourceVec3&);
    void SetYaw(float);
    void Attach(std::uint32_t arena);
    void Detach(std::uint32_t arena);
    void Close();
    std::uint32_t Address() const noexcept;
    RpClump* Clump() const noexcept;
    const GameResourceManager& State() const;
    // Borrowed original cleanup callback surface for its containing manager.
    IArenaActorRuntime& Runtime() noexcept;
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};
}
