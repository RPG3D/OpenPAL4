#pragma once

#include <cstdint>

namespace pal4 {

class IUiExitGameRuntime {
public:
    virtual ~IUiExitGameRuntime() = default;
    [[nodiscard]] virtual std::uint32_t GameInstance() = 0;
    virtual void HandleExitCommand(std::uint32_t game) = 0;
};

class UiExitGameHandler final {
public:
    // IDA 0x4bfe10. The callback argument is unused by the binary.
    [[nodiscard]] static std::uint8_t Handle(
        std::uint32_t callback_argument,
        IUiExitGameRuntime& runtime);
};

}  // namespace pal4
