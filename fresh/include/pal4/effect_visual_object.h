#pragma once

#include <cstddef>
#include <cstdint>
#include <array>

namespace pal4 {

struct EffectVector3 {
    float x{};
    float y{};
    float z{};
};

struct EffectMatrix {
    EffectVector3 right{};
    std::uint32_t flags{};
    EffectVector3 up{};
    std::uint32_t pad1{};
    EffectVector3 at{};
    std::uint32_t pad2{};
    EffectVector3 position{};
    std::uint32_t pad3{};
};
static_assert(sizeof(EffectMatrix) == 64u);

struct EffectVisualObjectState {
    std::uint32_t owner{};             // +0
    std::uint32_t source{};            // +4
    std::uint32_t context{};           // +8
    std::uint8_t active{};             // +12
    std::uint8_t transform_mode{};     // +13
    std::uint8_t geometry_mode{};      // +14
    std::uint8_t property_flag{};      // +15
    std::uint8_t enabled{};            // +16
    std::uint8_t padding[3]{};
    std::uint32_t frame{};             // +20
    std::uint32_t translation[4]{};   // +24..+39
    std::uint32_t scale[2]{};         // +40..+47
    std::uint32_t geometry{};          // +48
};
static_assert(sizeof(EffectVisualObjectState) == 52u);
static_assert(offsetof(EffectVisualObjectState, active) == 12u);
static_assert(offsetof(EffectVisualObjectState, enabled) == 16u);
static_assert(offsetof(EffectVisualObjectState, frame) == 20u);
static_assert(offsetof(EffectVisualObjectState, geometry) == 48u);

class IEffectVisualObjectRuntime {
public:
    virtual ~IEffectVisualObjectRuntime() = default;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    virtual void Free(std::uint32_t address) = 0;
    virtual void WriteGeometryByte(std::uint32_t address,
                                   std::uint8_t value) = 0;
    virtual void WriteGeometryWord(std::uint32_t address,
                                   std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t ReadGeometryWord(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual bool InitializeGeometry(
        std::uint32_t geometry, std::uint32_t source,
        std::uint32_t vector, std::uint8_t mode,
        std::uint32_t option) = 0;
    virtual void DestroyGeometry(std::uint32_t geometry) = 0;
    [[nodiscard]] virtual std::uint32_t CreateFrame() = 0;
    virtual void DestroyFrame(std::uint32_t frame) = 0;
    virtual void SetFrameIdentity(std::uint32_t frame) = 0;
    virtual void ScaleFrame(
        std::uint32_t frame,
        const std::array<std::uint32_t, 3u>& scale,
        std::uint32_t mode) = 0;
    virtual void RotateFrame(std::uint32_t frame,
                             std::uint32_t axis,
                             float angle, std::uint32_t mode) = 0;
    virtual void TranslateFrame(
        std::uint32_t frame,
        const std::array<std::uint32_t, 3u>& translation,
        std::uint32_t mode) = 0;
    [[nodiscard]] virtual std::uint8_t UpdateFrameObjects(
        std::uint32_t frame) = 0;
    virtual void AddFrameChild(std::uint32_t frame,
                               std::uint32_t child) = 0;
    [[nodiscard]] virtual std::int32_t SetGeometryState(
        std::uint32_t geometry, std::int32_t mode) = 0;
    virtual void NoopUpdate(std::uint32_t first, std::uint32_t second) = 0;
    [[nodiscard]] virtual std::uint32_t ReadObjectWord(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual EffectMatrix GetObjectTransform(
        std::uint32_t object) = 0;
    virtual void Normalize(EffectVector3& vector) = 0;
    [[nodiscard]] virtual EffectMatrix SetupParentTransform(
        std::uint32_t frame) = 0;
    virtual void TransformVector(EffectVector3& vector,
                                 const EffectMatrix& matrix) = 0;
    virtual void UpdateMatrix(EffectMatrix& matrix) = 0;
    virtual void ApplyMatrix(std::uint32_t frame,
                             const EffectMatrix& matrix,
                             std::uint32_t mode) = 0;
    [[nodiscard]] virtual std::uint8_t RenderWithStates(
        std::uint32_t geometry, std::uint32_t first,
        std::uint32_t second) = 0;
};

class EffectVisualObject final {
public:
    // IDA 0x6428d0: change the enabled flag and forward mode 0 or 2.
    [[nodiscard]] static std::int32_t SetEnabled(
        EffectVisualObjectState& object, std::uint8_t enabled,
        IEffectVisualObjectRuntime& runtime);
    // IDA 0x642900/0x6429f0: two initialization variants.
    [[nodiscard]] static std::uint8_t Initialize(
        EffectVisualObjectState& object, std::uint32_t source,
        std::uint32_t vector, std::uint8_t geometry_mode,
        std::uint8_t property_flag, std::uint32_t option,
        IEffectVisualObjectRuntime& runtime);
    [[nodiscard]] static std::uint8_t InitializeFromObject(
        EffectVisualObjectState& object,
        const EffectVisualObjectState& template_object,
        std::uint32_t source, std::uint32_t vector,
        std::uint8_t geometry_mode, std::uint8_t property_flag,
        std::uint32_t option, IEffectVisualObjectRuntime& runtime);
    // IDA 0x642af0: release the active frame and geometry.
    static void Deinitialize(EffectVisualObjectState& object,
                             IEffectVisualObjectRuntime& runtime);
    // IDA 0x642b50: route an active object to one of three update paths.
    [[nodiscard]] static std::uint8_t Update(
        EffectVisualObjectState& object,
        std::uint32_t first, std::uint32_t second,
        std::uint32_t value, IEffectVisualObjectRuntime& runtime);
    // IDA 0x642ba0: reset frame transform only for an active object.
    [[nodiscard]] static std::uint8_t InitializeTransform(
        EffectVisualObjectState& object,
        IEffectVisualObjectRuntime& runtime);
    // IDA 0x642c40: rotate and scale the geometry child frame.
    [[nodiscard]] static std::uint8_t UpdateRotation(
        EffectVisualObjectState& object,
        IEffectVisualObjectRuntime& runtime);
    // IDA 0x642ca0: orient from the input object's frame and parent frame.
    [[nodiscard]] static std::uint8_t UpdateTransform(
        EffectVisualObjectState& object, std::uint32_t value,
        IEffectVisualObjectRuntime& runtime);
    // IDA 0x6432f0: orient from the object's first three float words.
    [[nodiscard]] static std::uint8_t UpdateTransformComplex(
        EffectVisualObjectState& object,
        IEffectVisualObjectRuntime& runtime);
    // IDA 0x6438d0: forward rendering only while active.
    [[nodiscard]] static std::uint8_t Render(
        const EffectVisualObjectState& object,
        std::uint32_t first, std::uint32_t second,
        IEffectVisualObjectRuntime& runtime);

private:
    [[nodiscard]] static std::uint32_t AllocateGeometry(
        IEffectVisualObjectRuntime& runtime);
    static void ResetTransforms(EffectVisualObjectState& object);
    static void ReleaseFailedGeometry(
        EffectVisualObjectState& object,
        IEffectVisualObjectRuntime& runtime);
};

}  // namespace pal4
