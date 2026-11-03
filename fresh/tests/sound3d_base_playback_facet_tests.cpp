#include "pal4/sound3d_base_playback_facet.h"

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

struct Runtime final : pal4::ISound3DBasePlaybackFacetRuntime {
    std::uint32_t frees = 0u;
    void Free(pal4::Sound3DBasePlaybackFacetStorage& facet) override {
        Check(facet.vtable == 0x84a9d8u,
              "base sound facet vtable restored before free");
        ++frees;
    }
};

}  // namespace

int main() {
    pal4::Sound3DBasePlaybackFacetStorage facet{0x12345678u};
    Runtime runtime;
    pal4::Sound3DBasePlaybackFacet::Initialize(facet);
    Check(facet.vtable == 0x84a9d8u, "initializer writes native vtable");
    facet.vtable = 0u;
    Check(pal4::Sound3DBasePlaybackFacet::DeletingDestroy(
              facet, 0u, runtime) ==
              reinterpret_cast<std::uintptr_t>(&facet) &&
              facet.vtable == 0x84a9d8u && runtime.frees == 0u,
          "nondeleting path restores vtable and returns facet");
    Check(pal4::Sound3DBasePlaybackFacet::DeletingDestroy(
              facet, 3u, runtime) ==
              reinterpret_cast<std::uintptr_t>(&facet) && runtime.frees == 1u,
          "deleting flag frees facet exactly once");
}
