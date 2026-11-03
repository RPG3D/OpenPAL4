#include "pal4/camera_task_object.h"

#include <bit>
#include <cmath>
#include <cstdint>
#include <string_view>

namespace pal4 {
namespace {

float ReadFloat(const std::uint32_t address,
                ICameraTaskPlaybackRuntime& runtime) {
    return std::bit_cast<float>(runtime.ReadWord(address));
}

void WriteFloat(const std::uint32_t address, const float value,
                ICameraTaskPlaybackRuntime& runtime) {
    runtime.WriteWord(address, std::bit_cast<std::uint32_t>(value));
}

CameraTaskVector ReadVector(const std::uint32_t address,
                            ICameraTaskPlaybackRuntime& runtime) {
    return {ReadFloat(address, runtime), ReadFloat(address + 4u, runtime),
            ReadFloat(address + 8u, runtime)};
}

void CopyVector(const std::uint32_t destination,
                const std::uint32_t source,
                ICameraTaskPlaybackRuntime& runtime) {
    for (std::uint32_t offset = 0u; offset < 12u; offset += 4u)
        runtime.WriteWord(destination + offset,
                          runtime.ReadWord(source + offset));
}

CameraTaskVector Subtract(const CameraTaskVector& left,
                          const CameraTaskVector& right) {
    return {left.x - right.x, left.y - right.y, left.z - right.z};
}

CameraTaskVector Cross(const CameraTaskVector& left,
                       const CameraTaskVector& right) {
    return {left.y * right.z - left.z * right.y,
            left.z * right.x - left.x * right.z,
            left.x * right.y - left.y * right.x};
}

CameraTaskBasis MakeBasis(CameraTaskVector direction,
                          ICameraTaskGeometry& geometry) {
    CameraTaskVector right = Cross(direction, {0.0f, 1.0f, 0.0f});
    geometry.Normalize(right);
    // IDA592B12/592C4D: direction x right, before SDK quaternion conversion.
    CameraTaskVector up = Cross(direction, right);
    geometry.Normalize(up);
    return {right, up, direction};
}

float ReadConfig(const std::uint32_t child, const std::string_view key,
                 ICameraTaskPlaybackRuntime& runtime) {
    return runtime.ConfigFloat(child, key).value_or(0.0f);
}

CameraTaskVector ReadConfigVector(const std::uint32_t child,
                                  const bool eye,
                                  ICameraTaskPlaybackRuntime& runtime) {
    if (eye)
        return {ReadConfig(child, "SCLD_Extend_eye_x", runtime),
                ReadConfig(child, "SCLD_Extend_eye_y", runtime),
                ReadConfig(child, "SCLD_Extend_eye_z", runtime)};
    return {ReadConfig(child, "SCLD_Extend_lookat_x", runtime),
            ReadConfig(child, "SCLD_Extend_lookat_y", runtime),
            ReadConfig(child, "SCLD_Extend_lookat_z", runtime)};
}

float ShortAngleDelta(const float start, const float finish) {
    // IDA592D4B..592D63 /592DB8..592DCE: preserve the original PC24
    // operation association at sign crossings. (finish-start)+/-360 has
    // different rounding at the actual Q07 N03c camera001 endpoint.
    if (start < 0.0f && finish >= 0.0f &&
        std::fabs(start) + std::fabs(finish) > 180.0f) {
        const volatile float first = 180.0f - finish;
        const volatile float second = start + 180.0f;
        const volatile float third = first + second;
        return -third;
    }
    if (start >= 0.0f && finish < 0.0f &&
        std::fabs(start) + std::fabs(finish) > 180.0f) {
        const volatile float first = 180.0f - start;
        const volatile float second = finish + 180.0f;
        const volatile float third = first + second;
        return third;
    }
    return finish - start;
}

}  // namespace

void CameraTaskGeometryStub::ClampDirection(CameraTaskVector&) {}
void CameraTaskGeometryStub::Normalize(CameraTaskVector&) {}
CameraTaskQuaternion CameraTaskGeometryStub::ToQuaternion(
    const CameraTaskBasis&) { return {}; }
CameraTaskVector CameraTaskGeometryStub::ToEuler(
    const CameraTaskQuaternion& quaternion) {
    CameraTaskVector result;
    static_cast<void>(CameraTaskOrientation::ToEuler(quaternion, result));
    return result;
}

std::uint8_t CameraTaskObject::Enter(
    const std::uint32_t object, ICameraTaskPlaybackRuntime& runtime,
    ICameraTaskGeometry& geometry) {
    const auto camera = runtime.ReadWord(object + 0x48u);
    if (camera == 0u) return 0u;

    const auto record = runtime.ReadWord(object + 0x4cu);
    const auto child = runtime.FindXmlChild(runtime.ReadWord(record + 0x40u),
                                             "SCLD_Extend");
    CopyVector(object + 0x50u, camera + 0x74u, runtime);
    runtime.WriteWord(object + 0x5cu, runtime.ReadWord(camera + 0x40u));
    runtime.WriteWord(object + 0x60u, runtime.ReadWord(camera + 0x3cu));
    runtime.WriteWord(object + 0x64u, runtime.ReadWord(camera + 0x44u));
    CopyVector(object + 0x68u, camera + 0x68u, runtime);

    const auto eye = ReadConfigVector(child, true, runtime);
    WriteFloat(object + 0x74u, eye.x, runtime);
    WriteFloat(object + 0x78u, eye.y, runtime);
    WriteFloat(object + 0x7cu, eye.z, runtime);
    runtime.WriteWord(object + 0x80u, runtime.ReadWord(camera + 0x48u));
    const auto flag = runtime.ReadWord(record + 0x30u);
    runtime.WriteWord(object + 0x84u, flag);

    const auto mode = runtime.ReadWord(record + 0x38u);
    if (mode != 0u) {
        if (mode == 1u) {
            runtime.SetLookAt(camera, ReadVector(record + 0x0cu, runtime),
                              flag);
            runtime.SetSecondAngle(camera, ReadFloat(record + 0x18u, runtime),
                                   flag);
            runtime.SetAngle(camera, ReadFloat(record + 0x1cu, runtime), flag);
            runtime.SetThirdAngle(camera, ReadFloat(record + 0x20u, runtime),
                                  flag);
            runtime.SetScalar(camera, ReadFloat(record + 0x2cu, runtime), flag);
            runtime.WriteWord(object + 0x40u,
                              runtime.ReadWord(object + 0x44u));
            runtime.WriteByte(object + 0x3cu, 1u);
        }
        return 1u;
    }

    const auto look_at = ReadConfigVector(child, false, runtime);
    const auto end_eye = ReadConfigVector(child, true, runtime);
    auto initial_direction = Subtract(ReadVector(camera + 0x74u, runtime),
                                      ReadVector(camera + 0x68u, runtime));
    auto final_direction = Subtract(look_at, end_eye);
    geometry.ClampDirection(initial_direction);
    geometry.ClampDirection(final_direction);
    const auto initial_basis = MakeBasis(initial_direction, geometry);
    const auto final_basis = MakeBasis(final_direction, geometry);
    const auto initial_quaternion = geometry.ToQuaternion(initial_basis);
    const auto final_quaternion = geometry.ToQuaternion(final_basis);
    const auto initial_euler = geometry.ToEuler(initial_quaternion);
    const auto final_euler = geometry.ToEuler(final_quaternion);

    auto yaw_delta = ShortAngleDelta(initial_euler.y, final_euler.y);
    if (static_cast<std::int32_t>(runtime.ReadWord(record + 0x24u)) > 0)
        yaw_delta += yaw_delta < 0.0f ? 360.0f : -360.0f;
    WriteFloat(object + 0x88u, yaw_delta, runtime);
    WriteFloat(object + 0x8cu,
               ShortAngleDelta(initial_euler.x, final_euler.x), runtime);
    return 1u;
}

}  // namespace pal4
