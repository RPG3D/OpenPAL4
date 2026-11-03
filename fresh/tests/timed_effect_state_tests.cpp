#include "pal4/timed_effect_state.h"

#include <cstddef>
#include <bit>
#include <cstdint>
#include <iostream>
#include <limits>
#include <map>
#include <string>
#include <stdexcept>
#include <string_view>
#include <utility>
#include <vector>

namespace {

void Check(const bool condition, const std::string_view message) {
    if (!condition) throw std::runtime_error(message.data());
}

class Memory final : public pal4::ITimedEffectMemory {
public:
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::uint32_t> writes;

    std::uint32_t Read32(const std::uint32_t address) override {
        return words.at(address);
    }

    void Write32(const std::uint32_t address,
                 const std::uint32_t value) override {
        writes.push_back(address);
        words[address] = value;
    }
};

class ManagerRuntime final : public pal4::ITimedEffectManagerRuntime {
public:
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::uint32_t> records;
    std::vector<std::uint32_t> cleaned;
    std::vector<std::uint32_t> destroyed;
    std::vector<std::uint32_t> allocations;
    std::vector<std::uint32_t> freed;
    std::uint32_t fail_init_at{};
    std::uint32_t fail_alloc_at{};

    std::uint32_t Read32(const std::uint32_t address) override {
        return words.at(address);
    }
    void Write32(const std::uint32_t address,
                 const std::uint32_t value) override {
        words[address] = value;
    }
    std::uint32_t CreateCameraRecord() override {
        const auto record = 0x3000u + 0x100u *
            static_cast<std::uint32_t>(records.size());
        records.push_back(record);
        return record;
    }
    bool InitializeCamera(std::uint32_t /*record*/) override {
        return fail_init_at == 0u || records.size() != fail_init_at;
    }
    void CleanupCamera(const std::uint32_t record) override {
        cleaned.push_back(record);
    }
    void DestroyCameraRecord(const std::uint32_t record) override {
        destroyed.push_back(record);
    }
    std::uint32_t AllocateArray(const std::uint32_t bytes) override {
        allocations.push_back(bytes);
        if (fail_alloc_at == allocations.size()) return 0u;
        return 0x5000u + 0x1000u *
            (static_cast<std::uint32_t>(allocations.size()) - 1u);
    }
    void FreeArray(const std::uint32_t address) override {
        freed.push_back(address);
    }
};

class RenderRuntime final : public pal4::ITimedEffectRenderRuntime {
public:
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::pair<std::uint32_t, std::uint32_t>> states;
    std::vector<std::array<pal4::CeguiRenderVertex, 4>> draws;
    std::vector<std::uint32_t> textures;
    std::uint32_t config_calls{};
    std::uint32_t manager_calls{};
    std::int32_t width = 640;
    std::int32_t height = 480;

    std::uint32_t Read32(const std::uint32_t address) override {
        return words.at(address);
    }
    void Write32(const std::uint32_t address,
                 const std::uint32_t value) override {
        words[address] = value;
    }
    std::int32_t ConfigWidth() override {
        ++config_calls;
        return width;
    }
    std::int32_t ConfigHeight() override {
        ++config_calls;
        return height;
    }
    std::uint32_t InitializeCameraSubsystem() override {
        ++manager_calls;
        return 0x1000u;
    }
    std::uint32_t ActiveCameraObject(const std::uint32_t manager) override {
        Check(manager == 0x1000u, "render uses camera manager result");
        return 0x2000u;
    }
    std::uint32_t ObjectInternalId(const std::uint32_t camera) override {
        Check(camera == 0x2000u, "render uses active camera object");
        return 0x4000u;
    }
    float CameraDepth(const std::uint32_t context) override {
        Check(context == 0x4000u, "render forwards camera internal id");
        return 2.0f;
    }
    float ActiveDepth() override { return 0.5f; }
    std::uint32_t TextureRaster(const std::uint32_t texture) override {
        textures.push_back(texture);
        return texture + 0x100u;
    }
    void SetRenderState(const std::uint32_t state,
                        const std::uint32_t value) override {
        states.push_back({state, value});
    }
    void DrawPrimitive(
        const std::uint32_t primitive,
        const std::array<pal4::CeguiRenderVertex, 4>& vertices,
        const std::uint32_t count) override {
        Check(primitive == 4u && count == 4u,
              "timed effect renders textured quad");
        draws.push_back(vertices);
    }
};

void LayoutMatchesObservedOffsets() {
    Check(offsetof(pal4::TimedEffectState, loaded) == 4u &&
              offsetof(pal4::TimedEffectState, interval) == 8u &&
              offsetof(pal4::TimedEffectState, completed_cycles) == 20u &&
              offsetof(pal4::TimedEffectState, active_camera) == 28u &&
              offsetof(pal4::TimedEffectState, cameras_begin) == 36u,
          "timed effect fields retain IDA offsets");
}

void StartHonorsLoadedFlag() {
    pal4::TimedEffectState state{};
    state.enabled = 7u;
    state.completed_cycles = 4u;
    state.active_camera = 0x1010u;
    Check(pal4::TimedEffectControl::Start(state) == 0u &&
              state.enabled == 7u && state.completed_cycles == 4u &&
              state.active_camera == 0x1010u,
          "unloaded start leaves state unchanged");
    state.loaded = 1u;
    Check(pal4::TimedEffectControl::Start(state) == 0u &&
              state.enabled == 1u && state.completed_cycles == 0u &&
              state.active_camera == 0u,
          "loaded start enables and clears cycle and camera fields");
}

void StopRestoresCameraPointer() {
    Memory memory;
    pal4::TimedEffectState state{};
    state.cameras_begin = 0x2000u;
    state.active_camera = 0x3107u;
    state.enabled = 1u;
    memory.words[0x2000u] = 0xaaaa0000u;
    memory.words[0x3117u] = 0xbbbb0000u;
    Check(pal4::TimedEffectControl::Stop(state, memory) == 0u &&
              state.enabled == 1u && state.active_camera == 0x3107u &&
              memory.writes.empty(),
          "unloaded stop does not touch camera memory");
    state.loaded = 1u;
    Check(pal4::TimedEffectControl::Stop(state, memory) == 7u &&
              memory.words.at(0x2000u) == 0xbbbb0000u &&
              memory.words.at(0x3117u) == 0xaaaa0000u &&
              memory.writes == std::vector<std::uint32_t>({0x2000u, 0x3117u}) &&
              state.enabled == 0u && state.active_camera == 0u,
          "loaded stop swaps first entry with active camera slot");
    memory.writes.clear();
    Check(pal4::TimedEffectControl::Stop(state, memory) == 1u &&
              memory.writes.empty(),
          "stop with no active camera returns loaded byte");
}

void AdvanceRotatesOnThreshold() {
    Memory memory;
    pal4::TimedEffectState state{};
    state.loaded = 1u;
    state.enabled = 1u;
    state.interval = 0.5f;
    state.delta = 0.3f;
    state.camera_count = 3u;
    state.cameras_begin = 0x2000u;
    memory.words[0x2000u] = 10u;
    memory.words[0x2004u] = 20u;
    memory.words[0x2008u] = 30u;
    memory.words[0x3110u] = 40u;
    Check(pal4::TimedEffectControl::Advance(state, 0x3100u, memory) == 0u &&
              state.elapsed == 0.3f && memory.writes.empty(),
          "below threshold only accumulates time");
    Check(pal4::TimedEffectControl::Advance(state, 0x3100u, memory) == 3u &&
              state.elapsed == 0.0f && state.active_camera == 0x3100u &&
              state.completed_cycles == 1u &&
              memory.words.at(0x2000u) == 40u &&
              memory.words.at(0x2004u) == 10u &&
              memory.words.at(0x2008u) == 20u &&
              memory.words.at(0x3110u) == 30u,
          "threshold rotates camera entries and exchanges last camera slot");
    state.completed_cycles = 3u;
    state.delta = 0.5f;
    Check(pal4::TimedEffectControl::Advance(state, 0x3100u, memory) == 3u &&
              state.completed_cycles == 3u,
          "completed cycle count saturates at camera count");
}

void AdvanceGatesBeforeMemoryAndRejectsEmptyArray() {
    Memory memory;
    pal4::TimedEffectState state{};
    state.enabled = 1u;
    Check(pal4::TimedEffectControl::Advance(state, 0x3100u, memory) == 0u &&
              memory.writes.empty(), "unloaded advance is inert");
    state.loaded = 1u;
    state.enabled = 0u;
    Check(pal4::TimedEffectControl::Advance(state, 0x3100u, memory) == 0u &&
              memory.writes.empty(), "disabled advance is inert");
    state.enabled = 1u;
    state.interval = 0u;
    bool threw = false;
    try {
        static_cast<void>(pal4::TimedEffectControl::Advance(
            state, 0x3100u, memory));
    } catch (const std::invalid_argument&) {
        threw = true;
    }
    Check(threw, "zero camera count fails before unchecked array access");
    state.interval = std::numeric_limits<float>::quiet_NaN();
    state.camera_count = 1u;
    state.elapsed = 0u;
    Check(pal4::TimedEffectControl::Advance(state, 0x3100u, memory) == 0u &&
              memory.writes.empty(),
          "unordered x87 interval comparison does not rotate");
}

void ManagerInitializeGrowsArrayAndSetsFlags() {
    ManagerRuntime runtime;
    pal4::TimedEffectState state{};
    Check(pal4::TimedEffectManager::Initialize(state, runtime),
          "ten camera records initialize");
    Check(runtime.records.size() == 10u &&
              runtime.allocations == std::vector<std::uint32_t>({
                  4u, 8u, 16u, 32u, 64u}) &&
              runtime.freed == std::vector<std::uint32_t>({
                  0u, 0x5000u, 0x6000u, 0x7000u, 0x8000u}),
          "camera pointer array grows 1, 2, 4, 8, 16 entries");
    Check(state.cameras_begin == 0x9000u &&
              state.cameras_end == 0x9028u &&
              state.cameras_capacity_end == 0x9040u &&
              state.camera_count == 10u && state.loaded == 1u &&
              state.enabled == 0u && state.completed_cycles == 0u &&
              std::bit_cast<std::uint32_t>(state.interval) == 0x3d088889u,
          "successful initialization writes observed state and interval bits");
    for (std::uint32_t i = 0; i < 10u; ++i) {
        Check(runtime.words.at(state.cameras_begin + 4u * i) ==
                  runtime.records[i],
              "array retains camera records in allocation order");
    }
}

void ManagerFailureDestroysNewAndPreviousRecords() {
    ManagerRuntime runtime;
    runtime.fail_init_at = 5u;
    pal4::TimedEffectState state{};
    Check(!pal4::TimedEffectManager::Initialize(state, runtime) &&
              runtime.records.size() == 5u &&
              runtime.destroyed == std::vector<std::uint32_t>({
                  0x3400u, 0x3000u, 0x3100u, 0x3200u, 0x3300u}) &&
              runtime.cleaned == runtime.destroyed,
          "failed fifth camera is cleaned before four stored records");
    Check(state.loaded == 0u && state.camera_count == 10u &&
              state.cameras_begin == state.cameras_end &&
              state.cameras_capacity_end > state.cameras_begin &&
              runtime.words.at(state.cameras_begin) == 0u,
          "failure preserves pointer-array allocation but empties its range");
}

void ManagerCleanupAndRetryReuseArray() {
    ManagerRuntime runtime;
    pal4::TimedEffectState state{};
    Check(pal4::TimedEffectManager::Initialize(state, runtime),
          "setup initializes timed manager");
    state.enabled = 1u;
    state.delta = 0.25f;
    state.elapsed = 0.5f;
    const auto array_begin = state.cameras_begin;
    pal4::TimedEffectManager::Cleanup(state, runtime);
    Check(state.loaded == 0u && state.enabled == 0u &&
              state.camera_count == 0u && state.delta == 0.0f &&
              state.elapsed == 0.0f &&
              state.cameras_begin == array_begin &&
              state.cameras_end == array_begin &&
              runtime.destroyed == runtime.records,
          "cleanup releases records and retains array allocation");
    const auto allocation_count = runtime.allocations.size();
    Check(pal4::TimedEffectManager::Initialize(state, runtime) &&
              runtime.allocations.size() == allocation_count &&
              state.cameras_begin == array_begin,
          "reinitialization reuses the retained capacity");
    const auto destruction_count = runtime.destroyed.size();
    pal4::TimedEffectManager::Cleanup(state, runtime);
    pal4::TimedEffectManager::Cleanup(state, runtime);
    Check(runtime.destroyed.size() == destruction_count + 10u,
          "unloaded cleanup does not destroy records again");
}

void ManagerArrayAllocationFailureIsExplicit() {
    ManagerRuntime runtime;
    runtime.fail_alloc_at = 3u;
    pal4::TimedEffectState state{};
    bool threw = false;
    try {
        static_cast<void>(pal4::TimedEffectManager::Initialize(state, runtime));
    } catch (const std::bad_alloc&) {
        threw = true;
    }
    Check(threw && runtime.records.size() == 3u &&
              runtime.destroyed == std::vector<std::uint32_t>({
                  0x3200u, 0x3000u, 0x3100u}) &&
              state.cameras_end == state.cameras_begin,
          "failed pointer-array allocation cleans new and prior records");
}

void ManagerSetDeltaRequiresLoadedAndEnabled() {
    pal4::TimedEffectState state{};
    state.delta = 3.0f;
    Check(pal4::TimedEffectManager::SetDelta(state, 0.25) == 0u &&
              state.delta == 3.0f, "unloaded delta setter is inert");
    state.loaded = 1u;
    Check(pal4::TimedEffectManager::SetDelta(state, 0.25) == 0u &&
              state.delta == 3.0f, "disabled delta setter is inert");
    state.enabled = 1u;
    Check(pal4::TimedEffectManager::SetDelta(state, 0.25) == 1u &&
              state.delta == 0.25f, "enabled setter narrows double to float");
}

void RendererGatesAndCachesConfigDimensions() {
    RenderRuntime runtime;
    pal4::TimedEffectRenderState render_state{};
    pal4::TimedEffectState state{};
    Check(!pal4::TimedEffectRenderer::Render(
              state, render_state, 0x11u, 0x22u, runtime) &&
              runtime.states.empty() && runtime.config_calls == 0u,
          "unloaded render does not access globals or backend");
    state.loaded = 1u;
    Check(!pal4::TimedEffectRenderer::Render(
              state, render_state, 0x11u, 0x22u, runtime) &&
              runtime.states.empty(),
          "disabled render does not alter backend");
    state.enabled = 1u;
    Check(pal4::TimedEffectRenderer::Render(
              state, render_state, 0x11u, 0x22u, runtime) &&
              runtime.config_calls == 2u &&
              render_state.dimensions_initialized == 1u &&
              render_state.width == 640 && render_state.height == 480 &&
              runtime.states ==
                  std::vector<std::pair<std::uint32_t, std::uint32_t>>({
                      {12u, 1u}, {10u, 5u}, {11u, 6u}}) &&
              runtime.draws.empty(),
          "enabled zero-layer render caches dimensions and sets blend states");
    runtime.width = 800;
    runtime.height = 600;
    Check(pal4::TimedEffectRenderer::Render(
              state, render_state, 0u, 0u, runtime) &&
              runtime.config_calls == 2u && render_state.width == 640 &&
              render_state.height == 480,
          "later calls retain first config dimensions");
}

void RendererDrawsCompletedLayersWithFadingAlpha() {
    RenderRuntime runtime;
    pal4::TimedEffectRenderState render_state{};
    render_state.x = 5;
    render_state.y = 7;
    pal4::TimedEffectState state{};
    state.loaded = 1u;
    state.enabled = 1u;
    state.completed_cycles = 2u;
    state.camera_count = 4u;
    state.cameras_begin = 0x5000u;
    runtime.words[0x5000u] = 0x3000u;
    runtime.words[0x5004u] = 0x3100u;
    runtime.words[0x3004u] = 0x8000u;
    runtime.words[0x3104u] = 0x8100u;
    Check(pal4::TimedEffectRenderer::Render(
              state, render_state, 3u, 4u, runtime) &&
              runtime.manager_calls == 2u &&
              runtime.textures == std::vector<std::uint32_t>({
                  0x8000u, 0x8100u}) &&
              runtime.draws.size() == 2u,
          "render visits stored camera textures in order");
    Check(runtime.draws[0][0].colour == 0xffffffffu &&
              runtime.draws[1][0].colour == 0xbfffffffu &&
              runtime.draws[0][0].x == 5.0f &&
              runtime.draws[0][0].y == 487.0f &&
              runtime.draws[0][3].x == 645.0f &&
              runtime.draws[0][3].y == 7.0f,
          "layer alpha and rectangle geometry match observed formula");
    Check(runtime.states.size() == 15u &&
              runtime.states[0] == std::pair(12u, 1u) &&
              runtime.states[1] == std::pair(10u, 5u) &&
              runtime.states[2] == std::pair(11u, 6u),
          "three blend states precede both textured rectangles");
}

}  // namespace

int main() {
    LayoutMatchesObservedOffsets();
    StartHonorsLoadedFlag();
    StopRestoresCameraPointer();
    AdvanceRotatesOnThreshold();
    AdvanceGatesBeforeMemoryAndRejectsEmptyArray();
    ManagerInitializeGrowsArrayAndSetsFlags();
    ManagerFailureDestroysNewAndPreviousRecords();
    ManagerCleanupAndRetryReuseArray();
    ManagerArrayAllocationFailureIsExplicit();
    ManagerSetDeltaRequiresLoadedAndEnabled();
    RendererGatesAndCachesConfigDimensions();
    RendererDrawsCompletedLayersWithFadingAlpha();
    std::cout << "12 timed effect test cases passed\n";
}
