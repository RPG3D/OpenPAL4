#include "pal4/cegui_map_operations.h"

#include <cstdlib>
#include <iostream>
#include <map>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

class Runtime final : public pal4::ICeguiMapCleanupRuntime {
public:
    std::map<std::uint32_t, std::uint32_t> links;
    std::vector<std::uint32_t> reads;
    std::vector<std::uint32_t> freed;

    std::uint32_t ReadDword(const std::uint32_t address) override {
        reads.push_back(address);
        return links.at(address);
    }
    void FreeNode(const std::uint32_t node) override {
        freed.push_back(node);
    }
};

void NilInputDoesNothing() {
    Runtime runtime;
    const pal4::CeguiTreeGlobals globals{1u, 0x9000u};
    Check(pal4::CeguiMapOperations::CleanupSubtree(
              globals.nil, globals, runtime) == globals.nil &&
              runtime.reads.empty() && runtime.freed.empty(),
          "nil cleanup returns nil without memory access");
}

void RightBeforeLeftThenParent() {
    Runtime runtime;
    const pal4::CeguiTreeGlobals globals{1u, 0x9000u};
    runtime.links = {
        {0x3000u, 0x3100u}, {0x3008u, 0x3200u},
        {0x3100u, 0x3300u}, {0x3108u, 0x3400u},
        {0x3200u, 0x9000u}, {0x3208u, 0x3500u},
        {0x3300u, 0x9000u}, {0x3308u, 0x9000u},
        {0x3400u, 0x9000u}, {0x3408u, 0x9000u},
        {0x3500u, 0x9000u}, {0x3508u, 0x9000u},
    };
    Check(pal4::CeguiMapOperations::CleanupSubtree(
              0x3000u, globals, runtime) == globals.nil &&
              runtime.freed == std::vector<std::uint32_t>{
                  0x3500u, 0x3200u, 0x3000u, 0x3400u, 0x3100u, 0x3300u},
          "cleanup frees each right subtree before following the left chain");
    Check(runtime.reads.front() == 0x3008u &&
              runtime.reads.back() == 0x3300u,
          "cleanup reads right link before left link at each node");
}

}  // namespace

int main() {
    NilInputDoesNothing();
    RightBeforeLeftThenParent();
    std::cout << "2 CEGUI map cleanup test cases passed\n";
}
