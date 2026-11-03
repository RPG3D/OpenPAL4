#include "pal4/effect_system_object_base.h"

#include <cstdint>
#include <cstdlib>
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
    pal4::EffectSystemObjectBaseState state;
    state.words.fill(0xffffffffu);
    Check(&pal4::EffectSystemObjectBase::Construct(state) == &state &&
              state.words[0u] == 0x84b240u &&
              state.words[1u] == 0u && state.words[2u] == 0u &&
              state.words[3u] == 0u && state.words[4u] == 0u,
          "base constructor clears all sixteen data bytes");
    state.words = {0u, 1u, 2u, 3u, 4u};
    pal4::EffectSystemObjectBase::RestoreVtable(state);
    Check(state.words[0u] == 0x84b240u &&
              state.words[1u] == 1u && state.words[2u] == 2u &&
              state.words[3u] == 3u && state.words[4u] == 4u,
          "base destructor tail changes only the vtable");
}
