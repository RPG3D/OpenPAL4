#pragma once

#include <cstdint>

namespace pal4 {

enum class RwPlugin8452Callback {
    Identity,
    GeometryConstruct,
    GeometryDestruct,
    GeometryCopy,
    StreamSize,
    ProcessAtomic,
    ResetAtomic,
    QueryAtomic,
};

struct RwPlugin8452State final {
    std::int32_t engine_offset = -1;
    std::int32_t geometry_offset = -1;
};

struct RwPlugin8452Sphere final {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    float radius = 0.0f;
};

class IRwPlugin8452Lifecycle {
public:
    virtual ~IRwPlugin8452Lifecycle() = default;
    [[nodiscard]] virtual std::int32_t RegisterEngine(
        std::uint32_t size, std::uint32_t id,
        RwPlugin8452Callback construct,
        RwPlugin8452Callback destruct) = 0;
    [[nodiscard]] virtual std::int32_t RegisterGeometry(
        std::uint32_t size, std::uint32_t id,
        RwPlugin8452Callback construct,
        RwPlugin8452Callback destruct,
        RwPlugin8452Callback copy) = 0;
    [[nodiscard]] virtual std::int32_t RegisterGeometryStream(
        std::uint32_t id, RwPlugin8452Callback read,
        RwPlugin8452Callback write,
        RwPlugin8452Callback size) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
};

class IRwPlugin8452Runtime : public IRwPlugin8452Lifecycle {
public:
    ~IRwPlugin8452Runtime() override = default;
    [[nodiscard]] virtual std::uint32_t ForAllAtomics(
        std::uint32_t clump, RwPlugin8452Callback callback) = 0;
    virtual void ForAllAtomicsQuery(
        std::uint32_t clump, RwPlugin8452Callback callback,
        std::uint32_t& result) = 0;
    [[nodiscard]] virtual std::uint32_t CustomRenderCallback() = 0;
    [[nodiscard]] virtual std::uint32_t DefaultRenderCallback() = 0;
    [[nodiscard]] virtual std::uint32_t ActiveCamera() = 0;
    [[nodiscard]] virtual std::uint32_t GetObjectTransform(
        std::uint32_t frame) = 0;
    virtual void ResyncAtomicSphere(std::uint32_t atomic) = 0;
    virtual void CalculateSphere(RwPlugin8452Sphere& sphere,
                                 std::uint32_t transform) = 0;
    [[nodiscard]] virtual std::int32_t FrustumTestSphere(
        std::uint32_t camera, const RwPlugin8452Sphere& sphere) = 0;
    virtual void InvokeRender(std::uint32_t callback,
                              std::uint32_t atomic) = 0;
};

class RwPlugin8452RuntimeStub : public IRwPlugin8452Runtime {
public:
    std::int32_t RegisterEngine(
        std::uint32_t, std::uint32_t,
        RwPlugin8452Callback, RwPlugin8452Callback) override {
        return -1;
    }
    std::int32_t RegisterGeometry(
        std::uint32_t, std::uint32_t,
        RwPlugin8452Callback, RwPlugin8452Callback,
        RwPlugin8452Callback) override { return -1; }
    std::int32_t RegisterGeometryStream(
        std::uint32_t, RwPlugin8452Callback,
        RwPlugin8452Callback, RwPlugin8452Callback) override {
        return -1;
    }
    std::uint32_t ReadWord(std::uint32_t) override { return 0; }
    void WriteWord(std::uint32_t, std::uint32_t) override {}
    std::uint32_t ForAllAtomics(
        std::uint32_t, RwPlugin8452Callback) override { return 0; }
    void ForAllAtomicsQuery(
        std::uint32_t, RwPlugin8452Callback,
        std::uint32_t&) override {}
    std::uint32_t CustomRenderCallback() override { return 0; }
    std::uint32_t DefaultRenderCallback() override { return 0; }
    std::uint32_t ActiveCamera() override { return 0; }
    std::uint32_t GetObjectTransform(std::uint32_t) override {
        return 0;
    }
    void ResyncAtomicSphere(std::uint32_t) override {}
    void CalculateSphere(RwPlugin8452Sphere&,
                         std::uint32_t) override {}
    std::int32_t FrustumTestSphere(
        std::uint32_t, const RwPlugin8452Sphere&) override { return 0; }
    void InvokeRender(std::uint32_t, std::uint32_t) override {}
};

class RwPlugin8452 final {
public:
    static constexpr std::uint32_t kId = 8452u;
    static constexpr std::uint32_t kEngineOffsetAddress = 0x8c7058u;
    static constexpr std::uint32_t kGeometryOffsetAddress = 0x8c705cu;

    [[nodiscard]] static bool Register(RwPlugin8452State& state,
                                       IRwPlugin8452Lifecycle& runtime);
    [[nodiscard]] static std::uint32_t GeometryConstruct(
        std::uint32_t object, const RwPlugin8452State& state,
        IRwPlugin8452Lifecycle& runtime);
    [[nodiscard]] static std::uint32_t GeometryDestruct(
        std::uint32_t object, const RwPlugin8452State& state,
        IRwPlugin8452Lifecycle& runtime);
    [[nodiscard]] static std::uint32_t GeometryCopy(
        std::uint32_t target, std::uint32_t source,
        const RwPlugin8452State& state,
        IRwPlugin8452Lifecycle& runtime);
    [[nodiscard]] static std::uint32_t PluginSize();
    [[nodiscard]] static std::uint32_t ProcessClump(
        std::uint32_t clump, IRwPlugin8452Runtime& runtime);
    [[nodiscard]] static std::uint32_t ProcessAtomic(
        std::uint32_t atomic, const RwPlugin8452State& state,
        IRwPlugin8452Runtime& runtime);
    [[nodiscard]] static std::uint32_t SetPluginData(
        std::uint32_t atomic, std::uint32_t callback,
        const RwPlugin8452State& state,
        IRwPlugin8452Runtime& runtime);
    [[nodiscard]] static std::uint32_t RenderAtomic(
        std::uint32_t atomic, const RwPlugin8452State& state,
        IRwPlugin8452Runtime& runtime);
    [[nodiscard]] static std::uint32_t ResetClump(
        std::uint32_t clump, IRwPlugin8452Runtime& runtime);
    [[nodiscard]] static std::uint32_t ResetAtomic(
        std::uint32_t atomic, const RwPlugin8452State& state,
        IRwPlugin8452Runtime& runtime);
    [[nodiscard]] static std::uint32_t GetFrustumResult(
        std::uint32_t object, const RwPlugin8452State& state,
        IRwPlugin8452Runtime& runtime);
    [[nodiscard]] static std::uint32_t QueryClump(
        std::uint32_t clump, IRwPlugin8452Runtime& runtime);
    [[nodiscard]] static std::uint32_t QueryAtomic(
        std::uint32_t atomic, std::uint32_t& result,
        const RwPlugin8452State& state,
        IRwPlugin8452Runtime& runtime);
    [[nodiscard]] static std::uint32_t GetActiveFlag(
        std::uint32_t object, const RwPlugin8452State& state,
        IRwPlugin8452Runtime& runtime);
};

}  // namespace pal4
