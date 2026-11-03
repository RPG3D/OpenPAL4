#pragma once

#include "pal4/gob_resource_cleanup.h"

#include <cstdint>
#include <string_view>

namespace pal4 {

class IGobLocalFloorRuntime : public IGobInheritedCleanupRuntime {
public:
    virtual ~IGobLocalFloorRuntime() = default;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t size) = 0;
    virtual void Free(std::uint32_t address) = 0;
    virtual void ConstructBase(std::uint32_t object) = 0;
    virtual void DestructBase(std::uint32_t object) = 0;
    [[nodiscard]] virtual std::uint32_t CleanupBase(
        std::uint32_t object, std::uint32_t context) = 0;
    [[nodiscard]] virtual std::uint32_t RenderBaseIfVisible(
        std::uint32_t object, std::uint32_t context) = 0;
    [[nodiscard]] virtual std::uint32_t UpdateBaseClumpIfVisible(
        float elapsed) = 0;
    virtual void CleanupBeforeSceneLoad(std::uint32_t object) = 0;
    [[nodiscard]] virtual bool UsesObjectPackage(std::string_view path) = 0;
    [[nodiscard]] virtual bool UsesWeaponPackage(std::string_view path) = 0;
    virtual void LoadObjectPackage() = 0;
    virtual void LoadWeaponPackage() = 0;
    [[nodiscard]] virtual std::uint32_t CurrentScenePackage() = 0;
    virtual void LoadScenePackage(std::uint32_t package) = 0;
    [[nodiscard]] virtual std::uint32_t InitializeBaseModel(
        std::uint32_t object, std::uint32_t first,
        std::string_view path, std::uint32_t fourth,
        std::uint32_t fifth) = 0;
    virtual void CleanupScriptPackages() = 0;
    [[nodiscard]] virtual std::uint32_t AttachMovementHelper(
        std::uint32_t helper) = 0;
    [[nodiscard]] virtual std::uint32_t DetachMovementHelper(
        std::uint32_t helper) = 0;
    [[nodiscard]] virtual std::string_view ReadCString(
        std::uint32_t address) = 0;
    virtual void ConstructSnapshot(std::uint32_t snapshot) = 0;
    virtual void AssignSnapshotName(std::uint32_t snapshot,
                                    std::string_view name) = 0;
    [[nodiscard]] virtual std::uint32_t SnapshotType(
        std::uint32_t snapshot) = 0;
    [[nodiscard]] virtual std::uint32_t SetBaseVisible(
        std::uint32_t object, std::uint32_t visible) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
};

class GobLocalFloor final {
public:
    static constexpr std::uint32_t kSize = 0x1e0u;
    static constexpr std::uint32_t kVtable = 0x848e98u;
    static constexpr std::uint32_t kHelperOffset = 0x1dcu;
    static constexpr std::uint32_t kHelperVtable = 0x848e84u;
    static constexpr std::uint32_t kHelperBaseVtable = 0x848da8u;

    [[nodiscard]] static std::uint32_t Create(IGobLocalFloorRuntime& runtime);
    [[nodiscard]] static std::uint32_t Construct(
        std::uint32_t object, IGobLocalFloorRuntime& runtime);
    [[nodiscard]] static std::uint32_t TypeKey();
    [[nodiscard]] static double InteractionThreshold();
    static void Destruct(std::uint32_t object,
                         IGobLocalFloorRuntime& runtime);
    [[nodiscard]] static std::uint32_t CleanupResources(
        std::uint32_t object, std::uint32_t context,
        IGobInheritedCleanupRuntime& runtime);
    [[nodiscard]] static std::uint32_t RenderIfVisible(
        std::uint32_t object, std::uint32_t context,
        IGobLocalFloorRuntime& runtime);
    [[nodiscard]] static std::uint32_t UpdateClumpIfVisible(
        float elapsed, IGobLocalFloorRuntime& runtime);
    [[nodiscard]] static std::uint32_t InitializeSceneAndConfig(
        std::uint32_t object, std::uint32_t first,
        std::string_view path, std::uint32_t fourth,
        std::uint32_t fifth, IGobLocalFloorRuntime& runtime);
    [[nodiscard]] static std::uint32_t AttachMovementAdapter(
        std::uint32_t object, IGobLocalFloorRuntime& runtime);
    [[nodiscard]] static std::uint32_t DetachMovementAdapter(
        std::uint32_t object, IGobLocalFloorRuntime& runtime);
    [[nodiscard]] static std::uint32_t HelperClump(
        std::uint32_t helper, IGobLocalFloorRuntime& runtime);
    [[nodiscard]] static std::uint32_t CreateStateSnapshot(
        std::uint32_t object, IGobLocalFloorRuntime& runtime);
    [[nodiscard]] static std::uint32_t ApplyStateSnapshot(
        std::uint32_t object, std::uint32_t snapshot,
        IGobLocalFloorRuntime& runtime);
};

}  // namespace pal4
