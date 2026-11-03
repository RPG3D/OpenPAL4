#include "pal4/load_window_registration.h"

#include <cstdlib>
#include <iostream>
#include <string>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

}  // namespace

int main() {
    const char* destination = nullptr;
    const auto* result = pal4::LoadWindowRegistration::RegisterName(
        destination);
    Check(result == destination && std::string(result) == "loadWindow",
          "name registration stores and returns loadWindow pointer");
    destination = nullptr;
    result = pal4::LoadWindowRegistration::RegisterNameThunk(destination);
    Check(result == destination && std::string(result) == "loadWindow",
          "name registration thunk forwards the same result");

    std::cout << "Load window registration tests passed\n";
}
