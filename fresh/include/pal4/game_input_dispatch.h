#pragma once

#include <array>
#include <cstdint>

namespace pal4 {

class IGameInputCallbackRuntime {
public:
    virtual ~IGameInputCallbackRuntime() = default;
    [[nodiscard]] virtual std::uint32_t Invoke(std::uint32_t address) = 0;
};

class GameInputDispatch final {
public:
    explicit GameInputDispatch(std::array<std::uint32_t, 12>& config)
        : config_(config) {}

    [[nodiscard]] std::uint32_t HandleKeyboard(
        IGameInputCallbackRuntime& runtime) const;
    [[nodiscard]] std::uint32_t HandleMouse(
        IGameInputCallbackRuntime& runtime) const;

private:
    std::array<std::uint32_t, 12>& config_;
};

}  // namespace pal4
