#include "pal4/introduction_window_registration.h"

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
    const char* global = nullptr;
    const auto direct = pal4::IntroductionWindowRegistration::RegisterName(
        global);
    Check(direct == global &&
              std::strcmp(global, "IntroductionWindow") == 0,
          "direct registration stores and returns IntroductionWindow");

    global = nullptr;
    const auto thunk =
        pal4::IntroductionWindowRegistration::RegisterNameThunk(global);
    Check(thunk == global && thunk == direct,
          "registration thunk forwards to direct registration");
    std::cout << "Introduction window registration tests passed\n";
}
