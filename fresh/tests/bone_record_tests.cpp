#include "pal4/bone_record.h"

#include <cstdlib>
#include <iostream>

int main() {
    pal4::BoneRecordState state{{1u, 2u, 3u, 4u}};
    if (&pal4::BoneRecord::Initialize(state) != &state ||
        state.words != std::array<std::uint32_t, 4>{0u, 0u, 0u, 0u}) {
        std::cerr << "FAIL: bone constructor clears four words and returns this\n";
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
