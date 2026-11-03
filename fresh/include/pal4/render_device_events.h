#pragma once

#include "pal4/game_render_system.h"

#include <cstdint>

namespace pal4 {

class IRenderDeviceEventRuntime {
public:
    virtual ~IRenderDeviceEventRuntime() = default;
    [[nodiscard]] virtual std::int32_t TestCooperativeLevel(
        std::uint32_t device) = 0;
    [[nodiscard]] virtual std::int32_t Present(
        std::uint32_t device, std::uint32_t window) = 0;
    [[nodiscard]] virtual bool HasLostCallback() = 0;
    [[nodiscard]] virtual std::uint32_t InvokeLostCallback() = 0;
    [[nodiscard]] virtual bool HasRestoredCallback() = 0;
    virtual void InvokeRestoredCallback() = 0;
    virtual void ChangeVideoMode(std::uint32_t mode) = 0;
};

class RenderDeviceEvents final {
public:
    explicit RenderDeviceEvents(GameRenderState& state) : state_(state) {}

    [[nodiscard]] std::uint32_t HandleVideoModeChange(
        IRenderDeviceEventRuntime& runtime) const;
    [[nodiscard]] std::int32_t HandleDeviceLoss(
        IRenderDeviceEventRuntime& runtime) const;

private:
    GameRenderState& state_;
};

}  // namespace pal4
