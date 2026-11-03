#pragma once

#include "pal4/effect_system_object_f.h"

#include <array>
#include <cstdint>

namespace pal4 {

class IEffectSystemObjectFFrameRuntime {
public:
    virtual ~IEffectSystemObjectFFrameRuntime() = default;
    virtual void QueryTaskValues(EffectSystemObjectFState& state,
                                 std::array<std::int32_t, 4>& values) = 0;
    [[nodiscard]] virtual std::int32_t FloatToLong(double value) = 0;
    virtual void WriteTaskBytes(EffectSystemObjectFState& state,
                                const std::array<std::uint8_t, 4>& bytes) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    virtual void UpdateObject(std::uint32_t object,
                              std::uint32_t argument,
                              std::uint32_t delta_low_word,
                              std::uint32_t system_callback) = 0;
};

class EffectSystemObjectFFrameRuntimeStub final
    : public IEffectSystemObjectFFrameRuntime {
public:
    void QueryTaskValues(EffectSystemObjectFState&,
                         std::array<std::int32_t, 4>&) override {}
    [[nodiscard]] std::int32_t FloatToLong(double) override { return 0; }
    void WriteTaskBytes(EffectSystemObjectFState&,
                        const std::array<std::uint8_t, 4>&) override {}
    [[nodiscard]] std::uint32_t ReadWord(
        std::uint32_t) override { return 0u; }
    void UpdateObject(std::uint32_t, std::uint32_t,
                      std::uint32_t, std::uint32_t) override {}
};

class EffectSystemObjectFFrame final {
public:
    // IDA 0x605e60, ret8. Color facet get/set each pop16; the EDI push is
    // a saved register, not a fifth getter argument or an undefined color byte.
    static void Update(EffectSystemObjectFState& state,
                       double delta,
                       IEffectSystemObjectFFrameRuntime& runtime);
};

}  // namespace pal4
