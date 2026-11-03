#pragma once

#include "pal4/window_registration_runtime.h"

#include <cstdint>

namespace pal4 {

using IFrameToolbarRegistrationRuntime = IWindowRegistrationRuntime;

class FrameToolbarRegistration final {
public:
    static constexpr std::uint32_t kNamePointer = 0x8b1398u;
    static constexpr std::uint32_t kEventKey = 0x8b1394u;
    static constexpr std::uint32_t kFactory = 0x4a5ff0u;

    // IDA 0x4a5fd0-0x4a6090.
    [[nodiscard]] static std::uint32_t InitializeNameThunk(
        IFrameToolbarRegistrationRuntime& runtime);
    [[nodiscard]] static std::uint32_t InitializeName(
        IFrameToolbarRegistrationRuntime& runtime);
    [[nodiscard]] static std::uint32_t Create(
        std::uint32_t layout_name, IFrameToolbarRegistrationRuntime& runtime);
    [[nodiscard]] static std::uint32_t RegisterFactoryThunk(
        IFrameToolbarRegistrationRuntime& runtime);
    [[nodiscard]] static std::uint32_t RegisterFactory(
        IFrameToolbarRegistrationRuntime& runtime);
    [[nodiscard]] static std::uint32_t RegisterHandlerThunk(
        IFrameToolbarRegistrationRuntime& runtime);
    [[nodiscard]] static std::uint32_t RegisterHandler(
        IFrameToolbarRegistrationRuntime& runtime);
};

}  // namespace pal4
