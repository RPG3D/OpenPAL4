#pragma once

#include "pal4/game_frame_updater.h"

#include <cstdint>

namespace pal4 {

class IWin32GameFrameServices {
public:
    virtual ~IWin32GameFrameServices() = default;
    virtual void ObserveComparisonFrameEnd() {}
    virtual void ProcessInputs() = 0;
    virtual void InvokeFrame(std::uint32_t callback, double elapsed,
                             std::uint32_t flag) = 0;
};

class Win32GameFrameRuntime final : public IGameFrameTimingRuntime {
public:
    explicit Win32GameFrameRuntime(IWin32GameFrameServices& services)
        : services_(services) {}

    void Update(GameFrameUpdater& updater, std::uint32_t callback);
    [[nodiscard]] std::uint32_t TimeGetTime() override;
    void ProcessInputs() override;
    void InvokeFrame(double elapsed, std::uint32_t flag) override;

private:
    IWin32GameFrameServices& services_;
    std::uint32_t callback_ = 0u;
};

}  // namespace pal4
