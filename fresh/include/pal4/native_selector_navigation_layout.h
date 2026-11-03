#pragma once
#include <cstdint>
namespace pal4 {
// Shared navigation borrows the creating selector's bounded image and controls.
class INativeSelectorNavigationLayout {
public:
    virtual ~INativeSelectorNavigationLayout()=default;
    virtual std::uint32_t FrameAddress()const noexcept=0;
    virtual std::uint32_t RootAddress()const noexcept=0;
    virtual std::uint32_t ImageSize()const noexcept=0;
    virtual bool AcceptsInputWindow(std::uint32_t)const noexcept=0;
    virtual std::uint32_t& Word(std::uint32_t)=0;
    virtual void RethrowEventError()=0;
};
}
