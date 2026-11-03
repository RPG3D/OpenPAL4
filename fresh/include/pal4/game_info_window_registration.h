#pragma once

#include "pal4/window_registration_runtime.h"

#include <cstdint>

namespace pal4 {

using IGameInfoWindowRegistrationRuntime = IWindowRegistrationRuntime;

class GameInfoWindowRegistration final {
public:
    static constexpr std::uint32_t kNamePointer = 0x8b139cu;
    static constexpr std::uint32_t kFactory = 0x4a6420u;

    // IDA 0x4a6400-0x4a6620.
    [[nodiscard]] static std::uint32_t InitializeNameThunk(
        IGameInfoWindowRegistrationRuntime& runtime);
    [[nodiscard]] static std::uint32_t InitializeName(
        IGameInfoWindowRegistrationRuntime& runtime);
    [[nodiscard]] static std::uint32_t Create(
        std::uint32_t layout_name,
        IGameInfoWindowRegistrationRuntime& runtime);
    [[nodiscard]] static std::uint32_t RegisterFactoryThunk(
        IGameInfoWindowRegistrationRuntime& runtime);
    [[nodiscard]] static std::uint32_t RegisterFactory(
        IGameInfoWindowRegistrationRuntime& runtime);
    [[nodiscard]] static std::uint32_t RegisterShowThunk(
        IGameInfoWindowRegistrationRuntime& runtime);
    [[nodiscard]] static std::uint32_t RegisterShow(
        IGameInfoWindowRegistrationRuntime& runtime);
    [[nodiscard]] static std::uint32_t RegisterHelpThunk(
        IGameInfoWindowRegistrationRuntime& runtime);
    [[nodiscard]] static std::uint32_t RegisterHelp(
        IGameInfoWindowRegistrationRuntime& runtime);
    [[nodiscard]] static std::uint32_t RegisterUpdateThunk(
        IGameInfoWindowRegistrationRuntime& runtime);
    [[nodiscard]] static std::uint32_t RegisterUpdate(
        IGameInfoWindowRegistrationRuntime& runtime);
};

}  // namespace pal4
