#include "pal4/timed_effect_state.h"

#include <bit>
#include <new>
#include <stdexcept>

namespace pal4 {

std::uint32_t TimedEffectMemoryStub::Read32(std::uint32_t /*address*/) {
    return 0u;
}

void TimedEffectMemoryStub::Write32(std::uint32_t /*address*/,
                                    std::uint32_t /*value*/) {}

std::uint32_t TimedEffectControl::Start(TimedEffectState& state) {
    if (state.loaded != 0u) {
        state.enabled = 1u;
        state.completed_cycles = 0u;
        state.active_camera = 0u;
    }
    return 0u;
}

std::uint8_t TimedEffectControl::Stop(
    TimedEffectState& state, ITimedEffectMemory& memory) {
    if (state.loaded == 0u) return 0u;
    std::uint32_t result = state.loaded;
    if (state.active_camera != 0u) {
        const auto current = memory.Read32(state.cameras_begin);
        memory.Write32(state.cameras_begin,
                       memory.Read32(state.active_camera + 16u));
        memory.Write32(state.active_camera + 16u, current);
        result = state.active_camera;
    }
    state.enabled = 0u;
    state.active_camera = 0u;
    return static_cast<std::uint8_t>(result);
}

std::uint8_t TimedEffectControl::Advance(
    TimedEffectState& state, const std::uint32_t camera,
    ITimedEffectMemory& memory) {
    if (state.loaded == 0u) return 0u;
    if (state.enabled == 0u) return 0u;

    const double elapsed = static_cast<double>(state.delta) + state.elapsed;
    state.elapsed = static_cast<float>(elapsed);
    if (!(elapsed >= state.interval)) return 0u;

    state.elapsed = 0.0f;
    const auto count = state.camera_count;
    if (count == 0u) {
        throw std::invalid_argument("timed effect has no camera records");
    }
    const auto last = memory.Read32(state.cameras_begin + 4u * (count - 1u));
    for (auto index = count - 1u; index >= 1u; --index) {
        memory.Write32(state.cameras_begin + 4u * index,
                       memory.Read32(state.cameras_begin + 4u * (index - 1u)));
    }
    memory.Write32(state.cameras_begin, memory.Read32(camera + 16u));
    memory.Write32(camera + 16u, last);
    state.active_camera = camera;
    if (state.completed_cycles < count) ++state.completed_cycles;
    return static_cast<std::uint8_t>(count);
}

std::uint32_t TimedEffectManagerRuntimeStub::Read32(
    std::uint32_t /*address*/) { return 0u; }
void TimedEffectManagerRuntimeStub::Write32(
    std::uint32_t /*address*/, std::uint32_t /*value*/) {}
std::uint32_t TimedEffectManagerRuntimeStub::CreateCameraRecord() { return 0u; }
bool TimedEffectManagerRuntimeStub::InitializeCamera(
    std::uint32_t /*record*/) { return false; }
void TimedEffectManagerRuntimeStub::CleanupCamera(
    std::uint32_t /*record*/) {}
void TimedEffectManagerRuntimeStub::DestroyCameraRecord(
    std::uint32_t /*record*/) {}
std::uint32_t TimedEffectManagerRuntimeStub::AllocateArray(
    std::uint32_t /*bytes*/) { return 0u; }
void TimedEffectManagerRuntimeStub::FreeArray(
    std::uint32_t /*address*/) {}

namespace {

constexpr std::uint32_t kCameraCount = 10u;
constexpr std::uint32_t kDefaultIntervalBits = 0x3d088889u;

void DestroyRecord(const std::uint32_t record,
                   ITimedEffectManagerRuntime& runtime) {
    runtime.CleanupCamera(record);
    runtime.DestroyCameraRecord(record);
}

std::uint32_t ArraySize(const TimedEffectState& state) {
    return state.cameras_begin == 0u
               ? 0u : (state.cameras_end - state.cameras_begin) / 4u;
}

void ClearRecords(TimedEffectState& state,
                  ITimedEffectManagerRuntime& runtime,
                  const bool clear_slots) {
    const auto size = ArraySize(state);
    for (std::uint32_t i = 0; i < size; ++i) {
        const auto slot = state.cameras_begin + 4u * i;
        DestroyRecord(runtime.Read32(slot), runtime);
        if (clear_slots) runtime.Write32(slot, 0u);
    }
    state.cameras_end = state.cameras_begin;
}

void AppendCamera(TimedEffectState& state, const std::uint32_t camera,
                  ITimedEffectManagerRuntime& runtime) {
    if (state.cameras_end != state.cameras_capacity_end) {
        runtime.Write32(state.cameras_end, camera);
        state.cameras_end += 4u;
        return;
    }

    const auto size = ArraySize(state);
    const auto new_capacity = size + (size > 1u ? size : 1u);
    const auto new_begin = runtime.AllocateArray(4u * new_capacity);
    if (new_begin == 0u) throw std::bad_alloc();
    for (std::uint32_t i = 0; i < size; ++i) {
        runtime.Write32(new_begin + 4u * i,
                        runtime.Read32(state.cameras_begin + 4u * i));
    }
    runtime.Write32(new_begin + 4u * size, camera);
    runtime.FreeArray(state.cameras_begin);
    state.cameras_begin = new_begin;
    state.cameras_end = new_begin + 4u * (size + 1u);
    state.cameras_capacity_end = new_begin + 4u * new_capacity;
}

}  // namespace

bool TimedEffectManager::Initialize(
    TimedEffectState& state, ITimedEffectManagerRuntime& runtime) {
    Cleanup(state, runtime);
    state.camera_count = kCameraCount;
    for (std::uint32_t i = 0; i < kCameraCount; ++i) {
        const auto record = runtime.CreateCameraRecord();
        if (!runtime.InitializeCamera(record)) {
            DestroyRecord(record, runtime);
            ClearRecords(state, runtime, true);
            return false;
        }
        try {
            AppendCamera(state, record, runtime);
        } catch (...) {
            DestroyRecord(record, runtime);
            ClearRecords(state, runtime, true);
            throw;
        }
    }
    state.completed_cycles = 0u;
    state.elapsed = 0.0f;
    state.delta = 0.0f;
    state.loaded = 1u;
    state.enabled = 0u;
    state.interval = std::bit_cast<float>(kDefaultIntervalBits);
    return true;
}

void TimedEffectManager::Cleanup(
    TimedEffectState& state, ITimedEffectManagerRuntime& runtime) {
    if (state.loaded == 0u) return;
    static_cast<void>(TimedEffectControl::Stop(state, runtime));
    state.loaded = 0u;
    state.enabled = 0u;
    state.camera_count = 0u;
    state.interval = std::bit_cast<float>(kDefaultIntervalBits);
    state.elapsed = 0.0f;
    state.delta = 0.0f;
    ClearRecords(state, runtime, false);
}

std::uint8_t TimedEffectManager::SetDelta(
    TimedEffectState& state, const double delta) {
    if (state.loaded == 0u) return state.loaded;
    if (state.enabled == 0u) return state.enabled;
    state.delta = static_cast<float>(delta);
    return state.enabled;
}

std::uint32_t TimedEffectRenderRuntimeStub::Read32(
    std::uint32_t /*address*/) { return 0u; }
void TimedEffectRenderRuntimeStub::Write32(
    std::uint32_t /*address*/, std::uint32_t /*value*/) {}
std::int32_t TimedEffectRenderRuntimeStub::ConfigWidth() { return 0; }
std::int32_t TimedEffectRenderRuntimeStub::ConfigHeight() { return 0; }
std::uint32_t TimedEffectRenderRuntimeStub::InitializeCameraSubsystem() {
    return 0u;
}
std::uint32_t TimedEffectRenderRuntimeStub::ActiveCameraObject(
    std::uint32_t /*manager*/) { return 0u; }
std::uint32_t TimedEffectRenderRuntimeStub::ObjectInternalId(
    std::uint32_t /*camera*/) { return 0u; }
float TimedEffectRenderRuntimeStub::CameraDepth(
    std::uint32_t /*context*/) { return 1.0f; }
float TimedEffectRenderRuntimeStub::ActiveDepth() { return 0.0f; }
std::uint32_t TimedEffectRenderRuntimeStub::TextureRaster(
    std::uint32_t /*texture*/) { return 0u; }
void TimedEffectRenderRuntimeStub::SetRenderState(
    std::uint32_t /*state*/, std::uint32_t /*value*/) {}
void TimedEffectRenderRuntimeStub::DrawPrimitive(
    std::uint32_t /*primitive*/,
    const std::array<CeguiRenderVertex, 4>& /*vertices*/,
    std::uint32_t /*count*/) {}

bool TimedEffectRenderer::Render(
    const TimedEffectState& state, TimedEffectRenderState& render_state,
    std::uint32_t /*ignored_first*/, std::uint32_t /*ignored_second*/,
    ITimedEffectRenderRuntime& runtime) {
    if (state.loaded == 0u || state.enabled == 0u) return false;

    if ((render_state.dimensions_initialized & 1u) == 0u) {
        render_state.dimensions_initialized |= 1u;
        render_state.width = runtime.ConfigWidth();
        render_state.height = runtime.ConfigHeight();
    }
    runtime.SetRenderState(12u, 1u);
    runtime.SetRenderState(10u, 5u);
    runtime.SetRenderState(11u, 6u);

    const auto layers = static_cast<std::int32_t>(state.completed_cycles);
    const auto count = static_cast<std::int32_t>(state.camera_count);
    for (std::int32_t i = 0; i < layers; ++i) {
        if (count == 0) throw std::invalid_argument(
            "timed effect render has no camera records");
        const auto alpha = static_cast<std::uint8_t>(
            (255ll * (count - static_cast<std::int64_t>(i))) / count);
        const auto record = runtime.Read32(
            state.cameras_begin + 4u * static_cast<std::uint32_t>(i));
        const auto texture = runtime.Read32(record + 4u);
        const auto manager = runtime.InitializeCameraSubsystem();
        const auto active = runtime.ActiveCameraObject(manager);
        const auto context = runtime.ObjectInternalId(active);
        static_cast<void>(PalRectangleDrawer::DrawTextured(
            context, render_state.x, render_state.y,
            render_state.width, render_state.height, texture,
            {255u, 255u, 255u, alpha}, runtime));
    }
    return true;
}

}  // namespace pal4
