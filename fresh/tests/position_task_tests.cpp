#include "pal4/position_task.h"

#include <bit>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace {
void Check(bool ok, const char* message) {
    if (!ok) { std::cerr << "FAIL: " << message << '\n'; std::exit(1); }
}
void Float(pal4::PositionTaskState& state, std::size_t offset, float value) {
    state.words[offset / 4u] = std::bit_cast<std::uint32_t>(value);
}
float Float(const pal4::PositionTaskState& state, std::size_t offset) {
    return std::bit_cast<float>(state.words[offset / 4u]);
}
void Byte(pal4::PositionTaskState& state, std::size_t offset,
          std::uint8_t value) {
    const auto shift = 8u * (offset % 4u);
    auto& word = state.words[offset / 4u];
    word = (word & ~(0xffu << shift)) | (std::uint32_t{value} << shift);
}
std::uint8_t Byte(const pal4::PositionTaskState& state, std::size_t offset) {
    return static_cast<std::uint8_t>(
        state.words[offset / 4u] >> (8u * (offset % 4u)));
}
bool Close(float a, float b) { return std::fabs(a - b) < 0.0001f; }
struct MoveCall final {
    pal4::PositionVector value;
    bool incremental;
};
struct Runtime final : pal4::IPositionTaskRuntime {
    std::vector<std::string> calls;
    std::vector<MoveCall> moves;
    std::vector<pal4::PositionVector> faces;
    std::vector<std::uint8_t> control_values;
    bool base_complete = true;
    std::uint32_t face_facet = 0x7000u;
    std::uint32_t control_facet = 0x8000u;
    unsigned base_checks = 0u;
    unsigned freed = 0u;
    unsigned randoms = 0u;
    void ConstructBase(pal4::PositionTaskState& state) override {
        calls.emplace_back("construct"); state.words[1u] = 0x1234u;
    }
    void DestroyBase(pal4::PositionTaskState& state) override {
        Check(state.words[0u] == pal4::PositionTask::kVtable,
              "base sees derived vtable");
        calls.emplace_back("destroy");
    }
    void FreeObject(pal4::PositionTaskState&) override { ++freed; }
    bool BaseDurationComplete(const pal4::PositionTaskState&) override {
        ++base_checks; return base_complete;
    }
    std::uint32_t ResolvePositionFacet(std::uint32_t owner,
                                       std::string_view name) override {
        Check(owner == 0x5000u && name == "PositionTask", "position facet key");
        calls.emplace_back("resolve-position"); return 0x6000u;
    }
    pal4::PositionVector GetPosition(std::uint32_t facet) override {
        Check(facet == 0x6000u, "position facet");
        calls.emplace_back("get-position"); return {1.0f, 2.0f, 3.0f};
    }
    void Move(std::uint32_t facet, const pal4::PositionVector& value,
              bool incremental) override {
        Check(facet == 0x6000u, "move facet");
        calls.emplace_back("move"); moves.push_back({value, incremental});
    }
    std::uint32_t ResolveFaceFacet(std::uint32_t owner) override {
        Check(owner == 0x5000u, "face facet owner");
        calls.emplace_back("resolve-face"); return face_facet;
    }
    void Face(std::uint32_t facet,
              const pal4::PositionVector& direction) override {
        Check(facet == 0x7000u, "face facet");
        calls.emplace_back("face"); faces.push_back(direction);
    }
    void SetRotation(std::uint32_t facet,
                     const pal4::PositionVector& rotation) override {
        Check(facet == 0x7000u && rotation[0] == 15.0f,
              "fixed rotation dispatch");
        calls.emplace_back("rotate");
    }
    void ActivateFace(std::uint32_t facet) override {
        Check(facet == 0x7000u, "face activation");
        calls.emplace_back("activate-face");
    }
    std::uint32_t ResolveControlFacet(std::uint32_t owner) override {
        Check(owner == 0x5000u, "control facet owner");
        calls.emplace_back("resolve-control"); return control_facet;
    }
    std::uint8_t ControlEnabled(std::uint32_t facet) override {
        Check(facet == 0x8000u, "control status facet");
        calls.emplace_back("get-control"); return 2u;
    }
    void SetControlEnabled(std::uint32_t facet,
                           std::uint8_t enabled) override {
        Check(facet == 0x8000u, "control enable facet");
        calls.emplace_back("set-control"); control_values.push_back(enabled);
    }
    int RandomInt() override { ++randoms; return 0; }
};
void BaseState(pal4::PositionTaskState& state) {
    state.words.fill(0u);
    state.words[16u] = 0x5000u;
    Float(state, 20u, 2.0f);
    Float(state, 80u, 0.0f);
    Float(state, 88u, 5.0f);
    Float(state, 92u, 2.0f);
    Float(state, 96u, 3.0f);
    Float(state, 140u, 15.0f);
}
}  // namespace

int main() {
    pal4::PositionTaskState state{};
    state.words.fill(0xdeadbeefu);
    Runtime runtime;
    Check(&pal4::PositionTask::Construct(state, runtime) == &state &&
          sizeof(state) == 228u && state.words[0u] == 0x84b070u &&
          state.words[1u] == 0x1234u && Byte(state, 72u) == 0u &&
          state.words[21u] == 0u && state.words[38u] == 0u &&
          pal4::PositionTask::ClassName() == "PositionTask",
          "constructor and native layout");
    Byte(state, 188u, 0u);
    Check(pal4::PositionTask::IsDurationComplete(state, runtime) &&
          runtime.base_checks == 1u, "normal duration delegates to base");
    Byte(state, 188u, 1u);
    Check(!pal4::PositionTask::IsDurationComplete(state, runtime) &&
          runtime.base_checks == 1u, "rollback bypasses base completion");

    BaseState(state);
    Byte(state, 72u, 1u);
    Byte(state, 136u, 1u);
    Check(pal4::PositionTask::Start(state, runtime) == 1u &&
          state.words[38u] == 0x6000u && state.words[40u] == 0x8000u &&
          Byte(state, 156u) == 2u &&
          Float(state, 192u) == 1.0f && Float(state, 216u) == 1.0f &&
          Float(state, 76u) == 2.0f && Float(state, 100u) == 2.0f &&
          Float(state, 124u) == 5.0f &&
          runtime.faces.back()[0] == 5.0f &&
          runtime.control_values.back() == 1u,
          "result mode resolves facets and computes target velocity");
    Check(pal4::PositionTask::Update(state, 0.5f, runtime) == 1u &&
          runtime.moves.back().incremental &&
          Close(runtime.moves.back().value[0], 1.0f) &&
          Close(Float(state, 84u), 0.5f),
          "result mode integrates target displacement");
    Check(pal4::PositionTask::Update(state, 3.0f, runtime) == 1u &&
          Close(runtime.moves.back().value[0], 3.0f) &&
          Float(state, 84u) == 2.0f,
          "result mode clamps at expected time");
    Check(pal4::PositionTask::Stop(state, runtime) == 1u &&
          !runtime.moves.back().incremental &&
          runtime.moves.back().value[0] == 5.0f &&
          state.words[38u] == 0u && state.words[40u] == 0u &&
          runtime.control_values.back() == 2u,
          "stop snaps result and restores control");

    BaseState(state);
    Byte(state, 73u, 1u);
    Byte(state, 136u, 1u);
    Float(state, 204u, 9.0f);
    Check(pal4::PositionTask::Start(state, runtime) == 1u &&
          Float(state, 100u) == 5.0f && Float(state, 112u) == 2.0f &&
          runtime.faces.back()[0] == 500.0f,
          "velocity mode computes added velocity and faces forward");
    Check(pal4::PositionTask::Update(state, 1.0f, runtime) == 1u &&
          Float(state, 100u) == 7.0f &&
          runtime.moves.back().value[0] == 7.0f &&
          runtime.faces.back()[0] == 700.0f,
          "velocity mode updates speed before move and face");
    Check(pal4::PositionTask::Stop(state, runtime) == 1u &&
          runtime.moves.back().incremental,
          "velocity mode stop does not snap position");

    // IDA626C9A/62708A preserve every positive duration, including subsecond
    // motion, both at initial start and after a rollback returns to its origin.
    BaseState(state);
    Byte(state, 73u, 1u);
    Byte(state, 188u, 1u);
    Float(state, 20u, 0.5f);
    Float(state, 204u, 9.0f);
    Check(pal4::PositionTask::Start(state, runtime) == 1u &&
          Close(Float(state, 112u), 8.0f),
          "subsecond velocity transition uses its actual duration");
    Check(pal4::PositionTask::Update(state, 0.25f, runtime) == 1u &&
          Close(Float(state, 100u), 7.0f) &&
          Close(runtime.moves.back().value[0], 1.75f),
          "quarter-second movement reflects the original acceleration");
    (void)pal4::PositionTask::Update(state, 0.25f, runtime);
    Check(Byte(state, 189u) == 1u, "short transition reaches reverse boundary");
    (void)pal4::PositionTask::Update(state, 0.5f, runtime);
    Check(Byte(state, 189u) == 0u && Byte(state, 176u) == 1u &&
          Close(Float(state, 112u), 8.0f),
          "rollback recomputes acceleration with the same subsecond duration");
    (void)pal4::PositionTask::Stop(state, runtime);
    for(const auto duration : {0.0f, -0.5f}) {
        BaseState(state);
        Byte(state, 73u, 1u);
        Float(state, 20u, duration);
        Float(state, 204u, 9.0f);
        Check(pal4::PositionTask::Start(state, runtime) == 1u &&
              Close(Float(state, 112u), 4.0f),
              "nonpositive duration retains the original one-second fallback");
        (void)pal4::PositionTask::Stop(state, runtime);
    }

    BaseState(state);
    Byte(state, 188u, 1u);
    Float(state, 168u, 1.0f);
    Float(state, 180u, 1.0f);
    const auto random_before = runtime.randoms;
    Check(pal4::PositionTask::Start(state, runtime) == 1u &&
          runtime.randoms == random_before + 1u,
          "rollback randomizes duration at start");
    Check(pal4::PositionTask::Update(state, 2.0f, runtime) == 1u &&
          Byte(state, 189u) == 1u && Byte(state, 164u) == 1u &&
          Float(state, 100u) == -5.0f && Float(state, 84u) == 0.0f,
          "forward completion reverses velocity and begins first wait");
    const auto moved = runtime.moves.size();
    Check(pal4::PositionTask::Update(state, 0.5f, runtime) == 1u &&
          runtime.moves.size() == moved && Float(state, 172u) == 0.5f,
          "first rollback wait defers movement");
    Check(pal4::PositionTask::Update(state, 0.5f, runtime) == 1u &&
          runtime.moves.size() == moved && Float(state, 172u) == 1.0f,
          "wait accumulates to interval");
    Check(pal4::PositionTask::Update(state, 2.0f, runtime) == 1u &&
          Byte(state, 189u) == 0u && Byte(state, 176u) == 1u &&
          Float(state, 184u) == 0.0f &&
          !runtime.moves.back().incremental &&
          runtime.moves.back().value[0] == 1.0f &&
          runtime.randoms == random_before + 2u,
          "return completion restores initial position and second wait");
    const auto after_return = runtime.moves.size();
    Check(pal4::PositionTask::Update(state, 0.25f, runtime) == 1u &&
          runtime.moves.size() == after_return && Float(state, 184u) == 0.25f,
          "second rollback wait defers movement");

    state.words[0u] = 0u;
    runtime.calls.clear();
    Check(&pal4::PositionTask::Delete(state, 1u, runtime) == &state &&
          runtime.freed == 1u && state.words[0u] == 0x84b070u &&
          runtime.calls == std::vector<std::string>{"destroy"},
          "deleting destructor restores vtable then frees object");
}
