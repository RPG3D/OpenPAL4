#include "pal4/thunder_deletion.h"

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace {

void Check(const bool ok, const char* message) {
    if (!ok) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IThunderDeletionRuntime {
    std::vector<std::string> calls;
    std::uint32_t freed = 0u;
    void Destroy(pal4::ThunderObjectState&) override {
        calls.emplace_back("destroy");
    }
    void Free(const std::uint32_t address) override {
        calls.emplace_back("free");
        freed = address;
    }
};

}  // namespace

int main() {
    pal4::ThunderObjectState state{};
    Runtime runtime;
    Check(pal4::ThunderDeletion::Delete(
              state, 0x1000u, 0u, runtime) == 0x1000u &&
              runtime.calls == std::vector<std::string>{"destroy"},
          "deleting destructor always destroys and conditionally frees");
    runtime.calls.clear();
    Check(pal4::ThunderDeletion::Delete(
              state, 0x1000u, 3u, runtime) == 0x1000u &&
              runtime.calls == std::vector<std::string>{"destroy", "free"} &&
              runtime.freed == 0x1000u,
          "delete flag bit zero frees original owner after destruction");
    for (const auto offset : pal4::ThunderDeletion::kFacetOffsets) {
        runtime.calls.clear();
        Check(pal4::ThunderDeletion::DeleteFromFacet(
                  state, 0x1000u + offset, offset, 1u, runtime) ==
                  0x1000u &&
                  runtime.freed == 0x1000u &&
                  runtime.calls ==
                      std::vector<std::string>{"destroy", "free"},
              "facet thunk subtracts its own offset before shared delete");
    }
}
