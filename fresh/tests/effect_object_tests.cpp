#include "pal4/effect_object.h"

#include <array>
#include <bit>
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

constexpr std::array<std::uint32_t, 8> kBase{
    0x84a1a0u, 0x84a194u, 0x84a1d0u, 0x84a1c8u,
    0x84a718u, 0x84a1bcu, 0x84a178u, 0x84a408u};
constexpr std::array<std::uint32_t, 8> kDerived{
    0x84a6b4u, 0x84a6a8u, 0x84a69cu, 0x84a694u,
    0x84a67cu, 0x84a670u, 0x84a664u, 0x84a64cu};

void CheckFacets(const pal4::EffectObjectState& state,
                 const std::array<std::uint32_t, 8>& facets) {
    for (std::size_t i = 0u; i < facets.size(); ++i)
        Check(state.words[5u + i] == facets[i], "facet vtable offset");
}

struct Runtime final : pal4::IEffectObjectRuntime {
    std::vector<std::string> calls;
    std::uint32_t count = 8u;
    bool fail_instance = false;
    std::uint32_t SystemInstance() override {
        calls.push_back("instance");
        if (fail_instance)
            throw std::runtime_error("instance unavailable");
        return 0x5000u;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        Check(address == 0x50a4u, "instance count offset is +164");
        calls.push_back("read_count");
        return count;
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        Check(address == 0x50a4u, "instance count write offset");
        calls.push_back("write_count");
        count = value;
    }
    void FreeObject(pal4::EffectObjectState& state) override {
        Check(state.words[0u] == 0x84b240u,
              "object is destroyed before storage is freed");
        calls.push_back("free_object");
    }
};

struct TransformRuntime final : pal4::IEffectObjectTransformRuntime {
    std::array<std::uint32_t, 3> received{};
    std::uint32_t calls = 0u;
    std::uint32_t CallFacet28Slot8(
        pal4::EffectObjectState& state,
        const std::array<std::uint32_t, 3>& values,
        std::uint32_t mode) override {
        Check(state.words[7u] == 0x84a69cu && mode == 0u,
              "forwarding targets owner +28 facet slot +8 in mode zero");
        received = values;
        ++calls;
        return 0x12345678u;
    }
};

struct PlaybackRuntime final : pal4::IEffectObjectPlaybackRuntime {
    std::uint32_t calls = 0u;
    double ResetAndMeasure(std::uint32_t resource,
                           std::uint8_t mode) override {
        Check(resource == 0x7000u && mode == 0u,
              "playback reset receives resource +92 in mode zero");
        ++calls;
        return 12.5;
    }
};

void TestLifecycle() {
    pal4::EffectObjectState state{};
    state.words.fill(0xaaaaaaaau);
    Runtime runtime;
    Check(&pal4::EffectObject::Construct(state, runtime) == &state &&
              state.words[0u] == 0x84a6c0u && runtime.count == 9u &&
              state.words[1u] == 0u && state.words[2u] == 0u &&
              state.words[3u] == 0u && state.words[4u] == 0u &&
              state.words[22u] == 0xaaaaaa00u &&
              state.words[24u] == 0u &&
              state.words[23u] == 0xaaaaaaaau &&
              state.words[25u] == 0xaaaaaaaau &&
              runtime.calls == std::vector<std::string>{
                  "instance", "read_count", "write_count"},
          "constructor installs derived vtables and preserves unrelated tail");
    CheckFacets(state, kDerived);

    Check(pal4::EffectObject::ResourceHandle(state) == 0u &&
              pal4::EffectObject::ClassName() == "EffectObject",
          "resource access and vtable class-name string");
    state.words[24u] = 0x76543210u;
    Check(pal4::EffectObject::ResourceHandle(state) == 0x76543210u,
          "resource access reads owner +96 without an activity gate");

    PlaybackRuntime playback;
    state.words[2u] = 0x12345678u;
    state.words[23u] = 0x7000u;
    state.words[25u] = 0xaaaaaaaau;
    pal4::EffectObjectPlayback::SetEnabled(state, 0xffu, playback);
    Check(state.words[2u] == 0x12345678u &&
              state.words[25u] == 0xaaaaaaaau && playback.calls == 0u,
          "inactive playback setter leaves mode and duration unchanged");
    state.words[1u] = 1u;
    pal4::EffectObjectPlayback::SetEnabled(state, 2u, playback);
    Check(state.words[2u] == 0x02345678u && playback.calls == 1u &&
              pal4::EffectObject::GetDuration(state) == 12.5f,
          "nonzero playback mode caches native duration at +100");
    pal4::EffectObjectPlayback::SetEnabled(state, 0u, playback);
    Check(state.words[2u] == 0x00345678u && state.words[25u] == 0u &&
              playback.calls == 1u &&
              pal4::EffectObject::GetDuration(state) == 0.0f,
          "playback disable clears mode and duration without native call");

    TransformRuntime transform;
    Check(pal4::EffectObjectTransform::ForwardFacet28(
              state, {1u, 0xabcdef01u, 3u}, transform) == 0x12345678u &&
              transform.calls == 1u &&
              transform.received ==
                  std::array<std::uint32_t, 3>{1u, 0xabcdef01u, 3u},
          "facet +40 forwards raw triple to facet +28");

    runtime.calls.clear();
    pal4::EffectObject::Destroy(state, runtime);
    Check(state.words[0u] == 0x84b240u && runtime.count == 8u &&
              runtime.calls == std::vector<std::string>{
                  "instance", "read_count", "write_count"},
          "destructor decrements count before restoring base vtable");
    CheckFacets(state, kBase);
}

void TestDeletingAndUnwind() {
    pal4::EffectObjectState state{};
    Runtime runtime;
    (void)pal4::EffectObject::Construct(state, runtime);
    runtime.calls.clear();
    Check(pal4::EffectObject::DeletingDestroy(
              state, 2u, runtime) ==
              reinterpret_cast<std::uintptr_t>(&state) &&
              runtime.calls.back() == "write_count" &&
              runtime.count == 8u,
          "deleting thunk retains storage without flag bit zero");
    (void)pal4::EffectObject::Construct(state, runtime);
    runtime.calls.clear();
    Check(pal4::EffectObject::DeletingDestroy(
              state, 3u, runtime) ==
              reinterpret_cast<std::uintptr_t>(&state) &&
              runtime.calls.back() == "free_object" &&
              runtime.count == 8u,
          "deleting thunk frees storage after destruction");
    runtime.fail_instance = true;
    try {
        (void)pal4::EffectObject::Construct(state, runtime);
        Check(false, "constructor must throw");
    } catch (const std::runtime_error&) {
        Check(state.words[0u] == 0x84b240u,
              "constructor unwind restores base vtable");
        CheckFacets(state, kBase);
    }
}

}  // namespace

int main() {
    TestLifecycle();
    TestDeletingAndUnwind();
}
