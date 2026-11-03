#include "pal4/pal_actor_control_transform.h"

#include <bit>
#include <initializer_list>

namespace pal4 {

ActorControlMatrix ActorControlTransformRuntimeStub::CameraMatrix(
    std::uint32_t) { return {}; }
ActorControlMatrix ActorControlTransformRuntimeStub::ActorTransform(
    std::uint32_t) { return {}; }
void ActorControlTransformRuntimeStub::SetupTransformMatrix(
    ActorControlMatrix&, const ActorControlMatrix&) {}
void ActorControlTransformRuntimeStub::NormalizeBasis(
    ActorControlMatrix&, std::uint32_t) {}
ActorControlMatrix ActorControlTransformRuntimeStub::Multiply(
    const ActorControlMatrix&, const ActorControlMatrix&) { return {}; }
ActorControlRotation ActorControlTransformRuntimeStub::QueryRotation(
    const ActorControlMatrix&) { return {}; }
ActorControlVector3 ActorControlTransformRuntimeStub::CameraVector(
    std::uint32_t, std::uint32_t) { return {}; }

float PalActorControlTransform::Compute(
    PalActorControlState& state, std::uint32_t camera,
    std::uint32_t actor, float* axis_y,
    IActorControlTransformRuntime& runtime) {
    if (camera == 0u || actor == 0u)
        return 0.0f;

    const auto camera_matrix = runtime.CameraMatrix(camera);
    const auto actor_matrix = runtime.ActorTransform(actor);
    ActorControlMatrix prepared{};
    runtime.SetupTransformMatrix(prepared, actor_matrix);
    for (const auto offset : {0u, 16u, 32u})
        runtime.NormalizeBasis(prepared, offset);
    const auto combined = runtime.Multiply(camera_matrix, prepared);
    const auto rotation = runtime.QueryRotation(combined);
    if (axis_y != nullptr)
        *axis_y = rotation.axis.y;

    auto& words = state.words;
    if (words[6] != 0u) {
        const auto camera_direction = runtime.CameraVector(camera, 80u);
        const auto camera_position = runtime.CameraVector(camera, 116u);
        const auto actor_forward_x =
            std::bit_cast<float>(actor_matrix.words[8]);
        const auto actor_forward_z =
            std::bit_cast<float>(actor_matrix.words[10]);
        const auto put = [&words](std::uint32_t index, float value) {
            words[index] = std::bit_cast<std::uint32_t>(value);
        };
        const auto put_position = [&put](std::uint32_t index,
                                         ActorControlVector3 value) {
            put(index, value.x);
            put(index + 1u, value.y);
            put(index + 2u, value.z);
        };

        put_position(225u, camera_position);
        put(234u, camera_position.x - actor_forward_x * 90.0f);
        put(235u, camera_position.y);
        put(236u, camera_position.z - actor_forward_z * 90.0f);
        words[231] = 0xff0000ffu;
        words[240] = 0xff0000ffu;

        put_position(243u, camera_position);
        put(252u, camera_position.x - camera_direction.x * 140.0f);
        put(253u, camera_position.y);
        put(254u, camera_position.z - camera_direction.z * 140.0f);
        words[249] = 0xffffff00u;
        words[258] = 0xffffff00u;

        put_position(261u, camera_position);
        put(270u, camera_position.x);
        put(271u, camera_position.y + 40.0f);
        put(272u, camera_position.z);
        words[267] = 0xff0000ffu;
        words[276] = 0xff0000ffu;
    }
    return rotation.angle;
}

}  // namespace pal4
