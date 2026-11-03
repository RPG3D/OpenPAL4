#pragma once

#include <cstdint>
#include "pal4/window_registration_runtime.h"

namespace pal4 {

using IRepeatGameWindowRegistrationRuntime = IWindowRegistrationRuntime;

class RepeatGameWindowRegistration final {
public:
    static constexpr std::uint32_t kNamePointer = 0x8b1324u;
    static constexpr std::uint32_t kEventKey = 0x8b1320u;
    static constexpr std::uint32_t kFactory = 0x4a1d30u;
    static constexpr std::uint32_t kHandlerVtable = 0x843494u;

    // IDA 0x4a1d10/0x4a1d20.
    [[nodiscard]] static std::uint32_t InitializeNameThunk(
        IRepeatGameWindowRegistrationRuntime& runtime);
    [[nodiscard]] static std::uint32_t InitializeName(
        IRepeatGameWindowRegistrationRuntime& runtime);
    // IDA 0x4a1d30.
    [[nodiscard]] static std::uint32_t Create(
        std::uint32_t layout_name,
        IRepeatGameWindowRegistrationRuntime& runtime);
    // IDA 0x4a1d90/0x4a1da0.
    [[nodiscard]] static std::uint32_t RegisterFactoryThunk(
        IRepeatGameWindowRegistrationRuntime& runtime);
    [[nodiscard]] static std::uint32_t RegisterFactory(
        IRepeatGameWindowRegistrationRuntime& runtime);
    // IDA 0x4a1dc0/0x4a1dd0.
    [[nodiscard]] static std::uint32_t RegisterHandlerThunk(
        IRepeatGameWindowRegistrationRuntime& runtime);
    [[nodiscard]] static std::uint32_t RegisterHandler(
        IRepeatGameWindowRegistrationRuntime& runtime);
};

}  // namespace pal4
