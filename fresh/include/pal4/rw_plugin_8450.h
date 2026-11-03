#pragma once

#include <array>
#include <cstdint>
#include <optional>

namespace pal4 {

enum class RwPlugin8450Callback {
    EngineConstruct,
    EngineDestruct,
    AtomicConstruct,
    AtomicDestruct,
    AtomicCopy,
    StreamReadIdentity,
    StreamWriteIdentity,
    StreamSize,
    ProcessAtomic,
    ResetAtomic,
    QueryAtomic,
};

struct RwPlugin8450State final {
    std::int32_t engine_offset = -1;
    std::int32_t atomic_offset = -1;
};

class IRwPlugin8450Lifecycle {
public:
    virtual ~IRwPlugin8450Lifecycle() = default;
    [[nodiscard]] virtual std::int32_t RegisterEngine(
        std::uint32_t size, std::uint32_t id,
        RwPlugin8450Callback construct,
        RwPlugin8450Callback destruct) = 0;
    [[nodiscard]] virtual std::int32_t RegisterAtomic(
        std::uint32_t size, std::uint32_t id,
        RwPlugin8450Callback construct,
        RwPlugin8450Callback destruct,
        RwPlugin8450Callback copy) = 0;
    [[nodiscard]] virtual std::int32_t RegisterAtomicStream(
        std::uint32_t id, RwPlugin8450Callback read,
        RwPlugin8450Callback write,
        RwPlugin8450Callback size) = 0;
    [[nodiscard]] virtual std::uint32_t EngineBase() = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
};

// Rendering services remain explicit until their native boundaries are reviewed.
class IRwPlugin8450Runtime : public IRwPlugin8450Lifecycle {
public:
    [[nodiscard]] virtual std::uint32_t RenderCallbackAddress() = 0;
    [[nodiscard]] virtual std::uint32_t DefaultRenderCallbackAddress() = 0;
    [[nodiscard]] virtual std::uint32_t ForAllAtomics(
        std::uint32_t clump, RwPlugin8450Callback callback,
        std::uint32_t user_data) = 0;
    virtual void ForAllAtomicsWithOutput(
        std::uint32_t clump, RwPlugin8450Callback callback,
        std::uint32_t& output) = 0;
    virtual void ResyncAtomic(std::uint32_t atomic) = 0;
    [[nodiscard]] virtual std::uint32_t ObjectTransform(
        std::uint32_t object) = 0;
    [[nodiscard]] virtual std::array<float, 3> CalculatePosition(
        std::uint32_t center, std::uint32_t transform) = 0;
    [[nodiscard]] virtual double ApproximateMagnitude(
        const std::array<float, 3>& delta) = 0;
    [[nodiscard]] virtual std::int32_t FloatToLong(double value) = 0;
    virtual void InvokeCallback(std::uint32_t callback,
                                std::uint32_t atomic) = 0;
};

class RwPlugin8450 final {
public:
    [[nodiscard]] static bool Register(RwPlugin8450State& state,
                                       IRwPlugin8450Lifecycle& runtime);
    [[nodiscard]] static std::uint32_t EngineConstruct(
        std::uint32_t object, const RwPlugin8450State& state,
        IRwPlugin8450Lifecycle& runtime);
    [[nodiscard]] static std::uint32_t EngineDestruct(
        std::uint32_t object, const RwPlugin8450State& state,
        IRwPlugin8450Lifecycle& runtime);
    [[nodiscard]] static std::uint32_t AtomicConstruct(
        std::uint32_t object, const RwPlugin8450State& state,
        IRwPlugin8450Lifecycle& runtime);
    [[nodiscard]] static std::uint32_t AtomicDestruct(
        std::uint32_t object, const RwPlugin8450State& state,
        IRwPlugin8450Lifecycle& runtime);
    [[nodiscard]] static std::uint32_t AtomicCopy(
        std::uint32_t target, std::uint32_t source,
        const RwPlugin8450State& state, IRwPlugin8450Lifecycle& runtime);
    [[nodiscard]] static std::uint32_t ProcessClump(
        std::uint32_t clump, IRwPlugin8450Runtime& runtime);
    [[nodiscard]] static std::uint32_t ProcessAtomic(
        std::uint32_t atomic, const RwPlugin8450State& state,
        IRwPlugin8450Runtime& runtime);
    [[nodiscard]] static std::optional<std::uint32_t> SetPluginData(
        std::uint32_t atomic, std::uint32_t callback,
        const RwPlugin8450State& state, IRwPlugin8450Runtime& runtime);
    [[nodiscard]] static std::int32_t SetMaterialColor(
        std::uint32_t atomic, std::uint32_t color,
        IRwPlugin8450Runtime& runtime);
    [[nodiscard]] static std::uint32_t RenderAtomic(
        std::uint32_t atomic, const RwPlugin8450State& state,
        IRwPlugin8450Runtime& runtime);
    [[nodiscard]] static std::uint32_t GetPluginData(
        std::uint32_t atomic, const RwPlugin8450State& state,
        IRwPlugin8450Runtime& runtime);
    [[nodiscard]] static double CalculateDistanceToCamera(
        std::uint32_t atomic, IRwPlugin8450Runtime& runtime);
    [[nodiscard]] static std::uint32_t ResetClump(
        std::uint32_t clump, IRwPlugin8450Runtime& runtime);
    [[nodiscard]] static std::uint32_t ResetAtomic(
        std::uint32_t atomic, const RwPlugin8450State& state,
        IRwPlugin8450Runtime& runtime);
    [[nodiscard]] static std::uint32_t QueryClump(
        std::uint32_t clump, IRwPlugin8450Runtime& runtime);
    [[nodiscard]] static std::uint32_t QueryAtomic(
        std::uint32_t atomic, std::uint32_t& output,
        const RwPlugin8450State& state, IRwPlugin8450Runtime& runtime);
    [[nodiscard]] static std::uint32_t QueryDirect(
        std::uint32_t object, const RwPlugin8450State& state,
        IRwPlugin8450Runtime& runtime);
    [[nodiscard]] static std::uint32_t SetEngineFloat(
        float value, const RwPlugin8450State& state,
        IRwPlugin8450Runtime& runtime);
    [[nodiscard]] static std::uint32_t SetEngineInt(
        std::uint32_t value, const RwPlugin8450State& state,
        IRwPlugin8450Runtime& runtime);
};

}  // namespace pal4
