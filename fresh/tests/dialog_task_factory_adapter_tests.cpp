#include "pal4/dialog_object.h"

#include <cstdlib>
#include <iostream>

int main() {
    pal4::DialogTaskFactoryAdapterState source{{0x1111u, 0x2345u}};
    pal4::DialogTaskFactoryAdapterState state{{0xffffffffu, 0xffffffffu}};
    auto& result = pal4::DialogTaskFactoryAdapter::Construct(state, source);
    if (&result != &state ||
        state.words[0u] != pal4::DialogTaskFactoryAdapter::kVtable ||
        state.words[1u] != 0x2345u || source.words[0u] != 0x1111u) {
        std::cerr << "FAIL: native adapter vtable and XML node copy\n";
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
