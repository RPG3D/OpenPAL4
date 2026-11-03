#pragma once

#include "pal4/gob_resource_cleanup.h"

#include <cstdint>
#include <string>
#include <string_view>

namespace pal4 {

class IGobObjectConstructRuntime {
public:
    virtual ~IGobObjectConstructRuntime() = default;
    virtual void BaseConstruct(std::uint32_t object) = 0;
    virtual void BaseDestruct(std::uint32_t object) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    virtual void InitializeEmptyString(std::uint32_t record) = 0;
    virtual void CleanupString(std::uint32_t record) = 0;
    virtual void ConstructNameNode(std::uint32_t handle) = 0;
    virtual void ResetEffect(std::uint32_t handle) = 0;
};

class IGobObjectParameterValueRuntime {
public:
    virtual ~IGobObjectParameterValueRuntime() = default;
    [[nodiscard]] virtual float ReadFloat(std::string_view name,
                                          float fallback) = 0;
    [[nodiscard]] virtual std::int32_t ReadInt(std::string_view name,
                                                std::int32_t fallback) = 0;
    [[nodiscard]] virtual std::int32_t FloatToInteger(float value) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    virtual void ApplyScale(std::uint32_t object, float scale,
                            std::uint32_t mode) = 0;
};

class IGobObjectVisibilityRuntime {
public:
    virtual ~IGobObjectVisibilityRuntime() = default;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    [[nodiscard]] virtual bool DialogNeedsUpdate(
        std::uint32_t handle) = 0;
    virtual void UpdateDialogContent(std::uint32_t handle) = 0;
    [[nodiscard]] virtual bool EffectReadyToShow(
        std::uint32_t handle) = 0;
    [[nodiscard]] virtual std::uint32_t RemoveEffect(
        std::uint32_t handle) = 0;
};

class IGobObjectRenderRuntime {
public:
    virtual ~IGobObjectRenderRuntime()=default;
    virtual std::uint32_t ReadWord(std::uint32_t)=0;
    virtual std::uint32_t RenderClump(std::uint32_t object,std::uint32_t camera)=0;
};
class IGobInteractionBoundsRuntime {
public:
    virtual ~IGobInteractionBoundsRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual float InteractionThreshold(std::uint32_t object) = 0;
    [[nodiscard]] virtual double PlanarMagnitude(float x, float z) = 0;
};

// Research-use accounting does not construct resources or parse scene records.
// Keep this small endpoint available to native derived-object event adapters.
class IGobResearchUseRuntime {
public:
    virtual ~IGobResearchUseRuntime() = default;
    virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    virtual void AddActiveSceneObject(std::uint32_t object, float value) = 0;
    virtual std::uint32_t ProcessMarker() = 0;
};

class IGobObjectDocumentRuntime {
public:
    virtual ~IGobObjectDocumentRuntime() = default;
    virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,std::uint32_t value) = 0;
    virtual void DestructXmlDocument(std::uint32_t document) = 0;
    virtual std::uint32_t FreeMemory(std::uint32_t memory) = 0;
    virtual std::uint32_t AllocateMemory(std::uint32_t size) = 0;
    virtual std::uint32_t InitializeXmlDocument(std::uint32_t memory,
        std::string_view root,std::uint32_t mode) = 0;
    virtual std::uint32_t CreateXmlNode(std::uint32_t document,std::string_view name) = 0;
};

class IGobObjectBaseRuntime : public IGobObjectConstructRuntime,
                             public IGobObjectDocumentRuntime,
                             public IGobInteractionBoundsRuntime,
                              public IGobObjectVisibilityRuntime, public IGobBaseCleanupRuntime,
                              public IGobResearchUseRuntime {
public:
    ~IGobObjectBaseRuntime() override = default;
    virtual void WriteWord(std::uint32_t,std::uint32_t) override = 0;
    virtual bool EffectReadyToShow(std::uint32_t) override = 0;
    virtual std::uint32_t RemoveEffect(std::uint32_t) override = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) override = 0;
    [[nodiscard]] virtual bool EffectNeedsUpdate(
        std::uint32_t handle) override = 0;
    virtual void HandleEffectEvent(std::uint32_t handle,
                                   std::uint32_t event_context,
                                   std::uint32_t mode) override = 0;
    [[nodiscard]] virtual std::uint32_t CleanupClump(
        std::uint32_t object) override = 0;
    virtual void DestructXmlDocument(std::uint32_t document) override = 0;
    [[nodiscard]] virtual std::uint32_t FreeMemory(
        std::uint32_t memory) override = 0;
    [[nodiscard]] virtual std::uint32_t AllocateMemory(
        std::uint32_t size) = 0;
    [[nodiscard]] virtual std::uint32_t InitializeXmlDocument(
        std::uint32_t memory, std::string_view root,
        std::uint32_t mode) = 0;
    [[nodiscard]] virtual std::uint32_t CreateXmlNode(
        std::uint32_t document,
        std::string_view node_name) = 0;
    [[nodiscard]] virtual std::uint32_t RenderClump(
        std::uint32_t object, std::uint32_t argument) = 0;
    [[nodiscard]] virtual std::uint32_t UpdateClump(
        std::uint32_t object, float delta_seconds) = 0;
    [[nodiscard]] virtual std::uint32_t LoadClump(
        std::uint32_t object, std::uint32_t first,
        std::uint32_t second, std::uint32_t third) = 0;
    virtual void HideUserDataAtomics(std::uint32_t clump) = 0;
    [[nodiscard]] virtual std::string ReadCString(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual bool CanUseVariant(
        std::uint32_t plugin_context,
        std::uint32_t variant_name) = 0;
    [[nodiscard]] virtual std::uint32_t FindTexture(
        std::string_view path) = 0;
    virtual void LoadTexture(std::uint32_t object,
                             std::uint32_t variant_name,
                             std::uint32_t texture) = 0;
    [[nodiscard]] virtual std::uint32_t FindConfigAttribute(
        std::uint32_t document, std::string_view name) = 0;
    [[nodiscard]] virtual float ConfigAttributeToFloat(
        std::uint32_t attribute, float fallback) = 0;
    [[nodiscard]] virtual std::int32_t ConfigAttributeToInt(
        std::uint32_t attribute, std::int32_t fallback) = 0;
    [[nodiscard]] virtual std::int32_t FloatToInteger(float value) = 0;
    virtual void ApplyScale(std::uint32_t object, float scale,
                            std::uint32_t mode) = 0;
    [[nodiscard]] virtual std::uint32_t ReadStreamBuffer(
        std::uint32_t stream) = 0;
    virtual void ReadStreamBytes(std::uint32_t stream,
                                 std::uint32_t destination,
                                 std::uint32_t size) = 0;
    [[nodiscard]] virtual std::uint32_t InitializeFromRecord(
        std::uint32_t object, std::uint32_t name,
        std::uint32_t directory, std::uint32_t model,
        std::uint32_t variant, std::uint32_t context) = 0;
    virtual void AssignScriptName(std::uint32_t string_record,
                                  std::string_view name) = 0;
    [[nodiscard]] virtual std::uint32_t ReadStreamType(
        std::uint32_t stream) = 0;
    virtual void CheckRecordXml(std::uint32_t stream,
                                std::uint32_t document) = 0;
    virtual void FinalizeRecord(std::uint32_t object) = 0;
    virtual void ApplyPosition(std::uint32_t object,
                               std::uint32_t vector, std::uint32_t mode) = 0;
    virtual void ApplyRotation(std::uint32_t object,
                               std::uint32_t vector, std::uint32_t mode) = 0;
    virtual void ApplyRecordVisibility(std::uint32_t object,
                                       std::uint32_t visible) = 0;
    virtual void FreeStreamBuffer(std::uint32_t buffer) = 0;
    [[nodiscard]] virtual std::uint32_t ProcessMarker() override = 0;
    [[nodiscard]] virtual float InteractionThreshold(
        std::uint32_t object) override = 0;
    virtual void AddActiveSceneObject(std::uint32_t object,
                                      float value) override = 0;
};

class GobObjectBase final {
public:
    static constexpr std::uint32_t kVtable = 0x848a00u;
    // IDA 0x599f30..0x59a9b0; later vtable entries continue in address order.
    [[nodiscard]] static std::uint32_t Construct(
        std::uint32_t object, IGobObjectConstructRuntime& runtime);
    static void Destruct(std::uint32_t object,
                         IGobObjectConstructRuntime& runtime);
    [[nodiscard]] static std::uint32_t CleanupResources(
        std::uint32_t object, std::uint32_t event_context,
        IGobBaseCleanupRuntime& runtime);
    [[nodiscard]] static std::uint32_t RenderIfVisible(
        std::uint32_t object, std::uint32_t argument,
        IGobObjectBaseRuntime& runtime);
    [[nodiscard]] static std::uint32_t UpdateClumpIfVisible(
        std::uint32_t object, float delta_seconds,
        IGobObjectBaseRuntime& runtime);
    [[nodiscard]] static std::uint32_t CreateParametersNode(
        std::uint32_t object, IGobObjectDocumentRuntime& runtime);
    [[nodiscard]] static std::uint32_t LoadAndInitialize(
        std::uint32_t object, std::uint32_t first,
        std::uint32_t second, std::uint32_t third,
        std::uint32_t variant_name, IGobObjectBaseRuntime& runtime);
    static void ApplyParameters(std::uint32_t object,
                                IGobObjectBaseRuntime& runtime);
    static std::uint32_t RenderIfVisible(std::uint32_t,std::uint32_t,IGobObjectRenderRuntime&);
    static void ApplyParameters(std::uint32_t object,
                                IGobObjectParameterValueRuntime& runtime);
    [[nodiscard]] static std::uint32_t LoadRecord(
        std::uint32_t object, std::uint32_t context,
        std::uint32_t stream, IGobObjectBaseRuntime& runtime);
    [[nodiscard]] static std::uint32_t ProcessMarkerIfSet(
        std::uint32_t object, IGobObjectBaseRuntime& runtime);
    [[nodiscard]] static std::uint32_t ProcessMarkerIfSet(
        std::uint32_t object, IGobResearchUseRuntime& runtime);
    struct Position {
        float x;
        float y;
        float z;
    };
    [[nodiscard]] static bool IsWithinInteractionBounds(
        std::uint32_t object, const Position& position,
        float vertical_radius, float* horizontal_distance,
        IGobInteractionBoundsRuntime& runtime);
    [[nodiscard]] static bool IsResearchVisibleCandidate(
        std::uint32_t object, IGobObjectBaseRuntime& runtime);
    [[nodiscard]] static std::uint8_t SetVisible(
        std::uint32_t object, std::uint32_t visible,
        IGobObjectVisibilityRuntime& runtime);
    [[nodiscard]] static bool IsMarkedVisibleWithUses(
        std::uint32_t object, IGobObjectBaseRuntime& runtime);
    [[nodiscard]] static std::uint32_t ConsumeResearchUse(
        std::uint32_t object, IGobObjectBaseRuntime& runtime);
    [[nodiscard]] static std::uint32_t ConsumeResearchUse(
        std::uint32_t object, IGobResearchUseRuntime& runtime);
    [[nodiscard]] static std::uint32_t AlwaysOne();
    [[nodiscard]] static std::uint32_t AutoDisappear(
        std::uint32_t object, IGobObjectBaseRuntime& runtime);
    [[nodiscard]] static std::uint32_t CameraCollide(
        std::uint32_t object, IGobObjectBaseRuntime& runtime);
    [[nodiscard]] static float InteractionRangeThreshold(
        std::uint32_t object, IGobObjectBaseRuntime& runtime);
    [[nodiscard]] static std::uint32_t ConfigNode(
        std::uint32_t object, IGobObjectBaseRuntime& runtime);
};

}  // namespace pal4
