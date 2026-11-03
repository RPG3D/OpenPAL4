#pragma once

#include "pal4/effect_object.h"

#include <cstdint>

namespace pal4 {

class IEffectObjectPlaybackFacetRuntime {
public:
    virtual ~IEffectObjectPlaybackFacetRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    virtual void ResetInstance(std::uint32_t instance) = 0;
};

class EffectObjectPlaybackFacetRuntimeStub final
    : public IEffectObjectPlaybackFacetRuntime {
public:
    [[nodiscard]] std::uint32_t ReadWord(std::uint32_t) override { return 0u; }
    [[nodiscard]] std::uint8_t ReadByte(std::uint32_t) override { return 0u; }
    void WriteByte(std::uint32_t, std::uint8_t) override {}
    void ResetInstance(std::uint32_t) override {}
};

class EffectObjectPlaybackFacet final {
public:
    // IDA 0x610b00, tail 0x62a690.
    [[nodiscard]] static std::uint8_t Pause(
        const EffectObjectState& state,
        IEffectObjectPlaybackFacetRuntime& runtime);
    // IDA 0x610b20, tail 0x62a9e0.
    [[nodiscard]] static std::uint8_t Resume(
        const EffectObjectState& state,
        IEffectObjectPlaybackFacetRuntime& runtime);
    // IDA 0x610b40; entry_eax is preserved on early exits.
    [[nodiscard]] static std::uint32_t Clear(
        EffectObjectState& state, std::uint32_t entry_eax,
        IEffectObjectPlaybackFacetRuntime& runtime);
};

class IEffectObjectMetricRuntime {
public:
    virtual ~IEffectObjectMetricRuntime() = default;
    virtual void Prime(EffectObjectState& state, std::uint32_t mode) = 0;
    [[nodiscard]] virtual double Read(EffectObjectState& state) = 0;
};

class EffectObjectMetricRuntimeStub final : public IEffectObjectMetricRuntime {
public:
    void Prime(EffectObjectState&, std::uint32_t) override {}
    [[nodiscard]] double Read(EffectObjectState&) override { return 0.0; }
};

class EffectObjectMetric final {
public:
    // IDA 0x610b70, entered through the owner +36 facet.
    [[nodiscard]] static double GetProgress(
        EffectObjectState& state, IEffectObjectMetricRuntime& runtime);
};

struct EffectObjectFacetA718Storage final {
    std::uint32_t vtable = 0u;
};

class IEffectObjectFacetA718Runtime {
public:
    virtual ~IEffectObjectFacetA718Runtime() = default;
    virtual void FreeFacet(EffectObjectFacetA718Storage& facet) = 0;
};

class EffectObjectFacetA718RuntimeStub final
    : public IEffectObjectFacetA718Runtime {
public:
    void FreeFacet(EffectObjectFacetA718Storage&) override {}
};

class EffectObjectFacetA718 final {
public:
    static constexpr std::uint32_t kVtable = 0x84a718u;
    // IDA 0x611500/0x611510, used by the EffectObject +36 facet.
    static void Initialize(EffectObjectFacetA718Storage& facet);
    [[nodiscard]] static std::uintptr_t DeletingDestroy(
        EffectObjectFacetA718Storage& facet, std::uint32_t flags,
        IEffectObjectFacetA718Runtime& runtime);
};

}  // namespace pal4
