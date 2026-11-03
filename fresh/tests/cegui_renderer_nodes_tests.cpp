#include "pal4/cegui_renderer_nodes.h"

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

class Runtime final : public pal4::CeguiRendererNodeStub {
public:
    std::vector<std::uint32_t> allocation_results;
    std::vector<std::uint32_t> allocation_sizes;
    std::vector<std::uint32_t> freed;
    std::vector<std::string> events;
    std::uint32_t configured_object = 0u;
    std::uint32_t configured_first = 0u;
    std::uint32_t configured_second = 0u;
    std::uint32_t zeroed_buffer = 0u;
    std::uint32_t zeroed_bytes = 0u;
    std::uint32_t grid_object = 0u;
    std::uint32_t grid_buffer = 0u;
    std::int32_t grid_width = 0;
    std::int32_t grid_height = 0;
    std::vector<std::pair<std::uint32_t, std::uint32_t>> destroyed;
    bool throw_in_constructor = false;
    bool throw_in_grid = false;

    std::uint32_t Allocate(const std::uint32_t bytes) override {
        events.emplace_back("allocate");
        allocation_sizes.push_back(bytes);
        return allocation_results.at(allocation_sizes.size() - 1u);
    }
    void Free(const std::uint32_t address) override {
        events.emplace_back("free");
        freed.push_back(address);
    }
    std::uint32_t ConstructObject36(
        const std::uint32_t address,
        pal4::CeguiRendererState& /*owner*/) override {
        events.emplace_back("construct");
        if (throw_in_constructor) {
            throw std::runtime_error("constructor failed");
        }
        return address;
    }
    std::uint32_t ReadDword(const std::uint32_t address) override {
        events.emplace_back("read");
        return pal4::CeguiRendererNodeStub::ReadDword(address);
    }
    void WriteDword(const std::uint32_t address,
                    const std::uint32_t value) override {
        events.emplace_back("write");
        pal4::CeguiRendererNodeStub::WriteDword(address, value);
    }
    void ConfigureObject(const std::uint32_t object,
                         const std::uint32_t first,
                         const std::uint32_t second) override {
        events.emplace_back("configure");
        configured_object = object;
        configured_first = first;
        configured_second = second;
    }
    std::int32_t FloatToInt(const float value) override {
        events.emplace_back("convert");
        return pal4::CeguiRendererNodeStub::FloatToInt(value);
    }
    void ZeroBuffer(const std::uint32_t address,
                    const std::uint32_t bytes) override {
        events.emplace_back("zero");
        zeroed_buffer = address;
        zeroed_bytes = bytes;
    }
    void ApplyGrid(const std::uint32_t object,
                   const std::uint32_t buffer,
                   const std::int32_t width,
                   const std::int32_t height) override {
        events.emplace_back("grid");
        grid_object = object;
        grid_buffer = buffer;
        grid_width = width;
        grid_height = height;
        if (throw_in_grid) {
            throw std::runtime_error("grid operation failed");
        }
    }
    std::uint32_t DestroyObject(
        const std::uint32_t object,
        const std::uint32_t flags) override {
        events.emplace_back("destroy");
        destroyed.emplace_back(object, flags);
        return 0x55u;
    }
};

void InitializeSentinel(pal4::CeguiRendererState& state,
                        Runtime& runtime,
                        const std::uint32_t previous = 0x5000u) {
    state.WriteDword(236u, 0x5000u);
    runtime.WriteDword(0x5000u, 0x5000u);
    runtime.WriteDword(0x5004u, previous);
    runtime.events.clear();
}

void FirstNodeLinksBothSentinelDirections() {
    pal4::CeguiRendererState state;
    Runtime runtime;
    InitializeSentinel(state, runtime);
    runtime.allocation_results = {0x6000u, 0x7000u};
    Check(pal4::CeguiRendererNodes::Create(state, runtime) == 0x6000u &&
              runtime.allocation_sizes ==
                  std::vector<std::uint32_t>{36u, 12u} &&
              runtime.ReadDword(0x5000u) == 0x7000u &&
              runtime.ReadDword(0x5004u) == 0x7000u &&
              runtime.ReadDword(0x7000u) == 0x5000u &&
              runtime.ReadDword(0x7004u) == 0x5000u &&
              runtime.ReadDword(0x7008u) == 0x6000u &&
              state.ReadDword(240u) == 1u,
          "first node links to sentinel and stores constructed object");
    Check(runtime.events[0] == "allocate" &&
              runtime.events[1] == "construct" &&
              runtime.events[2] == "read" &&
              runtime.events[3] == "allocate",
          "object construction precedes node allocation and linking");
}

void SecondNodePreservesForwardAndBackwardLinks() {
    pal4::CeguiRendererState state;
    Runtime runtime;
    InitializeSentinel(state, runtime);
    runtime.allocation_results = {0x6000u, 0x7000u,
                                  0x6100u, 0x7100u};
    static_cast<void>(pal4::CeguiRendererNodes::Create(state, runtime));
    static_cast<void>(pal4::CeguiRendererNodes::Create(state, runtime));
    Check(runtime.ReadDword(0x5000u) == 0x7000u &&
              runtime.ReadDword(0x7000u) == 0x7100u &&
              runtime.ReadDword(0x7100u) == 0x5000u &&
              runtime.ReadDword(0x5004u) == 0x7100u &&
              runtime.ReadDword(0x7104u) == 0x7000u &&
              runtime.ReadDword(0x7108u) == 0x6100u &&
              state.ReadDword(240u) == 2u,
          "second node appends before sentinel and increments count");
}

void NullObjectAllocationStillAddsNode() {
    pal4::CeguiRendererState state;
    Runtime runtime;
    InitializeSentinel(state, runtime, 0u);
    runtime.allocation_results = {0u, 0x7000u};
    Check(pal4::CeguiRendererNodes::Create(state, runtime) == 0u &&
              runtime.ReadDword(0x7000u) == 0x7000u &&
              runtime.ReadDword(0x7004u) == 0x7000u &&
              runtime.ReadDword(0x7008u) == 0u &&
              runtime.ReadDword(0x5004u) == 0x7000u &&
              state.ReadDword(240u) == 1u,
          "null object and null predecessor keep observed fallback writes");
    Check(runtime.events[0] == "allocate" &&
              runtime.events[1] == "read" &&
              runtime.events[2] == "allocate",
          "null object storage skips constructor");
}

void ConstructorExceptionFreesObjectStorage() {
    pal4::CeguiRendererState state;
    Runtime runtime;
    InitializeSentinel(state, runtime);
    runtime.allocation_results = {0x6000u};
    runtime.throw_in_constructor = true;
    bool threw = false;
    try {
        static_cast<void>(pal4::CeguiRendererNodes::Create(state, runtime));
    } catch (const std::runtime_error&) {
        threw = true;
    }
    Check(threw && runtime.freed == std::vector<std::uint32_t>{0x6000u} &&
              runtime.events == std::vector<std::string>{
                  "allocate", "construct", "free"} &&
              state.ReadDword(240u) == 0u,
          "constructor exception releases storage before any node mutation");
}

void ConfigureWrapperReturnsCreatedObject() {
    pal4::CeguiRendererState state;
    Runtime runtime;
    InitializeSentinel(state, runtime);
    runtime.allocation_results = {0x6000u, 0x7000u};
    Check(pal4::CeguiRendererNodes::CreateAndConfigure(
              state, 0x1111u, 0x2222u, runtime) == 0x6000u &&
              runtime.configured_object == 0x6000u &&
              runtime.configured_first == 0x1111u &&
              runtime.configured_second == 0x2222u &&
              runtime.events.back() == "configure",
          "wrapper configures the vtable-created object and returns it");
}

void GridCreationZerosSquareBeforeObjectCall() {
    pal4::CeguiRendererState state;
    Runtime runtime;
    InitializeSentinel(state, runtime);
    runtime.allocation_results = {0x6000u, 0x7000u, 0x8000u};
    Check(pal4::CeguiRendererNodes::CreateZeroGrid(
              state, 3.9f, runtime) == 0x6000u &&
              runtime.allocation_sizes ==
                  std::vector<std::uint32_t>{36u, 12u, 36u} &&
              runtime.zeroed_buffer == 0x8000u &&
              runtime.zeroed_bytes == 36u &&
              runtime.grid_object == 0x6000u &&
              runtime.grid_buffer == 0x8000u &&
              runtime.grid_width == 3 && runtime.grid_height == 3 &&
              runtime.freed == std::vector<std::uint32_t>{0x8000u},
          "grid call uses converted side squared and releases buffer");
    for (std::uint32_t index = 0u; index < 9u; ++index) {
        Check(runtime.ReadDword(0x8000u + 4u * index) == 0u,
              "every grid DWORD was zeroed before external call");
    }
    const auto zero = std::find(runtime.events.begin(),
                                runtime.events.end(), "zero");
    Check(zero != runtime.events.end() &&
              std::next(zero) != runtime.events.end() &&
              *std::next(zero) == "grid",
          "redundant zero pass immediately precedes grid callback");
}

void GridCallbackExceptionReleasesTemporaryBuffer() {
    pal4::CeguiRendererState state;
    Runtime runtime;
    InitializeSentinel(state, runtime);
    runtime.allocation_results = {0x6000u, 0x7000u, 0x8000u};
    runtime.throw_in_grid = true;
    bool threw = false;
    try {
        static_cast<void>(pal4::CeguiRendererNodes::CreateZeroGrid(
            state, 1.0f, runtime));
    } catch (const std::runtime_error&) {
        threw = true;
    }
    Check(threw && runtime.freed ==
              std::vector<std::uint32_t>{0x8000u} &&
              state.ReadDword(240u) == 1u &&
              runtime.events.back() == "free",
          "grid exception frees temporary array but retains created object");
}

void RemoveUnlinksAllMatchingNodesThenDestroysOnce() {
    pal4::CeguiRendererState state;
    Runtime runtime;
    InitializeSentinel(state, runtime);
    runtime.allocation_results = {0x6000u, 0x7000u,
                                  0x6100u, 0x7100u};
    static_cast<void>(pal4::CeguiRendererNodes::Create(state, runtime));
    static_cast<void>(pal4::CeguiRendererNodes::Create(state, runtime));
    runtime.WriteDword(0x7108u, 0x6000u);
    runtime.events.clear();
    Check(pal4::CeguiRendererNodes::Remove(
              state, 0x6000u, runtime) == 0x55u &&
              state.ReadDword(240u) == 0u &&
              runtime.ReadDword(0x5000u) == 0x5000u &&
              runtime.ReadDword(0x5004u) == 0x5000u &&
              runtime.freed ==
                  std::vector<std::uint32_t>{0x7000u, 0x7100u} &&
              runtime.destroyed ==
                  std::vector<std::pair<std::uint32_t, std::uint32_t>>{
                      {0x6000u, 1u}},
          "remove unlinks every matching node but destroys object once");
    const auto free = std::find(runtime.events.begin(),
                                runtime.events.end(), "free");
    const auto destroy = std::find(runtime.events.begin(),
                                   runtime.events.end(), "destroy");
    Check(free != runtime.events.end() && destroy != runtime.events.end() &&
              free < destroy,
          "node release precedes object destruction");
}

void RemoveNullAndAbsentObjects() {
    pal4::CeguiRendererState state;
    Runtime runtime;
    InitializeSentinel(state, runtime);
    Check(!pal4::CeguiRendererNodes::Remove(
              state, 0u, runtime).has_value() &&
              runtime.events.empty(),
          "null object takes the no-effect return path");
    Check(pal4::CeguiRendererNodes::Remove(
              state, 0x9999u, runtime) == 0x55u &&
              runtime.destroyed ==
                  std::vector<std::pair<std::uint32_t, std::uint32_t>>{
                      {0x9999u, 1u}},
          "absent non-null object is still passed to destructor slot");
}

void DrainProcessesFirstPayloadUntilCountIsZero() {
    pal4::CeguiRendererState state;
    Runtime runtime;
    InitializeSentinel(state, runtime);
    runtime.allocation_results = {0x6000u, 0x7000u,
                                  0x6100u, 0x7100u};
    static_cast<void>(pal4::CeguiRendererNodes::Create(state, runtime));
    static_cast<void>(pal4::CeguiRendererNodes::Create(state, runtime));
    runtime.events.clear();
    Check(pal4::CeguiRendererNodes::Drain(state, runtime) == 0u &&
              state.ReadDword(240u) == 0u &&
              runtime.freed ==
                  std::vector<std::uint32_t>{0x7000u, 0x7100u} &&
              runtime.destroyed ==
                  std::vector<std::pair<std::uint32_t, std::uint32_t>>{
                      {0x6000u, 1u}, {0x6100u, 1u}},
          "drain removes the first node through virtual delete each pass");
    runtime.events.clear();
    Check(pal4::CeguiRendererNodes::Drain(state, runtime) == 0u &&
              runtime.events.empty(),
          "already-empty collection returns without callbacks");
}

}  // namespace

int main() {
    FirstNodeLinksBothSentinelDirections();
    SecondNodePreservesForwardAndBackwardLinks();
    NullObjectAllocationStillAddsNode();
    ConstructorExceptionFreesObjectStorage();
    ConfigureWrapperReturnsCreatedObject();
    GridCreationZerosSquareBeforeObjectCall();
    GridCallbackExceptionReleasesTemporaryBuffer();
    RemoveUnlinksAllMatchingNodesThenDestroysOnce();
    RemoveNullAndAbsentObjects();
    DrainProcessesFirstPayloadUntilCountIsZero();
    std::cout << "10 CEGUI renderer node test cases passed\n";
}
