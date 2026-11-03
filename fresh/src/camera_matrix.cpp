#include "pal4/camera_matrix.h"
#include "pal4/camera_frustum_geometry.h"
#include "pal4/camera_bounds.h"

#include <bit>

namespace pal4 {

std::uint32_t* CameraMatrix::Initialize(CameraDataState& state) {
    if (state.At(0) == 0u) return nullptr;
    constexpr std::array<std::size_t, 9> zero_offsets{
        0x138u, 0x130u, 0x12cu, 0x14cu, 0x148u,
        0x140u, 0x160u, 0x15cu, 0x158u};
    for (const auto offset : zero_offsets) state.At(offset) = 0u;
    constexpr auto one = std::bit_cast<std::uint32_t>(1.0f);
    state.At(0x150u) = one;
    state.At(0x13cu) = one;
    state.At(0x128u) = one;
    state.At(0x134u) |= 0x20003u;
    runtime_.Rotate(state, 0x128u, {0.0f, 1.0f, 0.0f},
                    std::bit_cast<float>(state.At(0x3cu)), 0u);
    return &state.At(0x128u);
}

std::optional<std::uint32_t> CameraMatrix::Update(
    CameraDataState& state, std::uint32_t solve_eye) {
    const auto camera = state.At(0);
    if (camera == 0u) return std::nullopt;

    constexpr std::array<std::size_t, 9> zero_offsets{
        0xb8u, 0xb0u, 0xacu, 0xccu, 0xc8u,
        0xc0u, 0xe0u, 0xdcu, 0xd8u};
    for (const auto offset : zero_offsets) state.At(offset) = 0u;
    constexpr auto one = std::bit_cast<std::uint32_t>(1.0f);
    state.At(0xd0u) = one;
    state.At(0xbcu) = one;
    state.At(0xa8u) = one;
    state.At(0xb4u) |= 0x20003u;
    runtime_.Rotate(state, 0xa8u, {1.0f, 0.0f, 0.0f},
                    std::bit_cast<float>(state.At(0x40u)), 0u);
    runtime_.Rotate(state, 0xa8u, {0.0f, 1.0f, 0.0f},
                    std::bit_cast<float>(state.At(0x3cu)), 2u);
    runtime_.Rotate(state, 0xa8u, {0.0f, 0.0f, 1.0f},
                    std::bit_cast<float>(state.At(0x44u)), 2u);

    for (std::size_t i = 0; i < 3u; ++i) {
        state.At(0x50u + i * 4u) = state.At(0xc8u + i * 4u);
        state.At(0x80u + i * 4u) = state.At(0xb8u + i * 4u);
        state.At(0x5cu + i * 4u) = state.At(0xa8u + i * 4u);
    }

    const auto distance = std::bit_cast<float>(state.At(0x48u));
    for (std::size_t i = 0; i < 3u; ++i) {
        const auto advance = static_cast<float>(
            static_cast<double>(distance) *
            std::bit_cast<float>(state.At(0x50u + i * 4u)));
        const auto source_offset = (solve_eye != 0u ? 0x74u : 0x68u) + i * 4u;
        const auto target_offset = (solve_eye != 0u ? 0x68u : 0x74u) + i * 4u;
        const auto source = std::bit_cast<float>(state.At(source_offset));
        const auto target = solve_eye != 0u ? source - advance
                                            : source + advance;
        state.At(target_offset) = std::bit_cast<std::uint32_t>(target);
    }
    for (std::size_t i = 0; i < 3u; ++i)
        state.At(0xd8u + i * 4u) = state.At(0x68u + i * 4u);

    const auto frame = runtime_.CameraFrame(camera);
    runtime_.SetFrameIdentity(frame);
    runtime_.ApplyTransform(frame, state, 0xa8u, 0u);
    runtime_.UpdateFrameObjects(frame);
    static_cast<void>(CameraFrustumGeometry::UpdateVertices(state,
                                                            distance));
    return bounds_.Prepare(state);
}

}  // namespace pal4
