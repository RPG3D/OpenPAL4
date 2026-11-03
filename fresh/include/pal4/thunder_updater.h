#pragma once

#include "pal4/thunder_object.h"

#include <array>
#include <cstdint>

namespace pal4 {

class IThunderUpdateRuntime {
public:
    virtual ~IThunderUpdateRuntime() = default;
    virtual void ReadPlayback(ThunderObjectState& state,
                              std::array<std::uint32_t, 4u>& outputs) = 0;
    [[nodiscard]] virtual std::int32_t FloatToInt(double value) = 0;
    [[nodiscard]] virtual std::uint8_t StackByte() = 0;
    virtual void WritePlayback(
        ThunderObjectState& state,
        const std::array<std::uint8_t, 4u>& bytes) = 0;
    virtual void ResolveTarget(
        ThunderObjectState& state, std::uint32_t owner,
        std::array<std::uint32_t, 3u>& output) = 0;
    [[nodiscard]] virtual std::uint32_t AdvanceVisual(
        ThunderObjectState& state, double elapsed) = 0;
};

class ThunderUpdateRuntimeStub final : public IThunderUpdateRuntime {
public:
    void ReadPlayback(ThunderObjectState&,
                      std::array<std::uint32_t, 4u>&) override {}
    [[nodiscard]] std::int32_t FloatToInt(double value) override {
        return static_cast<std::int32_t>(value);
    }
    [[nodiscard]] std::uint8_t StackByte() override { return 0u; }
    void WritePlayback(ThunderObjectState&,
                       const std::array<std::uint8_t, 4u>&) override {}
    void ResolveTarget(ThunderObjectState&, std::uint32_t,
                       std::array<std::uint32_t, 3u>&) override {}
    [[nodiscard]] std::uint32_t AdvanceVisual(
        ThunderObjectState&, double) override { return 0u; }
};

class ThunderUpdater final {
public:
    // IDA 0x61c370. Inactive paths preserve incoming EAX.
    [[nodiscard]] static std::uint32_t Update(
        ThunderObjectState& state, double elapsed,
        std::uint32_t entry_eax, IThunderUpdateRuntime& runtime);
};

}  // namespace pal4
