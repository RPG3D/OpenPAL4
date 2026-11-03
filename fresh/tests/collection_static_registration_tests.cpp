#include "pal4/collection_static_registration.h"

#include <cstdlib>
#include <cstring>
#include <iostream>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

}  // namespace

int main() {
    const char* destination = "old";
    const char* result =
        pal4::CollectionStaticRegistration::RegisterCastWindowName(destination);
    Check(result == destination && std::strcmp(result, "CastWindow") == 0,
          "direct entry writes and returns CastWindow pointer");
    destination = nullptr;
    result = pal4::CollectionStaticRegistration::RegisterCastWindowNameThunk(
        destination);
    Check(result == destination && std::strcmp(result, "CastWindow") == 0,
          "thunk reaches the same global string write");
    std::cout << "collection static registration tests passed\n";
}
