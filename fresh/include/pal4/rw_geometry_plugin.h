#pragma once

#include <cstdint>
#include <optional>

namespace pal4 {

struct RwGeometryPluginState final {
    std::int32_t engine_offset = -1;
    std::int32_t geometry_offset = -1;
};

struct RwAtomicProcessContext final {
    std::uint32_t material_match = 0;
    std::uint32_t geometry_callback = 0x65fab0u;
    std::uint32_t payload = 0;
};

struct RwPluginVector3 final {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
};

class IRwGeometryPluginLifecycle {
public:
    virtual ~IRwGeometryPluginLifecycle() = default;
    [[nodiscard]] virtual std::int32_t RegisterEnginePlugin(
        std::uint32_t bytes, std::uint32_t plugin_id) = 0;
    [[nodiscard]] virtual std::int32_t RegisterGeometryPlugin(
        std::uint32_t bytes, std::uint32_t plugin_id) = 0;
    [[nodiscard]] virtual std::int32_t RegisterGeometryStream(
        std::uint32_t plugin_id) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
};

class IRwGeometryPluginRuntime : public IRwGeometryPluginLifecycle {
public:
    ~IRwGeometryPluginRuntime() override = default;
    virtual void ForAllAtomics(std::uint32_t clump_address,
                               std::uint32_t atomic_callback,
                               RwAtomicProcessContext* context) = 0;
    virtual void ResyncInterpolatedSphere(std::uint32_t atomic_address) = 0;
    [[nodiscard]] virtual std::uint32_t GetObjectTransform(
        std::uint32_t frame_address) = 0;
    [[nodiscard]] virtual RwPluginVector3 CalculatePosition(
        std::uint32_t sphere_address, std::uint32_t transform_address) = 0;
    virtual void ForAllMaterials(std::uint32_t geometry_address,
                                 std::uint32_t material_callback,
                                 RwAtomicProcessContext& context) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t CreateImage(
        std::uint32_t width, std::uint32_t height, std::uint32_t depth) = 0;
    virtual void AllocateImagePixels(std::uint32_t image_address) = 0;
    virtual void SetImageFromRaster(std::uint32_t image_address,
                                    std::uint32_t raster_address) = 0;
    virtual void DestroyImage(std::uint32_t image_address) = 0;
    [[nodiscard]] virtual std::uint32_t DefaultAtomicRenderCallback() = 0;
    [[nodiscard]] virtual std::uint32_t AllocateBytes(
        std::uint32_t bytes) = 0;
    virtual void FreeBytes(std::uint32_t address) = 0;
    virtual void InvokeAtomicRenderCallback(std::uint32_t callback_address,
                                            std::uint32_t atomic_address) = 0;
    [[nodiscard]] virtual std::uint32_t GeometryRenderCallbackAddress() {
        return 0x65fab0u;
    }
    [[nodiscard]] virtual std::optional<bool> CompressedTextureHasAlpha(
        std::uint32_t) { return std::nullopt; }
    [[nodiscard]] virtual std::uint32_t ActiveCameraAddress() {
        return ReadWord(ReadWord(0x950cd0u));
    }
};

class RwGeometryPlugin final {
public:
    static constexpr std::uint32_t kPluginId = 8449u;
    [[nodiscard]] static bool Register(RwGeometryPluginState& state,
                                       IRwGeometryPluginLifecycle& runtime);
    [[nodiscard]] static std::uint32_t ConstructData(
        const RwGeometryPluginState& state, std::uint32_t object_address,
        IRwGeometryPluginLifecycle& runtime);
    [[nodiscard]] static std::uint32_t DestructData(
        const RwGeometryPluginState& state, std::uint32_t object_address,
        IRwGeometryPluginLifecycle& runtime);
    [[nodiscard]] static std::uint32_t CopyData(
        const RwGeometryPluginState& state, std::uint32_t destination,
        std::uint32_t source, IRwGeometryPluginLifecycle& runtime);
    [[nodiscard]] static std::uint32_t ProcessClumpAtomics(
        const RwGeometryPluginState& state, std::uint32_t clump_address,
        std::uint32_t payload, IRwGeometryPluginRuntime& runtime);
    [[nodiscard]] static std::uint32_t ProcessAtomicGeometry(
        const RwGeometryPluginState& state, std::uint32_t atomic_address,
        IRwGeometryPluginRuntime& runtime);
    [[nodiscard]] static std::uint32_t GetAtomicGeometryData(
        const RwGeometryPluginState& state, std::uint32_t atomic_address,
        IRwGeometryPluginRuntime& runtime);
    [[nodiscard]] static std::uint32_t GetAtomicGeometryDataArray(
        const RwGeometryPluginState& state, std::uint32_t atomic_address,
        IRwGeometryPluginRuntime& runtime);
    [[nodiscard]] static float CalculateDistanceToCamera(
        std::uint32_t atomic_address, IRwGeometryPluginRuntime& runtime);
    [[nodiscard]] static std::uint32_t ProcessAtomicMaterial(
        const RwGeometryPluginState& state, std::uint32_t atomic_address,
        RwAtomicProcessContext& context, IRwGeometryPluginRuntime& runtime);
    [[nodiscard]] static std::int32_t SetAtomicGeometryData(
        const RwGeometryPluginState& state, std::uint32_t atomic_address,
        std::uint32_t value, IRwGeometryPluginRuntime& runtime);
    [[nodiscard]] static std::int32_t SetAtomicGeometryFlag(
        const RwGeometryPluginState& state, std::uint32_t atomic_address,
        std::uint32_t value, IRwGeometryPluginRuntime& runtime);
    [[nodiscard]] static std::int32_t SetAtomicGeometryPayload(
        const RwGeometryPluginState& state, std::uint32_t atomic_address,
        std::uint32_t value, IRwGeometryPluginRuntime& runtime);
    [[nodiscard]] static std::uint32_t GetAtomicGeometryMaterial(
        std::uint32_t atomic_address, RwAtomicProcessContext& context,
        IRwGeometryPluginRuntime& runtime);
    [[nodiscard]] static bool CheckMaterialAlpha(
        std::uint32_t geometry_address, IRwGeometryPluginRuntime& runtime);
    [[nodiscard]] static std::uint32_t ProcessMaterial(
        std::uint32_t material_address, RwAtomicProcessContext& context,
        IRwGeometryPluginRuntime& runtime);
    [[nodiscard]] static bool ProcessImageAlpha(
        std::uint32_t texture_address, IRwGeometryPluginRuntime& runtime);
    [[nodiscard]] static bool ProcessClumpAtomicsWrapper(
        const RwGeometryPluginState& state, std::uint32_t clump_address,
        IRwGeometryPluginRuntime& runtime);
    [[nodiscard]] static std::uint32_t ProcessAtomic(
        const RwGeometryPluginState& state, std::uint32_t atomic_address,
        IRwGeometryPluginRuntime& runtime);
    [[nodiscard]] static std::uint32_t GetAtomicGeometryFlag(
        const RwGeometryPluginState& state, std::uint32_t atomic_address,
        IRwGeometryPluginRuntime& runtime);
    [[nodiscard]] static bool CompareAtomicGeometryFlags(
        std::uint32_t first_record, std::uint32_t second_record,
        IRwGeometryPluginRuntime& runtime);
    [[nodiscard]] static std::int32_t SortAtomicGeometries(
        const RwGeometryPluginState& state, std::uint32_t array_address,
        IRwGeometryPluginRuntime& runtime);
    [[nodiscard]] static std::int32_t ClearAtomicGeometryArray(
        const RwGeometryPluginState& state, std::uint32_t array_address,
        IRwGeometryPluginRuntime& runtime);
    [[nodiscard]] static std::uint32_t QuickSortAtomicGeometries(
        std::uint32_t begin, std::uint32_t end,
        IRwGeometryPluginRuntime& runtime);
    [[nodiscard]] static std::uint32_t InsertionSortAtomicGeometries(
        std::uint32_t begin, std::uint32_t end,
        IRwGeometryPluginRuntime& runtime);
};

}  // namespace pal4
