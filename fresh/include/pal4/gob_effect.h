#pragma once

#include "pal4/gob_resource_cleanup.h"

#include <cstdint>
#include <string>
#include <string_view>
#include <utility>

namespace pal4 {

class IGobEffectConstructRuntime {
public:
    virtual ~IGobEffectConstructRuntime() = default;
    virtual void ConstructBase(std::uint32_t object) = 0;
    virtual void DestructBase(std::uint32_t object) = 0;
    virtual void ConstructEffectHandle(std::uint32_t handle) = 0;
    virtual void ResetEffectHandle(std::uint32_t handle) = 0;
    virtual void InitializeEmptyString(std::uint32_t record) = 0;
    virtual void CleanupString(std::uint32_t record) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
};

class IGobEffectConfigValueRuntime {
public:
    virtual ~IGobEffectConfigValueRuntime() = default;
    [[nodiscard]] virtual std::string ReadString(
        std::string_view name, std::string_view fallback) = 0;
    [[nodiscard]] virtual std::int32_t ReadInt(
        std::string_view name, std::int32_t fallback) = 0;
    virtual void AssignEffectName(std::uint32_t record,
                                  std::string_view name) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
};

class IGobEffectVisibilityRuntime {
public:
    virtual ~IGobEffectVisibilityRuntime() = default;
    virtual void StoreWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual bool EffectNeedsUpdate(std::uint32_t handle) = 0;
    virtual void HandleEffectEvent(std::uint32_t handle,
                                   std::uint32_t context,
                                   std::uint32_t mode) = 0;
    [[nodiscard]] virtual std::string EffectName(std::uint32_t record) = 0;
    [[nodiscard]] virtual std::pair<std::uint32_t, std::uint32_t>
        CreateEffect(std::string_view name, std::uint32_t times,
                     bool distance_clip) = 0;
    virtual void ResetTemporaryEffect() = 0;
    virtual void UpdateEffectProperties(std::uint32_t handle,
                                        std::uint32_t atomic) = 0;
    [[nodiscard]] virtual std::uint8_t UpdateEffectState(
        std::uint32_t handle) = 0;
    [[nodiscard]] virtual std::uint8_t UpdateDialogContent(
        std::uint32_t handle) = 0;
};

class IGobEffectSaverRuntime {
public:
    virtual ~IGobEffectSaverRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t InitializeGameState(
        std::uint32_t state, std::uint32_t mode) = 0;
};

class IGobEffectSnapshotRuntime {
public:
    virtual ~IGobEffectSnapshotRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t size) = 0;
    virtual void Free(std::uint32_t memory) = 0;
    [[nodiscard]] virtual std::string ReadCString(std::uint32_t address) = 0;
    virtual void ConstructSnapshot(std::uint32_t snapshot) = 0;
    virtual void AssignSnapshotName(std::uint32_t snapshot,
                                    std::string_view name) = 0;
    [[nodiscard]] virtual std::uint32_t SnapshotType(
        std::uint32_t snapshot) = 0;
    virtual void SetBaseVisible(std::uint32_t object,
                                std::uint32_t visible) = 0;
};

class IGobEffectFrameRuntime {
public:
    virtual ~IGobEffectFrameRuntime()=default;
    virtual std::uint32_t ReadWord(std::uint32_t)=0;
    virtual std::uint32_t RenderBase(std::uint32_t,std::uint32_t)=0;
};
class IGobEffectRuntime : public IGobEffectConstructRuntime,
                          public IGobEffectVisibilityRuntime,
                          public IGobEffectSaverRuntime,
                          public IGobEffectSnapshotRuntime, public IGobEffectCleanupRuntime {
public:
    ~IGobEffectRuntime() override = default;
    virtual bool EffectNeedsUpdate(std::uint32_t) override = 0;
    virtual void HandleEffectEvent(std::uint32_t,std::uint32_t,std::uint32_t) override = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) override = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) override = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t size) override = 0;
    virtual void Free(std::uint32_t memory) override = 0;
    [[nodiscard]] virtual std::uint32_t CleanupBase(
        std::uint32_t object, std::uint32_t context) override = 0;
    [[nodiscard]] virtual std::uint32_t RenderBase(
        std::uint32_t object, std::uint32_t argument) = 0;
    virtual void CleanupBeforeSceneLoad(std::uint32_t object) = 0;
    [[nodiscard]] virtual bool UsesObjectPackage(
        std::string_view path) = 0;
    [[nodiscard]] virtual bool UsesWeaponPackage(
        std::string_view path) = 0;
    virtual void LoadObjectPackage() = 0;
    virtual void LoadWeaponPackage() = 0;
    [[nodiscard]] virtual std::uint32_t CurrentScenePackage() = 0;
    virtual void LoadScenePackage(std::uint32_t package) = 0;
    [[nodiscard]] virtual std::uint32_t LoadBaseModel(
        std::uint32_t object, std::uint32_t first,
        std::string_view path, std::uint32_t fourth,
        std::uint32_t fifth) = 0;
    [[nodiscard]] virtual std::uint32_t FindXmlChild(
        std::uint32_t parent, std::string_view name) = 0;
    virtual void RemoveXmlChild(std::uint32_t parent,
                                std::uint32_t child) = 0;
    [[nodiscard]] virtual std::uint32_t CreateXmlChild(
        std::uint32_t parent, std::string_view name) = 0;
    virtual void CleanupScriptPackages() = 0;
    virtual void ApplyBaseParameters(std::uint32_t object) = 0;
    [[nodiscard]] virtual std::string ReadXmlString(
        std::uint32_t node, std::string_view name,
        std::string_view fallback) = 0;
    virtual void AssignEffectName(std::uint32_t record,
                                  std::string_view name) = 0;
    [[nodiscard]] virtual std::uint32_t FindXmlAttribute(
        std::uint32_t node, std::string_view name) = 0;
    [[nodiscard]] virtual std::int32_t XmlAttributeToInt(
        std::uint32_t attribute, std::int32_t fallback) = 0;
    [[nodiscard]] virtual std::uint32_t UpdateClumpTransform(
        std::uint32_t object, std::uint32_t transform,
        std::uint32_t mode) = 0;
};

class GobEffect final {
public:
    static constexpr std::uint32_t kSize = 0x1fcu;
    static constexpr std::uint32_t kVtable = 0x848c18u;
    static constexpr std::uint32_t kEffectHandleOffset = 0x1dcu;
    static constexpr std::uint32_t kStringOffset = 0x1e4u;

    [[nodiscard]] static std::uint32_t TypeKey();
    [[nodiscard]] static std::uint32_t Create(
        IGobEffectRuntime& runtime);
    [[nodiscard]] static std::uint32_t Construct(
        std::uint32_t object, IGobEffectConstructRuntime& runtime);
    static void Destruct(std::uint32_t object,
                         IGobEffectConstructRuntime& runtime);
    [[nodiscard]] static std::uint32_t CleanupResources(
        std::uint32_t object, std::uint32_t context,
        IGobEffectCleanupRuntime& runtime);
    [[nodiscard]] static std::uint32_t HandleEventIfEnabled(
        std::uint32_t object, std::uint32_t argument,
        IGobEffectRuntime& runtime);
    static std::uint32_t HandleEventIfEnabled(std::uint32_t,std::uint32_t,IGobEffectFrameRuntime&);
    [[nodiscard]] static std::uint32_t InitializeSceneAndConfig(
        std::uint32_t object, std::uint32_t first,
        std::string_view path, std::uint32_t fourth,
        std::uint32_t fifth, IGobEffectRuntime& runtime);
    [[nodiscard]] static std::uint32_t TriggerSaverStateIfEnabled(
        std::uint32_t object, IGobEffectSaverRuntime& runtime);
    [[nodiscard]] static std::uint32_t GetConfigNode(
        std::uint32_t object, IGobEffectRuntime& runtime);
    static void ReadConfig(std::uint32_t object,
                           IGobEffectRuntime& runtime);
    static void ApplyConfigValues(std::uint32_t object,
                                  IGobEffectConfigValueRuntime& runtime);
    [[nodiscard]] static std::uint32_t UpdateClumpTransform(
        std::uint32_t object, std::uint32_t transform,
        std::uint32_t mode, IGobEffectRuntime& runtime);
    [[nodiscard]] static std::uint8_t SetVisibleAndCreateEffect(
        std::uint32_t object, std::uint32_t context,
        std::uint32_t visible, IGobEffectVisibilityRuntime& runtime);
    [[nodiscard]] static std::uint32_t CreateStateSnapshot(
        std::uint32_t object, IGobEffectSnapshotRuntime& runtime);
    static void ApplyStateSnapshot(std::uint32_t object,
                                   std::uint32_t snapshot,
                                   IGobEffectSnapshotRuntime& runtime);
};

}  // namespace pal4
