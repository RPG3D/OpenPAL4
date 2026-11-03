#pragma once

#include "pal4/config_composite.h"
#include "pal4/native_base_object_memory.h"
#include "pal4/native_clump_collision_query.h"
#include "pal4/native_movement_secondary_helpers.h"

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>

struct RpClump;
struct RpAtomic;

namespace pal4 {
class NativePackageArchive;

// Owns scene-package intri and scenedata floor/wall movement clumps.
class NativeSceneMovementAsset final : private IConfigCompositeRuntime {
public:
    NativeSceneMovementAsset() = default;
    NativeSceneMovementAsset(const NativeSceneMovementAsset&) = delete;
    NativeSceneMovementAsset& operator=(const NativeSceneMovementAsset&) = delete;
    ~NativeSceneMovementAsset();

    [[nodiscard]] bool Load(const std::filesystem::path& game_root,
                            std::string_view world, std::string_view scene);
    [[nodiscard]] bool LoadIntri(NativePackageArchive& archive,
                                 std::string_view member, std::uint32_t game);
    [[nodiscard]] bool RegisterObjectClump(RpClump* clump,
                                           std::uint32_t property_callback,
                                           std::uint32_t mode = 1u,
                                           std::uint32_t flag1 = 0u,
                                           std::uint32_t flag2 = 0u);
    [[nodiscard]] bool UnregisterObjectClump(RpClump* clump);
    // Reapply original Clump_SetProperties without acquiring another owner.
    [[nodiscard]] bool ReapplyObjectClumpProperties(
        RpClump* clump, std::uint32_t property_callback,
        std::uint32_t mode = 1u, std::uint32_t flag1 = 0u,
        std::uint32_t flag2 = 0u);
    [[nodiscard]] bool IsObjectClumpRegistered(RpClump* clump) const noexcept {
        return registered_object_clumps_.contains(clump);
    }
    [[nodiscard]] bool RegisterEventClump(RpClump* clump,
                                          std::uint32_t resource,
                                          bool secondary,
                                          std::uint32_t property = 0u);
    [[nodiscard]] bool UnregisterEventClump(RpClump* clump, bool secondary);
    [[nodiscard]] bool IsEventResourceRegistered(
        RpClump* clump, std::uint32_t resource) const noexcept;
    void Unload() noexcept;
    // IDA5FEFF0: retire owned resources/records, retaining container sentinels
    // and borrower identities until their scene-leave callbacks run.
    void ReleaseComponentResources(std::uint32_t component);
    [[nodiscard]] std::size_t SceneBorrowerCount() const noexcept {
        return registered_object_clumps_.size()+registered_event_primary_.size()+
            registered_event_secondary_.size()+platform_helpers_.Count();
    }
    [[nodiscard]] RpClump* Floor() const noexcept { return floor_; }
    [[nodiscard]] NativeMovementSecondaryHelpers& PlatformHelpers() noexcept { return platform_helpers_; }
    [[nodiscard]] RpClump* Wall() const noexcept { return wall_; }
    [[nodiscard]] RpClump* Intri() const noexcept {
        return reinterpret_cast<RpClump*>(composite_.words[20u]);
    }
    [[nodiscard]] std::uint32_t IntriAtomics() const noexcept { return intri_atomics_; }
    [[nodiscard]] std::uint32_t RegisteredIntriAtomics() const noexcept {
        return composite_.words[6u];
    }
    [[nodiscard]] bool FindFloorCollision(
        const CollisionPoint3& position, float above, float below,
        NativeClumpCollisionHit& hit) const;
    [[nodiscard]] bool FindSecondaryCollision(
        const CollisionPoint3& position, float above, float below,
        NativeClumpCollisionHit& hit) const;
    [[nodiscard]] bool CheckFloorHeight(
        const CollisionPoint3& position, float above, float below,
        float max_difference) const;
    [[nodiscard]] bool CheckSecondaryHeight(
        const CollisionPoint3& position, float above, float below,
        float max_difference) const;
    [[nodiscard]] const ConfigCompositeState& State() const noexcept {
        return composite_;
    }
    [[nodiscard]] ConfigCompositeState& MutableState() noexcept {
        return composite_;
    }
    [[nodiscard]] ConfigCompositeState& MutableSecondaryState() noexcept {
        return secondary_;
    }
    [[nodiscard]] std::uint32_t FloorAtomics() const noexcept {
        return floor_atomics_;
    }
    [[nodiscard]] std::uint32_t WallAtomics() const noexcept {
        return wall_atomics_;
    }
    [[nodiscard]] std::uint32_t FloorTriangles() const noexcept {
        return floor_triangles_;
    }
    [[nodiscard]] std::uint32_t RegisteredFloorAtomics() const noexcept {
        return composite_.words[3u];
    }
    [[nodiscard]] std::uint32_t RegisteredWallAtomics() const noexcept {
        return composite_.words[9u];
    }
    [[nodiscard]] std::uint32_t RegisteredObjectAtomics() const noexcept {
        return composite_.words[12u];
    }
    [[nodiscard]] std::uint32_t RegisteredSecondaryObjectAtomics() const noexcept {
        return secondary_.words[12u];
    }
    [[nodiscard]] std::uint32_t PrimaryAddress() const noexcept {
        return static_cast<std::uint32_t>(
            reinterpret_cast<std::uintptr_t>(&composite_));
    }
    [[nodiscard]] std::uint32_t SecondaryAddress() const noexcept {
        return static_cast<std::uint32_t>(
            reinterpret_cast<std::uintptr_t>(&secondary_));
    }
    [[nodiscard]] std::uint32_t MissingTextures() const noexcept {
        return missing_textures_;
    }
    [[nodiscard]] std::size_t OutstandingAllocations() const noexcept {
        return memory_.OutstandingAllocations();
    }
    [[nodiscard]] const std::string& LastError() const noexcept {
        return last_error_;
    }

private:
    void InitializePrimary();
    static RpAtomic* RegisterIntri(RpAtomic* atomic, void* owner) noexcept;
    static RpAtomic* RegisterFloor(RpAtomic* atomic, void* owner) noexcept;
    static RpAtomic* RegisterWall(RpAtomic* atomic, void* owner) noexcept;
    static RpAtomic* RegisterObjectAtomic(RpAtomic* atomic,
                                          void* context) noexcept;
    static RpAtomic* UnregisterObjectAtomic(RpAtomic* atomic,
                                            void* context) noexcept;
    [[nodiscard]] std::uint32_t AllocateNode(std::uint32_t size) override;
    void WriteWord(std::uint32_t address, std::uint32_t value) override;
    [[nodiscard]] std::uint32_t ReadWord(std::uint32_t address) override;
    [[nodiscard]] std::uint32_t FreeNode(std::uint32_t address) override;
    [[nodiscard]] std::uint32_t DestroyClump(std::uint32_t address) override;
    [[nodiscard]] std::uint32_t GameInstance() override;
    [[nodiscard]] std::uint32_t LoadClump(
        const char* source, std::uint32_t flags,
        std::uint32_t texture_context) override;
    void AddClumpBounds(std::uint32_t clump, std::uint32_t mode,
                        ConfigCompositeState& state,
                        std::uint32_t bounds_word_index) override;
    [[nodiscard]] std::uint32_t ForEachAtomic(
        std::uint32_t clump, std::uint32_t callback,
        ConfigCompositeState& state) override;
    [[nodiscard]] std::uint32_t ForEachAtomicWithContext(
        std::uint32_t clump, std::uint32_t callback,
        const std::array<std::uint32_t, 5u>& context) override;

    NativeBaseObjectMemory memory_;
    NativeMovementSecondaryHelpers platform_helpers_;
    ConfigCompositeState composite_{};
    ConfigCompositeState secondary_{};
    RpClump* floor_ = nullptr;
    RpClump* wall_ = nullptr;
    std::uint32_t floor_atomics_ = 0u;
    std::uint32_t intri_atomics_ = 0u;
    std::uint32_t wall_atomics_ = 0u;
    std::uint32_t floor_triangles_ = 0u;
    std::uint32_t textures_ = 0u;
    std::uint32_t missing_textures_ = 0u;
    bool loaded_ = false;
    bool intri_prepared_ = false;
    NativePackageArchive* loading_archive_ = nullptr;
    std::uint32_t loading_game_ = 0u;
    bool composite_initialized_ = false;
    bool secondary_initialized_ = false;
    bool registration_failed_ = false;
    std::unordered_set<RpClump*> registered_object_clumps_;
    std::unordered_map<RpClump*, std::uint32_t> registered_event_primary_;
    std::unordered_map<RpClump*, std::uint32_t> registered_event_secondary_;
    std::string last_error_;
};

}  // namespace pal4
