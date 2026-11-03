#pragma once

#include "pal4/window_registration_runtime.h"

#include <cstdint>

namespace pal4 {

using ITemperatureWindowRegistrationRuntime = IWindowRegistrationRuntime;

class TemperatureWindowRegistration final {
public:
    static constexpr std::uint32_t kNamePointer = 0x8b1360u;
    static constexpr std::uint32_t kEventKey = 0x8b135cu;
    static constexpr std::uint32_t kFactory = 0x4a2220u;

    // IDA 0x4a2200-0x4a22c0.
    [[nodiscard]] static std::uint32_t InitializeNameThunk(
        ITemperatureWindowRegistrationRuntime& runtime);
    [[nodiscard]] static std::uint32_t InitializeName(
        ITemperatureWindowRegistrationRuntime& runtime);
    [[nodiscard]] static std::uint32_t Create(
        std::uint32_t layout_name,
        ITemperatureWindowRegistrationRuntime& runtime);
    [[nodiscard]] static std::uint32_t RegisterFactoryThunk(
        ITemperatureWindowRegistrationRuntime& runtime);
    [[nodiscard]] static std::uint32_t RegisterFactory(
        ITemperatureWindowRegistrationRuntime& runtime);
    [[nodiscard]] static std::uint32_t RegisterHandlerThunk(
        ITemperatureWindowRegistrationRuntime& runtime);
    [[nodiscard]] static std::uint32_t RegisterHandler(
        ITemperatureWindowRegistrationRuntime& runtime);
};

}  // namespace pal4
