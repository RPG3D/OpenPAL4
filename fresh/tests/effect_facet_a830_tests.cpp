#include "pal4/effect_facet_a830.h"

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

struct Runtime final : pal4::IEffectFacetA830Runtime {
    std::uint32_t frees = 0u;
    void Free(pal4::EffectFacetA830Storage& facet) override {
        Check(facet.vtable == 0x84a830u, "native vtable restored before free");
        ++frees;
    }
};

void TestLifetime() {
    pal4::EffectFacetA830Storage facet{0x12345678u};
    Runtime runtime;
    pal4::EffectFacetA830::Initialize(facet);
    Check(facet.vtable == 0x84a830u, "facet initializer writes native vtable");
    facet.vtable = 0u;
    Check(pal4::EffectFacetA830::DeletingDestroy(facet, 0u, runtime) ==
              reinterpret_cast<std::uintptr_t>(&facet) &&
              facet.vtable == 0x84a830u && runtime.frees == 0u,
          "nondeleting destructor restores vtable and returns facet");
    Check(pal4::EffectFacetA830::DeletingDestroy(facet, 3u, runtime) ==
              reinterpret_cast<std::uintptr_t>(&facet) &&
              runtime.frees == 1u,
          "deleting flag bit zero frees facet exactly once");
}

}  // namespace

int main() {
    TestLifetime();
}
