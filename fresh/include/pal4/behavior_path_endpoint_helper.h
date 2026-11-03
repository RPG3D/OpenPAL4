#pragma once

#include <array>
#include <cstdint>

namespace pal4 {

struct BehaviorPathGlobalsState {
    std::uint8_t cleanup_guard = 0u;
    std::uint8_t high_ecx_byte = 0u;
    std::uint32_t begin = 0u;
    std::uint32_t end = 0u;
    std::uint32_t capacity = 0u;
};

class IBehaviorPathGlobalsRuntime {
public:
    virtual ~IBehaviorPathGlobalsRuntime() = default;
    [[nodiscard]] virtual int RegisterCleanup() = 0;
    virtual void Free(std::uint32_t address) = 0;
};

class BehaviorPathGlobalsRuntimeStub final : public IBehaviorPathGlobalsRuntime {
public:
    int RegisterCleanup() override;
    void Free(std::uint32_t address) override;
};

class BehaviorPathEndpointGlobals final {
public:
    static constexpr std::uint32_t kGuardAddress = 0x8e41e4u;
    static constexpr std::uint32_t kHighByteAddress = 0x8e41e8u;
    static constexpr std::uint32_t kBeginAddress = 0x8e41ecu;

    [[nodiscard]] static int RegisterExit(
        BehaviorPathGlobalsState& state, std::uint32_t ecx_value,
        IBehaviorPathGlobalsRuntime& runtime);
    [[nodiscard]] static int Initialize(
        BehaviorPathGlobalsState& state, std::uint32_t ecx_value);
    static void Cleanup(BehaviorPathGlobalsState& state,
                        IBehaviorPathGlobalsRuntime& runtime);
};

struct BehaviorPathEndpointState {
    std::array<std::uint32_t, 56> words{};
};
static_assert(sizeof(BehaviorPathEndpointState) == 0xe0u);

using BehaviorVector3 = std::array<float, 3>;
using BehaviorContext3 = std::array<std::uint32_t, 3>;

class IBehaviorPathInitializationRuntime {
public:
    virtual ~IBehaviorPathInitializationRuntime() = default;
    [[nodiscard]] virtual BehaviorVector3 ReadGlobalVector(
        std::uint32_t address) = 0;
    virtual void CallVoidSlot(BehaviorPathEndpointState& state,
                              std::uint32_t slot) = 0;
    virtual void CallScalarSlot(BehaviorPathEndpointState& state,
                                std::uint32_t slot,
                                std::uint32_t value_bits) = 0;
    virtual void CallVectorSlot0C(BehaviorPathEndpointState& state,
                                   BehaviorVector3& output,
                                   const BehaviorVector3& input) = 0;
    virtual void CallVectorSlot14(BehaviorPathEndpointState& state,
                                   BehaviorContext3& output,
                                   const BehaviorVector3& input) = 0;
    [[nodiscard]] virtual std::uint32_t CallContextSlot10(
        BehaviorPathEndpointState& state,
        BehaviorContext3& context) = 0;
    [[nodiscard]] virtual BehaviorVector3 CallTokenSlot48(
        BehaviorPathEndpointState& state, BehaviorVector3& output,
        std::uint32_t token) = 0;
    virtual void CallVectorSlot04(BehaviorPathEndpointState& state,
                                   BehaviorVector3& output,
                                   const BehaviorVector3& input) = 0;
    [[nodiscard]] virtual std::uint32_t Rand() = 0;
    [[nodiscard]] virtual std::uint32_t Read32(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t CallContextSlot18(
        BehaviorPathEndpointState& state,
        BehaviorContext3& context) = 0;
    [[nodiscard]] virtual std::uint32_t CallTargetSlot04(
        std::uint32_t captured_vtable, std::uint32_t target,
        std::uint32_t value) = 0;
};

class BehaviorPathInitializationRuntimeStub final
    : public IBehaviorPathInitializationRuntime {
public:
    BehaviorVector3 ReadGlobalVector(std::uint32_t address) override;
    void CallVoidSlot(BehaviorPathEndpointState& state,
                      std::uint32_t slot) override;
    void CallScalarSlot(BehaviorPathEndpointState& state,
                        std::uint32_t slot,
                        std::uint32_t value_bits) override;
    void CallVectorSlot0C(BehaviorPathEndpointState& state,
                           BehaviorVector3& output,
                           const BehaviorVector3& input) override;
    void CallVectorSlot14(BehaviorPathEndpointState& state,
                           BehaviorContext3& output,
                           const BehaviorVector3& input) override;
    std::uint32_t CallContextSlot10(
        BehaviorPathEndpointState& state,
        BehaviorContext3& context) override;
    BehaviorVector3 CallTokenSlot48(
        BehaviorPathEndpointState& state, BehaviorVector3& output,
        std::uint32_t token) override;
    void CallVectorSlot04(BehaviorPathEndpointState& state,
                           BehaviorVector3& output,
                           const BehaviorVector3& input) override;
    std::uint32_t Rand() override;
    std::uint32_t Read32(std::uint32_t address) override;
    std::uint32_t CallContextSlot18(
        BehaviorPathEndpointState& state,
        BehaviorContext3& context) override;
    std::uint32_t CallTargetSlot04(
        std::uint32_t captured_vtable, std::uint32_t target,
        std::uint32_t value) override;
};

class IBehaviorPathEndpointLifecycleRuntime {
public:
    virtual ~IBehaviorPathEndpointLifecycleRuntime() = default;
    virtual void ConstructBase(BehaviorPathEndpointState& state) = 0;
    virtual void DestructBase(BehaviorPathEndpointState& state) = 0;
    virtual void Free(std::uint32_t address) = 0;
};

class IBehaviorPathEndpointConfigurationRuntime {
public:
    virtual ~IBehaviorPathEndpointConfigurationRuntime() = default;
    [[nodiscard]] virtual std::uint32_t Read32(std::uint32_t address) = 0;
    virtual void ApplyPathParameter(BehaviorPathEndpointState& state,
                                    std::uint32_t parameter) = 0;
    [[nodiscard]] virtual std::uint32_t Activate(
        BehaviorPathEndpointState& state) = 0;
};

class IBehaviorPathEndpointRuntime : public IBehaviorPathEndpointLifecycleRuntime,
                                    public IBehaviorPathEndpointConfigurationRuntime {};

class BehaviorPathEndpointRuntimeStub final
    : public IBehaviorPathEndpointRuntime {
public:
    void ConstructBase(BehaviorPathEndpointState& state) override;
    void DestructBase(BehaviorPathEndpointState& state) override;
    void Free(std::uint32_t address) override;
    std::uint32_t Read32(std::uint32_t address) override;
    void ApplyPathParameter(BehaviorPathEndpointState& state,
                            std::uint32_t parameter) override;
    std::uint32_t Activate(BehaviorPathEndpointState& state) override;
};

class BehaviorPathEndpointHelper final {
public:
    static constexpr std::uint32_t kVtable = 0x842898u;

    [[nodiscard]] static BehaviorPathEndpointState& Construct(
        BehaviorPathEndpointState& state,
        IBehaviorPathEndpointLifecycleRuntime& runtime);
    [[nodiscard]] static std::uint32_t Delete(
        BehaviorPathEndpointState& state, std::uint32_t address,
        std::uint32_t flags, IBehaviorPathEndpointLifecycleRuntime& runtime);
    static void Destruct(BehaviorPathEndpointState& state,
                         IBehaviorPathEndpointLifecycleRuntime& runtime);
    [[nodiscard]] static std::uint32_t SetPathAndCacheEndpoints(
        BehaviorPathEndpointState& state, std::uint32_t parameter,
        std::uint32_t path_address, IBehaviorPathEndpointConfigurationRuntime& runtime);
    [[nodiscard]] static std::uint32_t InitializeMovement(
        BehaviorPathEndpointState& state,
        IBehaviorPathInitializationRuntime& runtime);
};

}  // namespace pal4
