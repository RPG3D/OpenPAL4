#include "pal4/camera_data.h"
#include "pal4/camera_frustum_geometry.h"
#include "pal4/camera_environment_probe.h"
#include "pal4/camera_obstacle_check.h"
#include "pal4/comparison_camera_collision_trace.h"

#include <bit>
#include <cmath>
#include <initializer_list>
#include <limits>

namespace pal4 {

void CameraGeometry::AdjustViewWindow(CameraDataState& state,
                                      CameraViewportGlobals& globals,
                                      CameraRect* rectangle,
                                      float scale,
                                      std::int32_t unused) {
    static_cast<void>(unused);
    const auto camera = state.At(0);
    if (camera == 0u) return;

    if (globals.original_width == 0 || globals.original_height == 0) {
        globals.original_x = 0;
        globals.original_y = 0;
        const auto [width, height] = runtime_.RasterSize(camera, 0x60u);
        globals.original_width = width;
        globals.original_height = height;
    }

    const auto video_mode = runtime_.VideoModeInfo(runtime_.CurrentVideoMode());
    if (rectangle == nullptr) {
        const auto [width, height] = runtime_.RasterSize(camera, 0x60u);
        globals.default_rect = {0, 0, width, height};
        rectangle = &globals.default_rect;
    }
    if ((video_mode.flags & 1u) != 0u)
        *rectangle = {0, 0, video_mode.width, video_mode.height};

    const auto width = (*rectangle)[2];
    const auto height = (*rectangle)[3];
    if (width <= 0 || height <= 0) return;

    runtime_.SetRasterSize(camera, 0x60u, width, height);
    runtime_.SetRasterSize(camera, 0x64u, width, height);
    float x = scale;
    float y = scale;
    if (width > height)
        y = static_cast<float>(static_cast<double>(height) * scale / width);
    else
        x = static_cast<float>(static_cast<double>(width) * scale / height);
    if (runtime_.ExpandedAspect()) {
        x *= 1.3333334f;
        y *= 1.3333334f;
    }
    state.At(0x4) = std::bit_cast<std::uint32_t>(x);
    state.At(0x8) = std::bit_cast<std::uint32_t>(y);
    runtime_.SetViewWindow(camera, x, y);
}

std::optional<std::uint32_t> CameraGeometry::SetParameters(
    CameraDataState& state,
    const std::array<float, 3>& target,
    float angle_y, float angle_x, float angle_z,
    float distance) {
    if (state.At(0) == 0u) return std::nullopt;

    state.At(0x74) = std::bit_cast<std::uint32_t>(target[0]);
    state.At(0x78) = std::bit_cast<std::uint32_t>(target[1]);
    state.At(0x7c) = std::bit_cast<std::uint32_t>(target[2]);
    state.At(0x3c) = std::bit_cast<std::uint32_t>(angle_y);
    state.At(0x40) = std::bit_cast<std::uint32_t>(angle_x);
    state.At(0x44) = std::bit_cast<std::uint32_t>(angle_z);
    if (!std::isnan(distance)) {
        const auto minimum = runtime_.MinimumDistance();
        if (distance < minimum) distance = minimum;
        const auto maximum = runtime_.MaximumDistance();
        if (distance >= maximum) distance = maximum;
    } else {
        static_cast<void>(runtime_.MinimumDistance());
        static_cast<void>(runtime_.MaximumDistance());
    }
    state.At(0x4c) = std::bit_cast<std::uint32_t>(distance);
    state.At(0x48) = state.At(0x4c);
    return runtime_.UpdateMatrix(state, 1u);
}

std::optional<std::uint32_t> CameraAnimation::Update(
    CameraDataState& state, float delta) {
    ComparisonCameraAdvanceScope observation(GetComparisonCameraCollisionTrace(),&state,delta);
    if (state.At(0) == 0u) return std::nullopt;
    static_cast<void>(AnimatePosition(state, delta));
    if (state.At(0x1a8) == 0u) return 0u;

    const auto target = std::bit_cast<float>(state.At(0x4c));
    const auto current = std::bit_cast<float>(state.At(0x48));
    float candidate = std::numeric_limits<float>::max();
    CameraPoint auxiliary{};
    if (current < target) {
        const auto speed = std::bit_cast<float>(state.At(0x1b4));
        // IDA5E94A0/5E94A4/5E94AA: PC24 rounds the product first.
        const auto increment = delta * speed;
        auto advanced = increment + current;
        if (advanced >= target) advanced = target;
        static_cast<void>(CameraFrustumGeometry::UpdateVertices(state,
                                                                advanced));
        if (environment_.Update(state, &candidate, &auxiliary)) {
            auto distance = candidate;
            if (distance >= target) distance = target;
            const auto minimum = runtime_.MinimumDistance();
            if (distance <= minimum) distance = minimum;
            state.At(0x48) = std::bit_cast<std::uint32_t>(distance);
        } else {
            state.At(0x48) = std::bit_cast<std::uint32_t>(advanced);
            auto retrieved = std::numeric_limits<float>::max();
            if (obstacle_.RetrieveDistance(state, retrieved))
                state.At(0x48) = std::bit_cast<std::uint32_t>(retrieved);
            const auto minimum = runtime_.MinimumDistance();
            if (std::bit_cast<float>(state.At(0x48)) <= minimum)
                state.At(0x48) = std::bit_cast<std::uint32_t>(minimum);
        }
        return runtime_.UpdateMatrix(state, state.At(0x38));
    }

    if (!environment_.Update(state, &candidate, &auxiliary)) return 0u;
    state.At(0x48) = std::bit_cast<std::uint32_t>(candidate);
    if (candidate < target || std::isnan(candidate)) return 0x100u;
    state.At(0x48) = state.At(0x4c);
    return 0u;
}

std::optional<std::uint32_t> CameraAnimation::AnimatePosition(
    CameraDataState& state, float delta) {
    if (state.At(0x8cu) == 0u) return 0u;
    const auto duration = std::bit_cast<float>(state.At(0x94u));
    if (!(duration > 0.0f)) return std::nullopt;

    const auto previous = std::bit_cast<float>(state.At(0x90u));
    auto elapsed = previous + delta;
    if (elapsed >= duration) {
        state.At(0x8cu) = 0u;
        elapsed = duration;
    }
    state.At(0x90u) = std::bit_cast<std::uint32_t>(elapsed);

    const auto random = runtime_.Random() % 100;
    const auto frequency = std::bit_cast<float>(state.At(0x9cu));
    const auto previous_phase = std::bit_cast<float>(state.At(0xa0u));
    // 5EA3D1/5EA3D7 run at PC24. The retained x87 value passed to FSIN
    // is already rounded by the addition, even though FST does not pop it.
    const auto phase_increment = delta * frequency;
    const auto phase = phase_increment + previous_phase;
    state.At(0xa0u) = std::bit_cast<std::uint32_t>(phase);
    const auto amplitude = std::bit_cast<float>(state.At(0x98u));
    const auto baseline = std::bit_cast<float>(state.At(0xa4u));
    // Keep the public double sine boundary; the surrounding game arithmetic
    // rounds at every original x87 instruction (5EA3C7, 5EA3E5..5EA407).
    const auto noise = static_cast<float>(random) * 0.0099999998f;
    const auto wave = static_cast<float>(
        std::sin(static_cast<double>(phase)) + noise);
    const auto fraction = elapsed / duration;
    const auto remaining = 1.0f - fraction;
    const auto fading = remaining * 0.5f;
    const auto faded_wave = wave * fading;
    const auto displacement = faded_wave * amplitude;
    const auto y = displacement + baseline;
    state.At(0x78u) = std::bit_cast<std::uint32_t>(y);
    return runtime_.UpdateMatrix(state, 1u);
}

}  // namespace pal4
