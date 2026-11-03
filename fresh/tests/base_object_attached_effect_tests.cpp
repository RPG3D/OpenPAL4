#include "pal4/base_object_attached_effect.h"

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <utility>
#include <vector>

namespace {

using Event = std::pair<char, std::uint32_t>;

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IBaseObjectAttachedEffectRuntime {
    std::vector<Event> events;
    bool rotated{};
    bool needs_update{true};

    std::uint32_t Offset() const { return rotated ? 0x53cu : 0x534u; }

    std::uint32_t ReadDword(std::uint32_t address) override {
        Check(address == 0x4004u, "effect reads visual frame pointer");
        events.emplace_back('D', address);
        return 0x5000u;
    }
    std::uint32_t GetObjectTransform(std::uint32_t frame) override {
        Check(!rotated && frame == 0x5000u,
              "ordinary effect gets its frame transform");
        events.emplace_back('T', frame);
        return 0x7000u;
    }
    bool EffectNeedsUpdate(pal4::BaseObjectState&,
                           std::uint32_t offset) override {
        Check(offset == Offset(), "readiness checks selected handle");
        events.emplace_back('?', offset);
        return needs_update;
    }
    void UpdateEffect(pal4::BaseObjectState&,
                      std::uint32_t offset) override {
        events.emplace_back('U', offset);
    }
    std::uint32_t EffectSingleton() override {
        events.emplace_back('S', 0u);
        return 0x8000u;
    }
    pal4::BaseObjectEffectCreation CreateEffectObject(
        std::uint32_t singleton, std::uint32_t name,
        std::uint32_t position, std::uint32_t mode,
        std::uint32_t flag) override {
        Check(singleton == 0x8000u && name == 0u &&
                  position == (rotated ? 0x84b1dcu : 0x7030u) &&
                  mode == 0x1234u && flag == 1u,
              "effect creator receives the exact mode and position");
        events.emplace_back('C', position);
        return {{0x111u, 0x222u}, {0x333u, 0x444u}};
    }
    void ResetTemporary(pal4::BaseObjectEffectHandle& handle) override {
        Check(handle.first == 0x333u && handle.second == 0x444u,
              "temporary effect handle is reset after transfer");
        events.emplace_back('R', 0u);
    }
    void SetVisibility(pal4::BaseObjectState& state,
                       std::uint32_t offset,
                       std::uint32_t frame) override {
        Check(!rotated && offset == 0x534u && frame == 0x5000u &&
                  state.Read(offset) == 0x111u &&
                  state.Read(offset + 4u) == 0x222u,
              "ordinary effect visibility follows handle transfer");
        events.emplace_back('V', frame);
    }
    void SetRotation(pal4::BaseObjectState& state,
                     std::uint32_t offset, float x, float y, float z,
                     std::uint8_t flag) override {
        Check(rotated && offset == 0x53cu && x == 90.0f &&
                  y == 0.0f && z == 0.0f && flag == 0u &&
                  state.Read(offset) == 0x111u &&
                  state.Read(offset + 4u) == 0x222u,
              "rotated effect applies the observed Euler vector and zero flag");
        events.emplace_back('A', offset);
    }
    void BindFrame(pal4::BaseObjectState&,
                   std::uint32_t offset,
                   std::uint32_t frame) override {
        Check(rotated && offset == 0x53cu && frame == 0x5000u,
              "rotated effect binds to the visual frame");
        events.emplace_back('B', frame);
    }
    void RefreshEffect(pal4::BaseObjectState&,
                       std::uint32_t offset) override {
        events.emplace_back('F', offset);
    }
};

std::uint32_t Setup(pal4::BaseObjectState& state, Runtime& runtime) {
    return runtime.rotated ?
        pal4::BaseObjectAttachedEffect::SetupRotated(
            state, 0u, 0x1234u, runtime) :
        pal4::BaseObjectAttachedEffect::Setup(
            state, 0u, 0x1234u, runtime);
}

void CheckSetup(bool rotated) {
    pal4::BaseObjectState state{};
    Runtime runtime;
    runtime.rotated = rotated;
    Check(Setup(state, runtime) == 0u && runtime.events.empty(),
          "inactive object exits before runtime calls");
    state.Write(0x1f0u, 1u);
    Check(Setup(state, runtime) == 0u && runtime.events.empty(),
          "missing visual exits before runtime calls");
    state.Write(0x530u, 0x4000u);
    const auto offset = runtime.Offset();
    Check(Setup(state, runtime) == 1u &&
              state.Read(offset) == 0x111u &&
              state.Read(offset + 4u) == 0x222u,
          "creation returns one and stores both handle words");
    const std::vector<Event> expected = rotated ?
        std::vector<Event>{{'?', offset}, {'U', offset}, {'S', 0u},
                           {'C', 0x84b1dcu}, {'R', 0u}, {'A', offset},
                           {'D', 0x4004u}, {'B', 0x5000u}, {'F', offset}} :
        std::vector<Event>{{'D', 0x4004u}, {'T', 0x5000u},
                           {'?', offset}, {'U', offset}, {'S', 0u},
                           {'C', 0x7030u}, {'R', 0u}, {'D', 0x4004u},
                           {'V', 0x5000u}, {'F', offset}};
    Check(runtime.events == expected,
          "creation follows IDA position, readiness and binding order");
}

void CheckUpdates() {
    pal4::BaseObjectState state{};
    Runtime runtime;
    runtime.needs_update = false;
    Check(pal4::BaseObjectAttachedEffect::Update(state, runtime) == 1u &&
              runtime.events == std::vector<Event>{{'?', 0x534u}},
          "ordinary update skips cleanup when not ready");
    runtime.events.clear();
    runtime.rotated = true;
    runtime.needs_update = true;
    Check(pal4::BaseObjectAttachedEffect::UpdateRotated(state, runtime) == 1u &&
              runtime.events == std::vector<Event>{{'?', 0x53cu},
                                                   {'U', 0x53cu}},
          "rotated update uses its own handle and returns one");
}

}  // namespace

int main() {
    CheckSetup(false);
    CheckSetup(true);
    CheckUpdates();
}
