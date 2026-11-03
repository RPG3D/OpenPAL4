#pragma once

#include "pal4/effect_system_object_f.h"

#include <cstdint>

namespace pal4 {

class IEffectSystemObjectFCollectionRuntime {
public:
    virtual ~IEffectSystemObjectFCollectionRuntime() = default;
    [[nodiscard]] virtual std::uint32_t HandleEvent(
        EffectSystemObjectFState& state, std::uint32_t xml_offset,
        std::uint32_t context, std::uint32_t event) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    virtual void Free(std::uint32_t address) = 0;
    virtual void Release(std::uint32_t object) = 0;
};

class EffectSystemObjectFCollectionRuntimeStub final
    : public IEffectSystemObjectFCollectionRuntime {
public:
    [[nodiscard]] std::uint32_t HandleEvent(
        EffectSystemObjectFState&, std::uint32_t,
        std::uint32_t, std::uint32_t) override { return 0u; }
    [[nodiscard]] std::uint32_t ReadWord(std::uint32_t) override { return 0u; }
    void WriteWord(std::uint32_t, std::uint32_t) override {}
    [[nodiscard]] std::uint32_t Allocate(std::uint32_t) override { return 0u; }
    void Free(std::uint32_t) override {}
    void Release(std::uint32_t) override {}
};

class EffectSystemObjectFCollection final {
public:
    // IDA 0x606520. Existing elements satisfy the request immediately.
    [[nodiscard]] static bool Update(EffectSystemObjectFState& state,
                                     std::uint32_t requested,
                                     IEffectSystemObjectFCollectionRuntime& runtime);
    // IDA 0x606750. Native char return is the old end pointer's low byte.
    [[nodiscard]] static std::uint8_t Clear(
        EffectSystemObjectFState& state,
        IEffectSystemObjectFCollectionRuntime& runtime);
    // IDA 0x6067a0.
    [[nodiscard]] static std::uint32_t Get(
        const EffectSystemObjectFState& state, std::uint32_t index,
        IEffectSystemObjectFCollectionRuntime& runtime);
};

}  // namespace pal4
