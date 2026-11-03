#pragma once

#include <array>
#include <cstdint>
#include <string_view>

namespace pal4 {

struct EffectCameraObjectState final {
    std::array<std::uint32_t, 48u> words{};
};

static_assert(sizeof(EffectCameraObjectState) == 0xc0);

class IEffectCameraObjectRuntime {
public:
    virtual ~IEffectCameraObjectRuntime() = default;
    virtual void ResetBase(EffectCameraObjectState& state) = 0;
    virtual void InitializeXml(EffectCameraObjectState& state,
                               std::uint32_t byte_offset) = 0;
    virtual void DestroyXml(EffectCameraObjectState& state,
                            std::uint32_t byte_offset) = 0;
    virtual void FinalizeField64(EffectCameraObjectState& state,
                                 std::uint32_t byte_offset,
                                 std::uint32_t flag) = 0;
    virtual void FinalizeBase(EffectCameraObjectState& state) = 0;
    [[nodiscard]] virtual std::uint32_t SystemInstance() = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint8_t StartCameraFacet(
        EffectCameraObjectState& state, std::uint32_t facet_offset) = 0;
};

class EffectCameraObjectRuntimeStub final
    : public IEffectCameraObjectRuntime {
public:
    void ResetBase(EffectCameraObjectState&) override {}
    void InitializeXml(EffectCameraObjectState&, std::uint32_t) override {}
    void DestroyXml(EffectCameraObjectState&, std::uint32_t) override {}
    void FinalizeField64(EffectCameraObjectState&, std::uint32_t,
                         std::uint32_t) override {}
    void FinalizeBase(EffectCameraObjectState&) override {}
    [[nodiscard]] std::uint32_t SystemInstance() override { return 0u; }
    [[nodiscard]] std::uint32_t ReadWord(std::uint32_t) override { return 0u; }
    void WriteWord(std::uint32_t, std::uint32_t) override {}
    [[nodiscard]] std::uint8_t StartCameraFacet(
        EffectCameraObjectState&, std::uint32_t) override { return 0u; }
};

class EffectCameraObject final {
public:
    // IDA 0x606bf0. The native stack byte stored at +64 is explicit.
    static EffectCameraObjectState& Construct(
        EffectCameraObjectState& state, std::uint8_t initial_byte,
        IEffectCameraObjectRuntime& runtime);
    // IDA 0x606d10.
    static void Destroy(EffectCameraObjectState& state,
                        IEffectCameraObjectRuntime& runtime);
    // IDA 0x607460/0x607470.
    [[nodiscard]] static std::uint32_t CachedWord124(
        const EffectCameraObjectState& state);
    [[nodiscard]] static std::string_view ClassName();
    // IDA 0x607480. Starts facet +24 only on a new nonzero flag.
    [[nodiscard]] static std::uint8_t SetStartTaskFlag(
        EffectCameraObjectState& state, std::uint8_t value,
        IEffectCameraObjectRuntime& runtime);
    // IDA 0x607f70..0x607fc0, through the owner facet at +24.
    [[nodiscard]] static std::uint8_t SetLookAtPosition(
        EffectCameraObjectState& state,
        const std::array<std::uint32_t, 3>& position);
    // IDA 0x6081d0. Stores three raw rotation words at +88..+96.
    [[nodiscard]] static std::uint8_t SetRotation(
        EffectCameraObjectState& state,
        const std::array<std::uint32_t, 3>& rotation);
    [[nodiscard]] static std::uint8_t InheritCurrentFlag(
        const EffectCameraObjectState& state);
    [[nodiscard]] static std::uint8_t LockLookAtFlag(
        const EffectCameraObjectState& state);
    [[nodiscard]] static std::uint8_t SetLockLookAtFlag(
        EffectCameraObjectState& state, std::uint8_t value);
};

}  // namespace pal4
