#include "pal4/introduction_window_registration.h"

namespace pal4 {

const char* IntroductionWindowRegistration::RegisterName(
    const char*& destination) noexcept {
    destination = "IntroductionWindow";
    return destination;
}

const char* IntroductionWindowRegistration::RegisterNameThunk(
    const char*& destination) noexcept {
    return RegisterName(destination);
}

}  // namespace pal4
