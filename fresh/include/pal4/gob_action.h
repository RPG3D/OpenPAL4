#pragma once

#include "pal4/gob_resource_cleanup.h"

#include <cstdint>
#include <string>
#include <string_view>

namespace pal4 {

class IGobActionConfigValueRuntime {
public:
    virtual ~IGobActionConfigValueRuntime() = default;
    virtual std::int32_t ReadInt(std::string_view name, std::int32_t fallback) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    virtual std::uint32_t ClearAnimationFlag(std::uint32_t state, std::uint32_t flag) = 0;
};

class IGobActionConstructRuntime {
public:
    virtual ~IGobActionConstructRuntime() = default;
    virtual void ConstructBase(std::uint32_t object) = 0;
    virtual void DestructBase(std::uint32_t object) = 0;
    virtual void InitializeAnimation(std::uint32_t state) = 0;
    virtual void DestructAnimation(std::uint32_t state) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
};

class IGobActionVisibilityRuntime {
public:
    virtual ~IGobActionVisibilityRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t ClearAnimationFlag(
        std::uint32_t state, std::uint32_t flag) = 0;
    [[nodiscard]] virtual float AnimationLength(std::uint32_t state) = 0;
    virtual void SetAnimationTime(std::uint32_t state, float time) = 0;
    virtual void SetBaseVisible(std::uint32_t object,
                                std::uint32_t visible) = 0;
};

class IGobActionResetRuntime : public IGobActionVisibilityRuntime {
public:
    ~IGobActionResetRuntime() override = default;
    virtual void ResetAnimationSlot(std::uint32_t object) = 0;
};

class IGobActionStateRuntime : public IGobActionResetRuntime {
public:
    virtual std::uint32_t Allocate(std::uint32_t size) = 0;
    virtual void Free(std::uint32_t memory) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    virtual std::string ReadCString(std::uint32_t address) = 0;
    virtual void ConstructSnapshot(std::uint32_t snapshot) = 0;
    virtual void AssignSnapshotName(std::uint32_t snapshot, std::string_view name) = 0;
    virtual std::uint32_t SnapshotType(std::uint32_t snapshot) = 0;
};

class IGobActionFrameRuntime {
public:
    virtual ~IGobActionFrameRuntime()=default;
    virtual void RenderBase(std::uint32_t,std::uint32_t)=0;
    virtual std::uint32_t ProcessAnimation(std::uint32_t,std::uint32_t)=0;
};
class IGobActionUpdateRuntime {
public:
    virtual ~IGobActionUpdateRuntime()=default;
    virtual void UpdateBase(std::uint32_t object,float delta)=0;
    virtual void UpdateInterpolator(std::uint32_t state,float delta)=0;
    virtual std::uint32_t AnimationFlag(std::uint32_t state)=0;
    virtual std::uint32_t ReadWord(std::uint32_t address)=0;
    virtual void WriteWord(std::uint32_t address,std::uint32_t value)=0;
    virtual void SetAnimationTime(std::uint32_t state,float time)=0;
    virtual std::uint32_t ClearAnimationFlag(std::uint32_t state,std::uint32_t flag)=0;
};
class IGobActionRuntime : public IGobActionConstructRuntime,
                          public IGobActionStateRuntime, public IGobActionCleanupRuntime {
public:
    ~IGobActionRuntime() override = default;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) override = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t size) override = 0;
    virtual void Free(std::uint32_t memory) override = 0;
    virtual void CleanupAnimation(std::uint32_t state) override = 0;
    [[nodiscard]] virtual std::uint32_t CleanupBase(
        std::uint32_t object, std::uint32_t context) override = 0;
    virtual void RenderBase(std::uint32_t object,
                            std::uint32_t argument) = 0;
    [[nodiscard]] virtual std::uint32_t ProcessAnimation(
        std::uint32_t state, std::uint32_t argument) = 0;
    virtual void UpdateBase(std::uint32_t object, float delta) = 0;
    virtual void UpdateInterpolator(std::uint32_t state, float delta) = 0;
    [[nodiscard]] virtual std::uint32_t AnimationFlag(
        std::uint32_t state) = 0;
    virtual void CleanupBeforeSceneLoad(std::uint32_t object) = 0;
    [[nodiscard]] virtual bool UsesObjectPackage(
        std::string_view model_path) = 0;
    [[nodiscard]] virtual bool UsesWeaponPackage(
        std::string_view model_path) = 0;
    virtual void LoadObjectPackage() = 0;
    virtual void LoadWeaponPackage() = 0;
    [[nodiscard]] virtual std::uint32_t CurrentScenePackage() = 0;
    virtual void LoadScenePackage(std::uint32_t package) = 0;
    [[nodiscard]] virtual std::uint32_t LoadBaseModel(
        std::uint32_t object, std::uint32_t first,
        std::string_view model_path, std::uint32_t animation_data,
        std::uint32_t final_argument) = 0;
    virtual void HideHookUserDataAtomics(std::uint32_t clump) = 0;
    virtual void LoadAnimationData(
        std::uint32_t state, std::uint32_t clump,
        std::string_view model_path,
        std::uint32_t animation_data) = 0;
    virtual void SetAnimationFlag(std::uint32_t state,
                                  std::uint32_t flag) = 0;
    virtual void CleanupScriptPackages() = 0;
    [[nodiscard]] virtual std::uint32_t FindXmlChild(
        std::uint32_t parent, std::string_view name) = 0;
    [[nodiscard]] virtual std::uint32_t CreateXmlChild(
        std::uint32_t parent, std::string_view name) = 0;
    virtual void RemoveXmlChild(std::uint32_t parent,
                                std::uint32_t child) = 0;
    virtual void WriteXmlInt(std::uint32_t node,
                             std::string_view name,
                             std::int32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t FindXmlAttribute(
        std::uint32_t node, std::string_view name) = 0;
    [[nodiscard]] virtual std::int32_t XmlAttributeToInt(
        std::uint32_t attribute, std::int32_t fallback) = 0;
    virtual void ApplyBaseParameters(std::uint32_t object) = 0;
    [[nodiscard]] virtual std::string ReadCString(
        std::uint32_t address) override = 0;
    virtual void ConstructSnapshot(std::uint32_t snapshot) override = 0;
    virtual void AssignSnapshotName(std::uint32_t snapshot,
                                    std::string_view name) override = 0;
    [[nodiscard]] virtual std::uint32_t SnapshotType(
        std::uint32_t snapshot) override = 0;
};

class GobAction final {
public:
    static constexpr std::uint32_t kObjectSize = 0x264u;
    static constexpr std::uint32_t kVtable = 0x848aa0u;
    static constexpr std::uint32_t kAnimationVtable = 0x848b40u;
    static constexpr std::uint32_t kAnimationOffset = 0x1dcu;

    [[nodiscard]] static std::uint32_t TypeKey();
    [[nodiscard]] static std::uint32_t Create(IGobActionRuntime& runtime);
    [[nodiscard]] static std::uint32_t Construct(
        std::uint32_t object, IGobActionConstructRuntime& runtime);
    static void Destruct(std::uint32_t object,
                         IGobActionConstructRuntime& runtime);
    [[nodiscard]] static std::uint32_t CleanupResources(
        std::uint32_t object, std::uint32_t context,
        IGobActionCleanupRuntime& runtime);
    [[nodiscard]] static std::uint32_t HandleEvent(
        std::uint32_t object, std::uint32_t argument,
        IGobActionRuntime& runtime);
    static std::uint32_t HandleEvent(std::uint32_t,std::uint32_t,IGobActionFrameRuntime&);
    [[nodiscard]] static std::uint32_t Update(
        std::uint32_t object, float delta, IGobActionRuntime& runtime);
    [[nodiscard]] static std::uint32_t Update(
        std::uint32_t object,float delta,IGobActionUpdateRuntime& runtime);
    [[nodiscard]] static std::uint32_t InitializeSceneAndAnimation(
        std::uint32_t object, std::uint32_t first,
        std::string_view model_path, std::uint32_t animation_data,
        std::uint32_t final_argument, IGobActionRuntime& runtime);
    [[nodiscard]] static std::uint32_t ResetAnimation(
        std::uint32_t object, IGobActionResetRuntime& runtime);
    [[nodiscard]] static std::uint32_t GetOrCreateActionConfigNode(
        std::uint32_t object, IGobActionRuntime& runtime);
    static void SetVisibleAndAnimationState(
        std::uint32_t object, std::uint32_t visible,
        IGobActionVisibilityRuntime& runtime);
    static void ReadConfig(std::uint32_t object,
                           IGobActionRuntime& runtime);
    // Value boundary of IDA59AEE0, shared by XML-node and native scene adapters.
    static void ApplyConfigValues(std::uint32_t object,
                                  IGobActionConfigValueRuntime& runtime);
    [[nodiscard]] static std::uint32_t CreateStateSnapshot(
        std::uint32_t object, IGobActionStateRuntime& runtime);
    static void ApplyStateSnapshot(std::uint32_t object,
                                   std::uint32_t snapshot,
                                   IGobActionStateRuntime& runtime);
};

}  // namespace pal4
