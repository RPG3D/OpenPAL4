#pragma once

#include "pal4/effect_object.h"

#include <cstdint>

namespace pal4 {

class IEffectObjectControlRuntime {
public:
    virtual ~IEffectObjectControlRuntime() = default;
    virtual void ResetPlaybackFacet(EffectObjectState& state) = 0;
    virtual void ResetInstance(std::uint32_t instance) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    virtual void UpdateFrame(EffectObjectState& state) = 0;
    [[nodiscard]] virtual bool FinalizeInstance(std::uint32_t instance) = 0;
    virtual void CleanupInstance(std::uint32_t instance,
                                 std::uint32_t context) = 0;
    virtual void DestroyInstance(std::uint32_t instance,
                                 std::uint32_t context) = 0;
    virtual void DestroyFrame(std::uint32_t frame) = 0;
    virtual void ProcessStop(std::uint32_t system,
                             EffectObjectState& state) = 0;
    [[nodiscard]] virtual std::uint32_t DeleteObject(
        EffectObjectState& state, std::uint32_t flags) = 0;
};

class EffectObjectControlRuntimeStub final : public IEffectObjectControlRuntime {
public:
    void ResetPlaybackFacet(EffectObjectState&) override {}
    void ResetInstance(std::uint32_t) override {}
    void WriteWord(std::uint32_t, std::uint32_t) override {}
    void WriteByte(std::uint32_t, std::uint8_t) override {}
    [[nodiscard]] std::uint8_t ReadByte(std::uint32_t) override { return 0u; }
    void UpdateFrame(EffectObjectState&) override {}
    [[nodiscard]] bool FinalizeInstance(std::uint32_t) override { return false; }
    void CleanupInstance(std::uint32_t, std::uint32_t) override {}
    void DestroyInstance(std::uint32_t, std::uint32_t) override {}
    void DestroyFrame(std::uint32_t) override {}
    void ProcessStop(std::uint32_t, EffectObjectState&) override {}
    [[nodiscard]] std::uint32_t DeleteObject(
        EffectObjectState&, std::uint32_t) override { return 0u; }
};

class EffectObjectControl final {
public:
    // IDA 0x6106d0.
    [[nodiscard]] static bool Reset(EffectObjectState& state,
                                    IEffectObjectControlRuntime& runtime);
    // IDA 0x610790; context is the native EBP value.
    [[nodiscard]] static std::uint32_t Stop(
        EffectObjectState& state, std::uint32_t context,
        IEffectObjectControlRuntime& runtime);
};

}  // namespace pal4
