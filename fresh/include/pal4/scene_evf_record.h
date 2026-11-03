#pragma once

#include <array>
#include <cstdint>
#include <string_view>
#include <vector>

#include "pal4/effect_property_state.h"

namespace pal4 {

struct SceneEvfRecordState final {
    std::array<std::uint32_t, 43u> words{};
};
static_assert(sizeof(SceneEvfRecordState) == 0xacu);

class ISceneEvfRecordRuntime {
public:
    virtual ~ISceneEvfRecordRuntime() = default;
    virtual void InitializeEmptyString(SceneEvfRecordState& state,
                                       std::uint32_t byte_offset) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
};

class ISceneEvfListRuntime {
public:
    virtual ~ISceneEvfListRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadListWord(
        std::uint32_t address) = 0;
    virtual void WriteListWord(std::uint32_t address,
                               std::uint32_t value) = 0;
    virtual void FreeListNode(std::uint32_t address) = 0;
};

class ISceneEvfRecordCleanupRuntime : public ISceneEvfListRuntime {
public:
    virtual ~ISceneEvfRecordCleanupRuntime() = default;
    virtual void ClearBufferRange(std::uint32_t begin,
                                  std::uint32_t end) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual EffectPropertyState& PropertyAt(
        std::uint32_t address) = 0;
    virtual void Free(std::uint32_t address) = 0;
    virtual void DestroyFrame(std::uint32_t frame,
                              std::uint32_t flag) = 0;
    [[nodiscard]] virtual std::uint32_t ClumpWorld(
        std::uint32_t clump) = 0;
    virtual void RemoveClump(std::uint32_t world,
                             std::uint32_t clump) = 0;
    virtual void DestroyClump(std::uint32_t clump) = 0;
    virtual void RemoveListElement(std::uint32_t list_head,
                                   std::uint32_t node) = 0;
    [[nodiscard]] virtual int CleanupString(
        SceneEvfRecordState& state, std::uint32_t offset) = 0;
};

class ISceneEvfRecordStringRuntime {
public:
    virtual ~ISceneEvfRecordStringRuntime() = default;
    [[nodiscard]] virtual std::uint32_t SetString(
        SceneEvfRecordState& state, std::uint32_t offset,
        std::string_view value) = 0;
};

class ISceneEvfRecordTypeRuntime {
public:
    virtual ~ISceneEvfRecordTypeRuntime() = default;
    virtual void DestroyOld(std::uint32_t object,
                            std::uint32_t flag) = 0;
    [[nodiscard]] virtual std::uint32_t FindFactory(
        std::uint32_t owner, std::uint32_t type) = 0;
    [[nodiscard]] virtual std::uint32_t Create(
        std::uint32_t factory) = 0;
    virtual void BindOwnerField(std::uint32_t object,
                                std::uint32_t field_address) = 0;
};

struct SceneEvfVertex final {
    std::array<std::uint32_t, 9u> words{};
};
static_assert(sizeof(SceneEvfVertex) == 36u);

struct SceneEvfTriangle final {
    std::array<std::uint32_t, 3u> indices{};
    std::array<float, 3u> normal{};
    float distance{};
};
static_assert(sizeof(SceneEvfTriangle) == 28u);

using SceneEvfVec3 = std::array<float, 3u>;

class ISceneEvfIntersectionRuntime {
public:
    virtual ~ISceneEvfIntersectionRuntime() = default;
    virtual void CalculatePixelRay(std::uint32_t camera, std::uint32_t pixel,
                                   SceneEvfVec3& start,
                                   SceneEvfVec3& end) = 0;
    virtual void TransformCameraRay(std::uint32_t camera,
                                    SceneEvfVec3& start,
                                    SceneEvfVec3& end) = 0;
    [[nodiscard]] virtual std::array<std::uint32_t, 3u> ReadTriangleIndices(
        std::uint32_t triangle_buffer, std::uint32_t index) = 0;
    [[nodiscard]] virtual SceneEvfVec3 ReadPropertyPosition(
        std::uint32_t property_buffer, std::uint32_t index) = 0;
    virtual void TransformTriangle(std::uint32_t camera,
                                   std::array<SceneEvfVec3, 3u>& points) = 0;
    [[nodiscard]] virtual bool IntersectLine(
        const SceneEvfVec3& start, const SceneEvfVec3& end,
        const std::array<SceneEvfVec3, 3u>& points,
        SceneEvfVec3& hit) = 0;
    [[nodiscard]] virtual float Length(const SceneEvfVec3& vector) = 0;
};

class ISceneEvfColorRuntime {
public:
    virtual ~ISceneEvfColorRuntime() = default;
    virtual void WriteVertexColor(std::uint32_t vertex_buffer,
                                  std::uint32_t index,
                                  std::uint32_t argb) = 0;
};

class ISceneEvfAtomicRuntime {
public:
    virtual ~ISceneEvfAtomicRuntime() = default;
    [[nodiscard]] virtual std::uint32_t IdentityRenderCallback() = 0;
    [[nodiscard]] virtual std::uint32_t DefaultRenderCallback() = 0;
    virtual void WriteCallback(std::uint32_t atomic,
                               std::uint32_t byte_offset,
                               std::uint32_t callback) = 0;
};

class ISceneEvfGeometryRuntime {
public:
    virtual ~ISceneEvfGeometryRuntime() = default;
    virtual void Free(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    [[nodiscard]] virtual std::vector<EffectPropertyState> ReadProperties(
        std::uint32_t begin, std::uint32_t count) = 0;
    [[nodiscard]] virtual std::array<std::uint8_t, 4u> ReadColor(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual float PlaneFromPoints(
        const EffectPropertyState& first,
        const EffectPropertyState& second,
        const EffectPropertyState& third,
        std::array<float, 3u>& normal) = 0;
    virtual void WriteIndices(std::uint32_t address,
                              const std::vector<std::uint16_t>& indices) = 0;
    virtual void WriteVertices(std::uint32_t address,
                               const std::vector<SceneEvfVertex>& vertices) = 0;
    virtual void WriteTriangles(
        std::uint32_t address,
        const std::vector<SceneEvfTriangle>& triangles) = 0;
    virtual void WarnInsufficientVertices() = 0;
    virtual void WarnInsufficientPolygonVertices() = 0;
    virtual void WarnInsufficientVolumeVertices() = 0;
    virtual void ClearPropertyRange(std::uint32_t begin,
                                    std::uint32_t end) = 0;
};

class ISceneEvfPropertyBufferRuntime {
public:
    virtual ~ISceneEvfPropertyBufferRuntime() = default;
    [[nodiscard]] virtual EffectPropertyState& PropertyAt(
        std::uint32_t address) = 0;
    virtual void FreePropertyBuffer(std::uint32_t address) = 0;
};

class ISceneEvfStreamRuntime : public ISceneEvfPropertyBufferRuntime {
public:
    virtual ~ISceneEvfStreamRuntime() = default;
    virtual void ReadName(std::uint32_t stream, SceneEvfRecordState& state,
                          std::uint32_t byte_offset) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t stream) = 0;
    virtual void ReadEffectFields(std::uint32_t stream,
                                  SceneEvfRecordState& state) = 0;
    virtual void ReadProperty(std::uint32_t stream,
                              EffectPropertyState& property) = 0;
    virtual void WriteProperty(std::uint32_t address,
                               const EffectPropertyState& property) = 0;
    [[nodiscard]] virtual std::uint32_t AllocatePropertyBytes(
        std::uint32_t bytes) = 0;
    [[nodiscard]] virtual bool FindClumpChunk(std::uint32_t stream,
                                              std::uint32_t chunk_id) = 0;
    [[nodiscard]] virtual std::uint32_t ClumpWorld(std::uint32_t clump) = 0;
    virtual void RemoveClump(std::uint32_t world,
                             std::uint32_t clump) = 0;
    virtual void DestroyClump(std::uint32_t clump) = 0;
    [[nodiscard]] virtual std::uint32_t ReadClump(std::uint32_t stream) = 0;
    [[nodiscard]] virtual std::vector<std::uint32_t> Atomics(
        std::uint32_t clump) = 0;
};

class SceneEvfRecord final {
public:
    // IDA 0x64bd30; base_address represents the original 32-bit object address.
    [[nodiscard]] static SceneEvfRecordState& Initialize(
        SceneEvfRecordState& state, std::uint32_t base_address,
        std::uint32_t owner, ISceneEvfRecordRuntime& runtime);
    // IDA 0x64be80: destroy resources in reverse construction order.
    [[nodiscard]] static int Cleanup(
        SceneEvfRecordState& state,
        ISceneEvfRecordCleanupRuntime& runtime);
    // IDA 0x64e8d0: clean the four embedded strings in reverse order.
    [[nodiscard]] static int CleanupEmbeddedStrings(
        SceneEvfRecordState& state,
        ISceneEvfRecordCleanupRuntime& runtime);
    // IDA 0x64e3f0: unlink and free every node from the sentinel list.
    static void ClearLinkedList(SceneEvfRecordState& state,
                                ISceneEvfListRuntime& runtime);
    // IDA 0x64e440: install the EVF atomic render callback at +72.
    [[nodiscard]] static std::uint32_t SetAtomicRenderCallback(
        std::uint32_t atomic, ISceneEvfAtomicRuntime& runtime);
    // IDA 0x64c080: assign a C string to the field at +104.
    [[nodiscard]] static std::uint32_t SetName(
        SceneEvfRecordState& state, const char* name,
        ISceneEvfRecordStringRuntime& runtime);
    // IDA 0x64c0b0: replace type object and its inline colour.
    [[nodiscard]] static std::uint8_t SetTypeAndColor(
        SceneEvfRecordState& state, std::uint32_t base_address,
        std::uint32_t type, ISceneEvfRecordTypeRuntime& runtime);
    // IDA 0x64c1f0: generate paired-strip vertices, triangles and planes.
    [[nodiscard]] static bool GenerateStripGeometry(
        SceneEvfRecordState& state, bool double_sided, bool closed,
        ISceneEvfGeometryRuntime& runtime);
    // IDA 0x64c960: generate a two-sided polygon from the same records.
    [[nodiscard]] static bool GeneratePolygonGeometry(
        SceneEvfRecordState& state, ISceneEvfGeometryRuntime& runtime);
    // IDA 0x64d080: generate a closed strip with two-sided end caps.
    [[nodiscard]] static bool GenerateVolumeGeometry(
        SceneEvfRecordState& state, ISceneEvfGeometryRuntime& runtime);
    // IDA 0x64e170: return the indexed 80-byte property record address.
    [[nodiscard]] static std::uint32_t PropertyAddress(
        const SceneEvfRecordState& state, std::uint32_t index);
    // IDA 0x64e1b0: select the nearest ray/triangle intersection.
    [[nodiscard]] static bool IntersectsPixel(
        const SceneEvfRecordState& state, std::uint32_t camera,
        std::uint32_t pixel, float* distance,
        ISceneEvfIntersectionRuntime& runtime);
    // IDA 0x64e370: select an inline color and update generated vertices.
    [[nodiscard]] static std::uint32_t UpdateVertexColors(
        SceneEvfRecordState& state, std::uint32_t base_address,
        bool alternate, ISceneEvfColorRuntime& runtime);
    // IDA 0x64e460: read an EVF record, optional clump and generated geometry.
    [[nodiscard]] static bool ReadFromStream(
        SceneEvfRecordState& state, std::uint32_t base_address,
        std::uint32_t stream, ISceneEvfStreamRuntime& stream_runtime,
        ISceneEvfRecordTypeRuntime& type_runtime,
        ISceneEvfGeometryRuntime& geometry_runtime,
        ISceneEvfAtomicRuntime& atomic_runtime);
    // IDA 0x64e830: destroy and release the 80-byte property buffer.
    [[nodiscard]] static int CleanupPropertyBuffer(
        SceneEvfRecordState& state,
        ISceneEvfPropertyBufferRuntime& runtime);
    // IDA 0x64e870: signed count of 80-byte records in the active range.
    [[nodiscard]] static std::int32_t PropertyBufferSize(
        const SceneEvfRecordState& state);
    // IDA 0x64e8a0: clear every 80-byte property in a half-open range.
    static void ClearPropertyRange(std::uint32_t begin, std::uint32_t end,
                                   ISceneEvfPropertyBufferRuntime& runtime);
    // IDA 0x64e940: copy a property range forward and return destination end.
    [[nodiscard]] static std::uint32_t CopyPropertyRange(
        std::uint32_t begin, std::uint32_t end,
        std::uint32_t destination, ISceneEvfStreamRuntime& runtime);
    // IDA 0x64e980: fill a property range with one 80-byte record.
    [[nodiscard]] static std::uint32_t FillPropertyRange(
        std::uint32_t destination, std::uint32_t count,
        const EffectPropertyState& property, ISceneEvfStreamRuntime& runtime);
    // IDA 0x64e9b0: copy forward without a null-destination check.
    [[nodiscard]] static std::uint32_t CopyPropertyRangeUnchecked(
        std::uint32_t begin, std::uint32_t end,
        std::uint32_t destination, ISceneEvfStreamRuntime& runtime);
};

}  // namespace pal4
