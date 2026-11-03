#include "pal4/base_object_static_accessors.h"

#include <cstdlib>
#include <iostream>
#include <string_view>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

}  // namespace

int main() {
    pal4::BaseObjectState state{};
    state.Write(0u, 0x12345678u);
    state.Write(4u, 0xaaaaaaaau);
    pal4::BaseObjectStaticAccessors::SetDefaultVtable(state);
    Check(state.Read(0u) == 0x8425a0u &&
              state.Read(4u) == 0xaaaaaaaau,
          "default vtable setter touches only first word");
    Check(std::string_view(pal4::BaseObjectStaticAccessors::HoldString()) ==
              "hold" &&
              std::string_view(pal4::BaseObjectStaticAccessors::RpeString()) ==
                  "rpe",
          "constant getters return IDA-backed string literals");
}
