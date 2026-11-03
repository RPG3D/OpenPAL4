#pragma once

#include "pal4/rtk_prt_std_atomic_object.h"

#include <cstdint>

namespace pal4 {

class IRtkPrtStdAtomicDrawRuntime {
public:
    virtual ~IRtkPrtStdAtomicDrawRuntime() = default;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint8_t HandleStateChange(
        std::uint32_t resource, std::uint32_t context) = 0;
    [[nodiscard]] virtual std::uint32_t GetObjectTransform(std::uint32_t object) = 0;
    [[nodiscard]] virtual std::uint32_t EffectManager() = 0;
    [[nodiscard]] virtual std::uint8_t ValidateTransform(
        std::uint32_t manager, std::uint32_t context, std::uint32_t transform) = 0;
};

class IRtkPrtStdAtomicResetRuntime {
public:
    virtual ~IRtkPrtStdAtomicResetRuntime() = default;
    virtual void ResetPlaybackFacet(std::uint32_t facet) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    virtual void WriteFloat(std::uint32_t address, float value) = 0;
    virtual void UpdateFrame(std::uint32_t resource) = 0;
    virtual void DeactivateResource(std::uint32_t resource) = 0;
};

class IRtkPrtStdAtomicReleaseRuntime {
public:
    virtual ~IRtkPrtStdAtomicReleaseRuntime() = default;
    virtual void DestroyResource(std::uint32_t resource) = 0;
    virtual void FreeResource(std::uint32_t resource) = 0;
    virtual void ProcessEvent(std::uint32_t system, std::uint32_t owner) = 0;
};

class IRtkPrtStdAtomicControlRuntime : public IRtkPrtStdAtomicUpdateRuntime,
                                     public IRtkPrtStdAtomicDrawRuntime,
                                     public IRtkPrtStdAtomicResetRuntime,
                                     public IRtkPrtStdAtomicReleaseRuntime {
public:
    virtual ~IRtkPrtStdAtomicControlRuntime() = default;
    virtual void DestroyResource(std::uint32_t resource) = 0;
    virtual void FreeResource(std::uint32_t resource) = 0;
    virtual void ProcessEvent(std::uint32_t system, std::uint32_t owner) = 0;
    [[nodiscard]] virtual std::uint32_t DeleteObject(std::uint32_t owner,
                                                      std::uint32_t flags) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint8_t HandleStateChange(
        std::uint32_t resource, std::uint32_t context) = 0;
    [[nodiscard]] virtual std::uint32_t GetObjectTransform(
        std::uint32_t object) = 0;
    [[nodiscard]] virtual std::uint32_t EffectManager() = 0;
    [[nodiscard]] virtual std::uint8_t ValidateTransform(
        std::uint32_t manager, std::uint32_t context,
        std::uint32_t transform) = 0;
};

class RtkPrtStdAtomicControlRuntimeStub final
    : public IRtkPrtStdAtomicControlRuntime {
public:
    void ResetPlaybackFacet(std::uint32_t) override {}
    void WriteWord(std::uint32_t, std::uint32_t) override {}
    void WriteFloat(std::uint32_t, float) override {}
    void UpdateFrame(std::uint32_t) override {}
    void DeactivateResource(std::uint32_t) override {}
    void DestroyResource(std::uint32_t) override {}
    void FreeResource(std::uint32_t) override {}
    void ProcessEvent(std::uint32_t, std::uint32_t) override {}
    [[nodiscard]] std::uint32_t DeleteObject(std::uint32_t,
                                              std::uint32_t) override { return 0u; }
    [[nodiscard]] std::uint8_t ReadByte(std::uint32_t) override { return 0u; }
    [[nodiscard]] std::uint32_t ReadWord(std::uint32_t) override { return 0u; }
    [[nodiscard]] std::uint8_t UpdateState(std::uint32_t, double,
        std::uint32_t, std::uint8_t) override { return 0u; }
    [[nodiscard]] std::uint8_t HandleStateChange(std::uint32_t,
                                                  std::uint32_t) override { return 0u; }
    [[nodiscard]] std::uint32_t GetObjectTransform(std::uint32_t) override { return 0u; }
    [[nodiscard]] std::uint32_t EffectManager() override { return 0u; }
    [[nodiscard]] std::uint8_t ValidateTransform(std::uint32_t,
                                                  std::uint32_t,
                                                  std::uint32_t) override { return 0u; }
};

class RtkPrtStdAtomicControl final {
public:
    // IDA 0x6140b0, called on the owner.
    [[nodiscard]] static std::uint8_t Reset(
        RtkPrtStdAtomicObjectState& state, std::uint32_t owner,
        IRtkPrtStdAtomicResetRuntime& runtime);
    // IDA 0x6141d0; native deletion is unconditional.
    // Shared resource/queue release phase, for native scoped owners whose
    // storage is deleted by the factory immediately after this returns.
    static void ReleasePlayback(RtkPrtStdAtomicObjectState& state,
        std::uint32_t owner, IRtkPrtStdAtomicReleaseRuntime& runtime);
    [[nodiscard]] static std::uint32_t Stop(
        RtkPrtStdAtomicObjectState& state, std::uint32_t owner,
        IRtkPrtStdAtomicControlRuntime& runtime);
    // IDA 0x614260.
    [[nodiscard]] static std::uint8_t Advance(
        const RtkPrtStdAtomicObjectState& state, double elapsed,
        IRtkPrtStdAtomicUpdateRuntime& runtime);
    // IDA 0x614290.
    [[nodiscard]] static std::uint8_t ActivatePending(
        const RtkPrtStdAtomicObjectState& state, std::uint32_t context,
        IRtkPrtStdAtomicDrawRuntime& runtime);
    // IDA 0x614320 / 0x614340, entered through facet +20.
    [[nodiscard]] static std::uint8_t EnablePlayback(
        RtkPrtStdAtomicObjectState& state);
    // Shared614320 only accesses the base status word, including SequenceAnim.
    [[nodiscard]] static std::uint8_t EnablePlaybackStatus(std::uint32_t& status);
    [[nodiscard]] static std::uint8_t DisablePlayback(
        RtkPrtStdAtomicObjectState& state);
};

}  // namespace pal4
