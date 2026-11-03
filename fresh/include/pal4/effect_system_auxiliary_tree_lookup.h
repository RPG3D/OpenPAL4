#pragma once

#include <cstdint>
#include "pal4/effect_system_registration.h"

namespace pal4 {

class IEffectSystemAuxiliaryLookupRuntime {
public:
    virtual ~IEffectSystemAuxiliaryLookupRuntime() = default;
    [[nodiscard]] virtual std::uint32_t Nil(EffectRegistrationTree kind) const {
        return kind == EffectRegistrationTree::Clumps ? 0x93ce10u : 0x93cdfcu;
    }
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) const = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
};

class EffectSystemAuxiliaryTreeLookup final {
public:
    // IDA 0x63d7e0: result-slot lower bound for the 24-byte atomic tree.
    [[nodiscard]] static std::uint32_t LowerBoundAtomicInto(
        std::uint32_t tree, std::uint32_t result,
        std::uint32_t key_address,
        IEffectSystemAuxiliaryLookupRuntime& runtime);
    // IDA 0x63da90/0x63db20: direct lower bounds for both trees.
    [[nodiscard]] static std::uint32_t LowerBoundClump(
        std::uint32_t tree, std::uint32_t key_address,
        IEffectSystemAuxiliaryLookupRuntime& runtime);
    [[nodiscard]] static std::uint32_t LowerBoundAtomic(
        std::uint32_t tree, std::uint32_t key_address,
        IEffectSystemAuxiliaryLookupRuntime& runtime);
    // IDA 0x63dac0/0x63db50: advance an iterator and preserve native EAX.
    [[nodiscard]] static std::uint32_t NextClump(
        std::uint32_t& cursor,
        IEffectSystemAuxiliaryLookupRuntime& runtime);
    [[nodiscard]] static std::uint32_t NextAtomic(
        std::uint32_t& cursor,
        IEffectSystemAuxiliaryLookupRuntime& runtime);
};

}  // namespace pal4
