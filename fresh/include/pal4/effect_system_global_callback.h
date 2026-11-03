#pragma once

#include <cstdint>

namespace pal4 {

class IEffectSystemGlobalCallbackRuntime {
public:
    virtual ~IEffectSystemGlobalCallbackRuntime() = default;
    [[nodiscard]] virtual std::uint32_t Invoke(std::uint32_t address) = 0;
};

class EffectSystemGlobalCallback final {
public:
    // IDA 0x6536b0: store and return the callback at global 0x93cec8.
    [[nodiscard]] static std::uint32_t Set(std::uint32_t& slot,
                                           std::uint32_t callback) {
        slot = callback;
        return callback;
    }

    // IDA 0x6536c0: call a non-null callback and return its result.
    [[nodiscard]] static std::uint32_t Call(
        std::uint32_t slot, IEffectSystemGlobalCallbackRuntime& runtime) {
        return slot == 0u ? 0u : runtime.Invoke(slot);
    }
};

}  // namespace pal4
