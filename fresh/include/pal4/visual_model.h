#pragma once

#include <cstddef>
#include <cstdint>

namespace pal4 {

struct VisualModelVector {
    float x{};
    float y{};
    float z{};
};

struct VisualModelState {
    std::uint8_t active{};
    std::uint8_t property{};
    std::uint8_t reserved[2]{};
    std::uint32_t clump{};
    std::uint32_t frame{};
    VisualModelVector rotation{};
    VisualModelVector scale{};
    VisualModelVector position{};
    std::uint32_t material_flags{};
};
static_assert(sizeof(VisualModelState) == 52u);
static_assert(offsetof(VisualModelState, clump) == 4u);
static_assert(offsetof(VisualModelState, frame) == 8u);
static_assert(offsetof(VisualModelState, rotation) == 12u);
static_assert(offsetof(VisualModelState, scale) == 24u);
static_assert(offsetof(VisualModelState, position) == 36u);
static_assert(offsetof(VisualModelState, material_flags) == 48u);

class IVisualModelRuntime {
public:
    virtual ~IVisualModelRuntime() = default;
    virtual void SetFrameIdentity(std::uint32_t frame) = 0;
    virtual void ScaleFrame(std::uint32_t frame,
                            VisualModelVector scale,
                            std::uint32_t mode) = 0;
    virtual void RotateFrame(std::uint32_t frame,
                             std::uint32_t axis, float angle,
                             std::uint32_t mode) = 0;
    virtual void TranslateFrame(std::uint32_t frame,
                                VisualModelVector position,
                                std::uint32_t mode) = 0;
    [[nodiscard]] virtual int UpdateFrameObjects(
        std::uint32_t frame) = 0;
    [[nodiscard]] virtual int SetMaterialFlagsForAllAtomics(
        std::uint32_t clump, std::uint32_t flags) = 0;
};

class IVisualModelDestroyRuntime {
public:
    virtual ~IVisualModelDestroyRuntime() = default;
    [[nodiscard]] virtual std::uint32_t FindClumpWorld(
        std::uint32_t clump) = 0;
    virtual void RemoveClumpFromWorld(std::uint32_t world,
                                      std::uint32_t clump) = 0;
    virtual void ClearClumpFrameLink(std::uint32_t clump) = 0;
    virtual void DestroyClump(std::uint32_t clump) = 0;
    virtual void DestroyFrame(std::uint32_t frame) = 0;
};

class IVisualModelRenderRuntime {
public:
    virtual ~IVisualModelRenderRuntime() = default;
    [[nodiscard]] virtual std::uint32_t GetRenderState(
        std::uint32_t state) = 0;
    virtual void SetRenderState(std::uint32_t state,
                                std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t GetDeviceState(
        std::uint32_t state) = 0;
    virtual void SetDeviceState(std::uint32_t state,
                                std::uint32_t value) = 0;
    virtual void RenderClump(std::uint32_t clump) = 0;
    virtual void SetTextureStage(std::uint32_t first,
                                 std::uint32_t second,
                                 std::uint32_t stage) = 0;
    [[nodiscard]] virtual int ClearStreamSource(
        std::uint32_t stream) = 0;
};

class VisualModel final {
public:
    // IDA 0x643940/0x643990/0x6439e0.
    [[nodiscard]] static int SetRotation(
        VisualModelState& model, VisualModelVector value,
        bool additive, IVisualModelRuntime& runtime);
    [[nodiscard]] static int SetScale(
        VisualModelState& model, VisualModelVector value,
        bool additive, IVisualModelRuntime& runtime);
    [[nodiscard]] static int SetPosition(
        VisualModelState& model, VisualModelVector value,
        bool additive, IVisualModelRuntime& runtime);
    // IDA 0x643a30.
    [[nodiscard]] static int SetMaterialFlags(
        VisualModelState& model, std::uint32_t flags,
        IVisualModelRuntime& runtime);
    // IDA 0x644680 and 0x644850.
    static void Destroy(VisualModelState& model,
                        IVisualModelDestroyRuntime& runtime);
    [[nodiscard]] static int UpdateFrameTransform(
        const VisualModelState& model, IVisualModelRuntime& runtime);
    // IDA 0x6446f0: render with temporary D3D states.
    [[nodiscard]] static int Render(
        const VisualModelState& model,
        std::uint32_t first, std::uint32_t second,
        IVisualModelRenderRuntime& runtime);

private:
    [[nodiscard]] static int SetVector(
        VisualModelState& model, VisualModelVector& destination,
        VisualModelVector value, bool additive,
        IVisualModelRuntime& runtime);
};

}  // namespace pal4
