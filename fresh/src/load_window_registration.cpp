#include "pal4/load_window_registration.h"

namespace pal4 {

const char* LoadWindowRegistration::RegisterName(
    const char*& destination) noexcept {
    destination = "loadWindow";
    return destination;
}

const char* LoadWindowRegistration::RegisterNameThunk(
    const char*& destination) noexcept {
    return RegisterName(destination);
}

}  // namespace pal4
