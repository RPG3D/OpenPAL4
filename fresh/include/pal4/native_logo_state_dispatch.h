#pragma once
#include <cstdint>

namespace pal4 {
// Borrowed state-0 boundary. The registered address must belong to this owner.
// Implementations must invoke real lifecycle/frame services, once per frame.
class INativeLogoStateDispatch {
public:
    virtual ~INativeLogoStateDispatch() = default;
    virtual std::uint32_t Address() const = 0;
    virtual bool Enter() = 0;
    virtual void Leave() = 0;
    virtual void Render(std::uint32_t frame, bool obey_movie_clock) = 0;
    virtual std::uint32_t Update(float elapsed, bool suppress_screen_effect) = 0;
};
}
