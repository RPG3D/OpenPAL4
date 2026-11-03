#pragma once

#include "pal4/screen_effect_dispatcher.h"

#include <cstdint>

namespace pal4 {

class IGameManagerRenderParameterRuntime {
public:
    virtual ~IGameManagerRenderParameterRuntime() = default;
    virtual void SyncCamera(std::uint32_t camera_record,
                            std::uint32_t first,
                            std::uint32_t second) = 0;
    [[nodiscard]] virtual std::uint32_t InvokeHandler(
        std::uint32_t handler, std::uint32_t slot_offset,
        std::uint32_t first, std::uint32_t second) = 0;
};

class GameManagerRenderParameterRuntimeStub final
    : public IGameManagerRenderParameterRuntime {
public:
    void SyncCamera(std::uint32_t camera_record,
                    std::uint32_t first, std::uint32_t second) override;
    [[nodiscard]] std::uint32_t InvokeHandler(
        std::uint32_t handler, std::uint32_t slot_offset,
        std::uint32_t first, std::uint32_t second) override;
};

class GameManagerRenderParameters final {
public:
    [[nodiscard]] static std::uint32_t Update(
        const ScreenEffectDispatcherState& state,
        std::uint32_t first, std::uint32_t second,
        std::uint32_t incoming_eax,
        IGameManagerRenderParameterRuntime& runtime);
};

}  // namespace pal4
