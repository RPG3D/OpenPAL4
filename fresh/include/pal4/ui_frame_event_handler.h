#pragma once

#include <cstdint>

namespace pal4 {

struct UiFrameEvent {
    std::uint32_t enabled{};
    std::uint32_t argument{};
};

class IUiFrameEventRuntime {
public:
    virtual ~IUiFrameEventRuntime() = default;
    [[nodiscard]] virtual std::uint32_t FrameManager() = 0;
    [[nodiscard]] virtual std::uint32_t FindFrame(
        std::uint32_t manager, std::uint32_t name) = 0;
    virtual void RegisterFrame(std::uint32_t manager,
                               std::uint32_t name) = 0;
    virtual void SetVisible(std::uint32_t frame,
                            std::uint32_t enabled,
                            std::uint32_t argument) = 0;
};

class UiFrameEventHandler final {
public:
    [[nodiscard]] static bool Handle(std::uint32_t name,
                                     UiFrameEvent event,
                                     IUiFrameEventRuntime& runtime);
};

}  // namespace pal4
