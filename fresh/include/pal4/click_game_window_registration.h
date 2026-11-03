#pragma once

#include <cstdint>
#include "pal4/window_registration_runtime.h"

namespace pal4 {

using IClickGameWindowRegistrationRuntime = IWindowRegistrationRuntime;

class ClickGameWindowRegistration final {
public:
    // IDA 0x49f250/0x49f260.
    [[nodiscard]] static std::uint32_t InitializeNameThunk(
        IClickGameWindowRegistrationRuntime& runtime);
    [[nodiscard]] static std::uint32_t InitializeName(
        IClickGameWindowRegistrationRuntime& runtime);
    // IDA 0x49f270.
    [[nodiscard]] static std::uint32_t Create(
        std::uint32_t layout_name,
        IClickGameWindowRegistrationRuntime& runtime);
    // IDA 0x49f2d0/0x49f2e0.
    [[nodiscard]] static std::uint32_t RegisterFactoryThunk(
        IClickGameWindowRegistrationRuntime& runtime);
    [[nodiscard]] static std::uint32_t RegisterFactory(
        IClickGameWindowRegistrationRuntime& runtime);
    // IDA 0x49f300/0x49f310.
    [[nodiscard]] static std::uint32_t RegisterHandlerThunk(
        IClickGameWindowRegistrationRuntime& runtime);
    [[nodiscard]] static std::uint32_t RegisterHandler(
        IClickGameWindowRegistrationRuntime& runtime);
};

}  // namespace pal4
