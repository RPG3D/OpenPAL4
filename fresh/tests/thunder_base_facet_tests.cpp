#include "pal4/thunder_base_facet.h"

#include <cstdint>
#include <cstdlib>
#include <iostream>

namespace {

void Check(const bool ok, const char* message) {
    if (!ok) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IThunderBaseFacetRuntime {
    std::uint32_t calls = 0u;
    std::uint32_t address = 0u;
    void Free(const std::uint32_t value) override {
        ++calls;
        address = value;
    }
};

}  // namespace

int main() {
    std::uint32_t vtable = 0u;
    pal4::ThunderBaseFacet::InstallVtable(vtable);
    Check(vtable == 0x84abd0u, "base facet installs native vtable");
    Runtime runtime;
    vtable = 3u;
    Check(pal4::ThunderBaseFacet::DeletingDestructor(
              vtable, 0x1000u, 2u, runtime) == 0x1000u &&
              vtable == 0x84abd0u && runtime.calls == 0u,
          "delete without free restores vtable");
    Check(pal4::ThunderBaseFacet::DeletingDestructor(
              vtable, 0x1000u, 3u, runtime) == 0x1000u &&
              runtime.calls == 1u && runtime.address == 0x1000u,
          "delete bit zero frees standalone facet address");
}
