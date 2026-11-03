#pragma once
#include "pal4/native_logo_state_dispatch.h"

namespace pal4 {
class NativeLogoStateLifecycle;
class NativeLogoStateUpdate;
class NativeLogoStateRenderer;

// Borrows the three capabilities of one Logo object. Unbind from dispatch and
// retire the registry before destroying this session or its dependencies.
class NativeLogoStateSession final : public INativeLogoStateDispatch {
public:
    NativeLogoStateSession(NativeLogoStateLifecycle&, NativeLogoStateUpdate&,
                           NativeLogoStateRenderer&);
    std::uint32_t Address() const override;
    bool Enter() override;
    void Leave() override;
    void Render(std::uint32_t frame, bool obey_movie_clock) override;
    std::uint32_t Update(float elapsed, bool suppress_screen_effect) override;
private:
    NativeLogoStateLifecycle& lifecycle_;
    NativeLogoStateUpdate& update_;
    NativeLogoStateRenderer& render_;
};
}
