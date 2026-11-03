#include "pal4/collection_static_registration.h"

namespace pal4 {

const char* CollectionStaticRegistration::RegisterCastWindowName(
    const char*& destination) noexcept {
    destination = "CastWindow";
    return destination;
}

const char* CollectionStaticRegistration::RegisterCastWindowNameThunk(
    const char*& destination) noexcept {
    return RegisterCastWindowName(destination);
}

}  // namespace pal4
