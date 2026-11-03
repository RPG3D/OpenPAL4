#include "pal4/d3d_face_trace_reachability.h"

#include <cstdlib>
#include <iostream>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Step final {
    std::uint32_t face;
    std::uint32_t status;
    std::uint32_t neighbor;
};

struct Runtime final : pal4::ID3dFaceTraceReachabilityRuntime {
    std::vector<Step> steps;
    std::size_t next = 0;

    std::uint32_t Trace(std::uint32_t face,
                        const pal4::D3dLine2State& line,
                        std::uint32_t& neighbor) override {
        const auto& step = steps.at(next++);
        Check(face == step.face && line.start.x == 1.0f &&
                  line.start.z == 3.0f && line.end.x == 4.0f &&
                  line.end.z == 6.0f && line.normal_cached == 0u,
              "every trace uses the original X/Z segment and next face");
        neighbor = step.neighbor;
        return step.status;
    }
};

bool Run(Runtime& runtime) {
    return pal4::D3dFaceTraceReachability::CanReach(
        0x10u, {1.0f, 2.0f, 3.0f}, {4.0f, 5.0f, 6.0f}, runtime);
}

void TestTerminalStatuses() {
    for (const auto status : {0u, 1u, 3u, 5u}) {
        Runtime runtime;
        runtime.steps = {{0x10u, status, 0x20u}};
        Check(Run(runtime) == (status == 1u) && runtime.next == 1u,
              "only terminal status one is reachable");
    }
    Runtime runtime;
    runtime.steps = {{0x10u, 2u, 0u}};
    Check(!Run(runtime) && runtime.next == 1u,
          "crossing without a neighbor is unreachable");
}

void TestNeighborChain() {
    Runtime runtime;
    runtime.steps = {{0x10u, 2u, 0x20u},
                     {0x20u, 2u, 0x30u},
                     {0x30u, 1u, 0u}};
    Check(Run(runtime) && runtime.next == 3u,
          "successive status-two hits follow neighbor pointers");
    pal4::D3dFaceTraceReachabilityRuntimeStub stub;
    Check(!pal4::D3dFaceTraceReachability::CanReach(
              0x10u, {1.0f, 2.0f, 3.0f},
              {4.0f, 5.0f, 6.0f}, stub),
          "trace stub is linkable and returns false");
}

}  // namespace

int main() {
    TestTerminalStatuses();
    TestNeighborChain();
}
