#include "pal4/d3d_face_path.h"

#include <bit>
#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::ID3dFacePathRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::uint32_t> freed;
    std::vector<std::uint32_t> sizes;
    std::vector<std::string> events;
    std::uint32_t next_node = 0x2000u;

    std::uint32_t ReadWord(const std::uint32_t address) override {
        events.push_back("read:" + std::to_string(address));
        return words.at(address);
    }
    void WriteWord(const std::uint32_t address,
                   const std::uint32_t value) override {
        events.push_back("write:" + std::to_string(address));
        words[address] = value;
    }
    std::uint32_t Allocate(const std::uint32_t size) override {
        sizes.push_back(size);
        const auto allocated = next_node;
        next_node += 0x100u;
        return allocated;
    }
    void Free(const std::uint32_t address) override {
        freed.push_back(address);
    }

    void InitializeSentinel() {
        words[0x1000u] = 0x1000u;
        words[0x1004u] = 0x1000u;
    }
};

struct TraceRuntime final : pal4::ID3dFaceTraceReachabilityRuntime {
    std::vector<float> destinations;
    std::vector<std::uint32_t> faces;
    float last_reachable_x = 0.0f;

    std::uint32_t Trace(const std::uint32_t face,
                        const pal4::D3dLine2State& line,
                        std::uint32_t&) override {
        faces.push_back(face);
        destinations.push_back(line.end.x);
        return line.end.x <= last_reachable_x ? 1u : 0u;
    }
};

void TestPointCopyAndAddress() {
    const pal4::D3dFacePathPoint input{{1.0f, 2.0f, 3.0f}, 0x1234u};
    pal4::D3dFacePathPoint output{};
    pal4::D3dFacePath::CopyPoint(&output, input);
    Check(output.position.x == 1.0f && output.position.y == 2.0f &&
              output.position.z == 3.0f && output.face == 0x1234u,
          "node copy includes all vector components and face word");
    pal4::D3dFacePath::CopyPoint(nullptr, input);
    Check(pal4::D3dFacePath::ListFieldsAddress(0x3000u) == 0x3024u,
          "list accessor returns native +36 address");
}

void TestResetAppendAndRelease() {
    Runtime runtime;
    runtime.InitializeSentinel();
    pal4::D3dFacePathState state{};
    state.words.fill(0xa5a5a5a5u);
    state.words[10u] = 0x1000u;
    state.words[11u] = 0u;
    const pal4::D3dTriangleVector3 start{1.0f, 2.0f, 3.0f};
    const pal4::D3dTriangleVector3 end{7.0f, 8.0f, 9.0f};
    Check(pal4::D3dFacePath::Reset(
              state, 0x55u, start, 0x1111u, end, 0x9999u, runtime) == 1u,
          "reset appends initial path point and returns count");
    Check(state.words[0u] == 0x55u && state.words[1u] ==
              std::bit_cast<std::uint32_t>(1.0f) &&
              state.words[2u] == std::bit_cast<std::uint32_t>(2.0f) &&
              state.words[3u] == std::bit_cast<std::uint32_t>(3.0f) &&
              state.words[4u] == 0x1111u &&
              state.words[5u] == std::bit_cast<std::uint32_t>(7.0f) &&
              state.words[6u] == std::bit_cast<std::uint32_t>(8.0f) &&
              state.words[7u] == std::bit_cast<std::uint32_t>(9.0f) &&
              state.words[8u] == 0x9999u &&
              state.words[9u] == 0xa5a5a5a5u,
          "reset writes storage and path fields, preserving +36 word");
    Check(runtime.words.at(0x1000u) == 0x2000u &&
              runtime.words.at(0x1004u) == 0x2000u &&
              runtime.words.at(0x2000u) == 0x1000u &&
              runtime.words.at(0x2004u) == 0x1000u &&
              runtime.words.at(0x2008u) ==
                  std::bit_cast<std::uint32_t>(1.0f) &&
              runtime.words.at(0x2014u) == 0x1111u,
          "first 24-byte node links to sentinel and stores start payload");

    Check(pal4::D3dFacePath::Append(
              state, {4.0f, 5.0f, 6.0f}, 0x2222u, runtime) == 2u &&
              pal4::D3dFacePath::AppendEnd(state, runtime) == 3u,
          "middle and terminal points increase count");
    Check(runtime.sizes == std::vector<std::uint32_t>{24u, 24u, 24u} &&
              runtime.words.at(0x2000u) == 0x2100u &&
              runtime.words.at(0x2100u) == 0x2200u &&
              runtime.words.at(0x2200u) == 0x1000u &&
              runtime.words.at(0x1000u) == 0x2000u &&
              runtime.words.at(0x1004u) == 0x2200u,
          "nodes form a circular list in append order");
    Check(runtime.words.at(0x2108u) ==
              std::bit_cast<std::uint32_t>(4.0f) &&
              runtime.words.at(0x2114u) == 0x2222u &&
              runtime.words.at(0x2208u) ==
                  std::bit_cast<std::uint32_t>(7.0f) &&
              runtime.words.at(0x2214u) == 0x9999u,
          "middle and end nodes preserve their own point and face");

    runtime.events.clear();
    Check(pal4::D3dFacePath::Reset(
              state, 0x66u, end, 0x3333u, start, 0x4444u, runtime) == 1u &&
              runtime.freed ==
                  std::vector<std::uint32_t>{0x2000u, 0x2100u, 0x2200u} &&
              runtime.words.at(0x1000u) == 0x2300u &&
              runtime.words.at(0x1004u) == 0x2300u &&
              runtime.words.at(0x2314u) == 0x3333u,
          "reset unlinks and frees old nodes before appending new start");
}

void TestReachableSkip() {
    Runtime runtime;
    runtime.InitializeSentinel();
    pal4::D3dFacePathState state{};
    state.words[10u] = 0x1000u;
    TraceRuntime trace;
    Check(pal4::D3dFacePath::SkipReachable(
              state, 0x1000u, runtime, trace) == 0x1000u &&
              trace.faces.empty(),
          "sentinel cursor returns immediately");
    static_cast<void>(pal4::D3dFacePath::Reset(
        state, 1u, {1.0f, 2.0f, 3.0f}, 0x11u,
        {9.0f, 2.0f, 3.0f}, 0x99u, runtime));
    Check(pal4::D3dFacePath::SkipReachable(
              state, 0x2000u, runtime, trace) == 0x1000u &&
              trace.faces.empty(),
          "last node returns sentinel without tracing");
    static_cast<void>(pal4::D3dFacePath::Append(
        state, {2.0f, 2.0f, 3.0f}, 0x22u, runtime));
    Check(pal4::D3dFacePath::SkipReachable(
              state, 0x2000u, runtime, trace) == 0x2100u &&
              trace.faces.empty(),
          "one successor returns that node without tracing");
    static_cast<void>(pal4::D3dFacePath::Append(
        state, {3.0f, 2.0f, 3.0f}, 0x33u, runtime));
    static_cast<void>(pal4::D3dFacePath::AppendEnd(state, runtime));
    trace.last_reachable_x = 3.0f;
    Check(pal4::D3dFacePath::SkipReachable(
              state, 0x2000u, runtime, trace) == 0x2200u &&
              trace.faces == std::vector<std::uint32_t>{0x11u, 0x11u} &&
              trace.destinations == std::vector<float>{3.0f, 9.0f},
          "reachable lookahead is accepted until first failed segment");
    trace.faces.clear();
    trace.destinations.clear();
    trace.last_reachable_x = 9.0f;
    Check(pal4::D3dFacePath::SkipReachable(
              state, 0x2000u, runtime, trace) == 0x2300u &&
              trace.faces == std::vector<std::uint32_t>{0x11u, 0x11u},
          "all reachable lookaheads return last node");
}

}  // namespace

int main() {
    TestPointCopyAndAddress();
    TestResetAppendAndRelease();
    TestReachableSkip();
}
