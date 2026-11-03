#pragma once

#include "pal4/window_registration_runtime.h"

#include <cstdint>

namespace pal4 {

using IHelpWindowRegistrationRuntime = IWindowRegistrationRuntime;

class HelpWindowRegistration final {
public:
    static constexpr std::uint32_t kNamePointer = 0x8b13acu;
    static constexpr std::uint32_t kStoredName = 0x8eaca4u;
    static constexpr std::uint32_t kFactory = 0x4a7450u;
    static constexpr std::uint32_t kShowEventKey = 0x8b1c30u;

    // IDA 0x4a7430-0x4a74f0.
    [[nodiscard]] static std::uint32_t InitializeNameThunk(
        IHelpWindowRegistrationRuntime& runtime);
    [[nodiscard]] static std::uint32_t InitializeName(
        IHelpWindowRegistrationRuntime& runtime);
    [[nodiscard]] static std::uint32_t Create(
        std::uint32_t layout_name, IHelpWindowRegistrationRuntime& runtime);
    [[nodiscard]] static std::uint32_t RegisterFactoryThunk(
        IHelpWindowRegistrationRuntime& runtime);
    [[nodiscard]] static std::uint32_t RegisterFactory(
        IHelpWindowRegistrationRuntime& runtime);
    [[nodiscard]] static std::uint32_t RegisterShowThunk(
        IHelpWindowRegistrationRuntime& runtime);
    [[nodiscard]] static std::uint32_t RegisterShow(
        IHelpWindowRegistrationRuntime& runtime);
};

}  // namespace pal4
