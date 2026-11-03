#pragma once

#include <array>
#include <cstdint>
#include <string_view>

namespace pal4 {

struct EffectObjectState final {
    std::array<std::uint32_t, 42u> words{};
};

static_assert(sizeof(EffectObjectState) == 168u);

class IEffectObjectRuntime {
public:
    virtual ~IEffectObjectRuntime() = default;
    [[nodiscard]] virtual std::uint32_t SystemInstance() = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    virtual void FreeObject(EffectObjectState& state) = 0;
};

class EffectObjectRuntimeStub final : public IEffectObjectRuntime {
public:
    [[nodiscard]] std::uint32_t SystemInstance() override { return 0u; }
    [[nodiscard]] std::uint32_t ReadWord(
        std::uint32_t) override { return 0u; }
    void WriteWord(std::uint32_t, std::uint32_t) override {}
    void FreeObject(EffectObjectState&) override {}
};

class EffectObject final {
public:
    // IDA 0x60fab0/0x60fbc0; factory 0x63e38b allocates 0xa8 bytes.
    static EffectObjectState& Construct(EffectObjectState& state,
                                         IEffectObjectRuntime& runtime);
    static void Destroy(EffectObjectState& state,
                        IEffectObjectRuntime& runtime);
    // IDA 0x60fba0 deleting-destructor code island.
    [[nodiscard]] static std::uintptr_t DeletingDestroy(
        EffectObjectState& state, std::uint32_t flags,
        IEffectObjectRuntime& runtime);
    // IDA 0x6103d0/0x6103e0/0x610430.
    [[nodiscard]] static std::uint32_t ResourceHandle(
        const EffectObjectState& state);
    [[nodiscard]] static std::string_view ClassName();
    [[nodiscard]] static float GetDuration(const EffectObjectState& state);
};

class IEffectObjectPlaybackRuntime {
public:
    virtual ~IEffectObjectPlaybackRuntime() = default;
    [[nodiscard]] virtual double ResetAndMeasure(
        std::uint32_t resource, std::uint8_t mode) = 0;
};

class EffectObjectPlaybackRuntimeStub final
    : public IEffectObjectPlaybackRuntime {
public:
    [[nodiscard]] double ResetAndMeasure(
        std::uint32_t, std::uint8_t) override { return 0.0; }
};

class EffectObjectPlayback final {
public:
    // IDA 0x6103f0.
    static void SetEnabled(EffectObjectState& state, std::uint8_t enabled,
                           IEffectObjectPlaybackRuntime& runtime);
};

class IEffectObjectTransformRuntime {
public:
    virtual ~IEffectObjectTransformRuntime() = default;
    [[nodiscard]] virtual std::uint32_t CallFacet28Slot8(
        EffectObjectState& state,
        const std::array<std::uint32_t, 3>& values,
        std::uint32_t mode) = 0;
};

class EffectObjectTransformRuntimeStub final
    : public IEffectObjectTransformRuntime {
public:
    [[nodiscard]] std::uint32_t CallFacet28Slot8(
        EffectObjectState&, const std::array<std::uint32_t, 3>&,
        std::uint32_t) override { return 0u; }
};

class EffectObjectTransform final {
public:
    // IDA 0x60fb80; owner facet +40 forwards to facet +28 slot +8.
    [[nodiscard]] static std::uint32_t ForwardFacet28(
        EffectObjectState& state,
        const std::array<std::uint32_t, 3>& values,
        IEffectObjectTransformRuntime& runtime);
};

}  // namespace pal4
