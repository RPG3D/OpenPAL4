#include "pal4/cylinder_object.h"

#include <array>
#include <bit>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

constexpr std::array<std::uint32_t, 10> kBase{
    0x84a1dcu, 0x84a1a0u, 0x84a194u, 0x84a1d0u, 0x84a1c8u,
    0x84a1bcu, 0x84a1acu, 0x84a184u, 0x84a408u, 0x84a178u};
constexpr std::array<std::uint32_t, 10> kDerived{
    0x84a4e0u, 0x84a4d4u, 0x84a4c8u, 0x84a4bcu, 0x84a4b4u,
    0x84a4a8u, 0x84a498u, 0x84a488u, 0x84a470u, 0x84a464u};

void CheckFacets(const pal4::CylinderObjectState& state,
                 const std::array<std::uint32_t, 10>& expected) {
    for (std::uint32_t i = 0u; i < expected.size(); ++i)
        Check(state.words[5u + i] == expected[i],
              "cylinder facet offset and vtable");
}

struct Runtime final : pal4::ICylinderObjectRuntime {
    std::vector<std::string> calls;
    std::uint32_t count = 9u;
    bool fail_instance = false;
    std::array<std::uint32_t, 3> forwarded{};
    std::uint32_t forwarded_mode = 99u;

    std::uint32_t SystemInstance() override {
        calls.push_back("instance");
        if (fail_instance)
            throw std::runtime_error("instance");
        return 0x5000u;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        Check(address == 0x50a4u, "instance count address");
        calls.push_back("read");
        return count;
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        Check(address == 0x50a4u, "instance count write");
        calls.push_back("write");
        count = value;
    }
    void FreeBuffer(std::uint32_t address) override {
        calls.push_back("free_buffer:" + std::to_string(address));
    }
    void FreeObject(pal4::CylinderObjectState& state) override {
        Check(state.words[0u] == 0x84b240u,
              "deleting destructor frees after base restoration");
        calls.push_back("free_object");
    }
    std::uint32_t ForwardAction(
        pal4::CylinderObjectState& state,
        const std::array<std::uint32_t, 3>& args,
        std::uint32_t mode) override {
        Check(state.words[0u] == 0x84a4f0u,
              "facet dispatch keeps derived owner");
        forwarded = args;
        forwarded_mode = mode;
        calls.push_back("forward");
        return 0xdeadbeefu;
    }
};

struct AccessRuntime final : pal4::ICylinderObjectAccessRuntime {
    std::vector<std::uint32_t> addresses;
    std::uint32_t ReadWord(std::uint32_t address) override {
        addresses.push_back(address);
        return address == 0x2008u ? 0x1234u : 0x5678u;
    }
};

struct PlaybackRuntime final : pal4::ICylinderObjectPlaybackRuntime {
    std::uint32_t queries = 0u;
    std::uint32_t durations = 0u;
    void QueryTaskValues(pal4::CylinderObjectState& state,
                         std::uint32_t facet_offset,
                         std::array<std::int32_t, 4>& values) override {
        Check(state.words[1u] == 1u && facet_offset == 44u,
              "timed query uses cylinder color facet");
        ++queries;
        values = {1, 2, 3, 15};
    }
    double Duration(pal4::CylinderObjectState&) override {
        ++durations;
        return 4.0;
    }
};

void TestLifecycle() {
    pal4::CylinderObjectState state{};
    state.words.fill(0xa5a5a5a5u);
    Runtime runtime;
    Check(&pal4::CylinderObject::Construct(state, 0x5au, runtime) ==
              &state && runtime.count == 10u &&
              runtime.calls == std::vector<std::string>{
                  "instance", "read", "write"} &&
              state.words[0u] == 0x84a4f0u,
          "constructor sets derived owner and increments system count");
    CheckFacets(state, kDerived);
    for (std::uint32_t i = 1u; i <= 4u; ++i)
        Check(state.words[i] == 0u, "base reset fields");
    Check(state.words[22u] == 0u && state.words[23u] == 0u &&
              state.words[35u] == 0xa5a5a55au &&
              state.words[36u] == 0u && state.words[37u] == 0u &&
              state.words[38u] == 0u && state.words[17u] == 0xa5a5a5a5u,
          "constructor writes exact fields and stack-derived byte");
    Check(pal4::CylinderObject::ForwardAction(
              state, {11u, 22u, 33u}, runtime) == 0xdeadbeefu &&
              runtime.forwarded == std::array<std::uint32_t, 3>{
                  11u, 22u, 33u} && runtime.forwarded_mode == 0u,
          "facet at +8 forwards three arguments with mode zero");
    state.words[36u] = 0x3000u;
    runtime.calls.clear();
    pal4::CylinderObject::Destroy(state, runtime);
    Check(runtime.count == 9u &&
              runtime.calls == std::vector<std::string>{
                  "instance", "read", "write", "free_buffer:12288"} &&
              state.words[0u] == 0x84b240u &&
              state.words[36u] == 0u && state.words[37u] == 0u &&
              state.words[38u] == 0u,
          "destructor decrements count, frees buffer and clears range");
    CheckFacets(state, kBase);
}

void TestDeletingDestructor() {
    pal4::CylinderObjectState state{};
    Runtime runtime;
    static_cast<void>(pal4::CylinderObject::Construct(state, 0u, runtime));
    runtime.calls.clear();
    Check(pal4::CylinderObject::DeletingDestroy(
              state, 2u, runtime) ==
              reinterpret_cast<std::uintptr_t>(&state) &&
              runtime.calls == std::vector<std::string>{
                  "instance", "read", "write", "free_buffer:0"},
          "deleting destructor without bit zero retains storage");
    static_cast<void>(pal4::CylinderObject::Construct(state, 0u, runtime));
    runtime.calls.clear();
    Check(pal4::CylinderObject::DeletingDestroy(
              state, 3u, runtime) ==
              reinterpret_cast<std::uintptr_t>(&state) &&
              runtime.calls == std::vector<std::string>{
                  "instance", "read", "write", "free_buffer:0",
                  "free_object"},
          "deleting destructor frees after normal destruction");
}

void TestConstructorUnwind() {
    pal4::CylinderObjectState state{};
    Runtime runtime;
    runtime.fail_instance = true;
    try {
        static_cast<void>(pal4::CylinderObject::Construct(
            state, 7u, runtime));
        Check(false, "constructor must throw");
    } catch (const std::runtime_error&) {
        Check(runtime.calls == std::vector<std::string>{
                  "instance", "free_buffer:0"} &&
                  state.words[0u] == 0x84b240u,
              "constructor unwind frees local buffer and base facets");
        CheckFacets(state, kBase);
    }
}

void TestAccessAndPlayback() {
    pal4::CylinderObjectState state{};
    state.words[22u] = 0x2000u;
    AccessRuntime access;
    PlaybackRuntime playback;
    Check(pal4::CylinderObject::ResourceHandle(state, access) == 0u &&
              pal4::CylinderObject::Resource(state, access) == 0u &&
              access.addresses.empty() &&
              pal4::CylinderObject::ClassName() == "CylinderObject",
          "inactive access is gated and class name matches IDA string");
    state.words[1u] = 0x100u;
    pal4::CylinderObject::SetTimedMode(state, 1u, playback);
    Check(playback.queries == 0u && state.words[2u] == 0u,
          "timed mode checks active low byte");

    state.words[1u] = 1u;
    Check(pal4::CylinderObject::ResourceHandle(state, access) == 0x1234u &&
              pal4::CylinderObject::Resource(state, access) == 0x5678u &&
              access.addresses == std::vector<std::uint32_t>{
                  0x2008u, 0x2004u},
          "active access reads distinct resource offsets");
    state.words[2u] = 0x55abcdefu;
    pal4::CylinderObject::SetTimedMode(state, 1u, playback);
    Check(state.words[2u] == 0x01abcdefu &&
              std::bit_cast<float>(state.words[24u]) == 3.75f &&
              playback.queries == 1u && playback.durations == 1u,
          "enabling timed mode preserves neighboring bytes and stores rate");
    pal4::CylinderObject::SetTimedMode(state, 1u, playback);
    Check(playback.queries == 1u,
          "unchanged timed mode avoids repeated native calls");
    pal4::CylinderObject::SetTimedMode(state, 0x100u, playback);
    Check(state.words[2u] == 0x00abcdefu && playback.queries == 1u,
          "mode parameter truncates to byte and disable keeps old rate");
}

}  // namespace

int main() {
    TestLifecycle();
    TestDeletingDestructor();
    TestConstructorUnwind();
    TestAccessAndPlayback();
}
