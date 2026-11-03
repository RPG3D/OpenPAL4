#include "pal4/camera_task_object.h"

#include <bit>
#include <cstdint>

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

float Interpolate(const float start, const float end,
                  const float fraction, ICameraTaskPlaybackRuntime& runtime) {
    return runtime.InterpolateFloat(start, end, fraction, 0u);
}

}  // namespace

std::uint8_t CameraTaskObject::Update(
    const std::uint32_t object, const float delta,
    ICameraTaskPlaybackRuntime& runtime) {
    if (runtime.ReadByte(object + 0x3cu) != 0u) return 0u;

    const auto elapsed = ReadFloat(object + 0x40u, runtime) + delta;
    WriteFloat(object + 0x40u, elapsed, runtime);
    const auto duration_bits = runtime.ReadWord(object + 0x44u);
    const auto duration = std::bit_cast<float>(duration_bits);
    if (elapsed >= duration) {
        runtime.WriteByte(object + 0x3cu, 1u);
        runtime.WriteWord(object + 0x40u, duration_bits);
    }

    const auto record = runtime.ReadWord(object + 0x4cu);
    if (runtime.ReadWord(record + 0x38u) != 0u) return 1u;

    const auto fraction = ReadFloat(object + 0x40u, runtime) / duration;
    const auto flag = runtime.ReadWord(object + 0x84u);
    const auto camera = runtime.ReadWord(object + 0x48u);
    if (flag != 0u) {
        const auto z = Interpolate(ReadFloat(object + 0x58u, runtime),
                                   ReadFloat(record + 0x14u, runtime),
                                   fraction, runtime);
        const auto y = Interpolate(ReadFloat(object + 0x54u, runtime),
                                   ReadFloat(record + 0x10u, runtime),
                                   fraction, runtime);
        const auto x = Interpolate(ReadFloat(object + 0x50u, runtime),
                                   ReadFloat(record + 0x0cu, runtime),
                                   fraction, runtime);
        runtime.SetPosition(camera, {x, y, z}, flag);
    } else {
        const auto z = Interpolate(ReadFloat(object + 0x70u, runtime),
                                   ReadFloat(object + 0x7cu, runtime),
                                   fraction, runtime);
        const auto y = Interpolate(ReadFloat(object + 0x6cu, runtime),
                                   ReadFloat(object + 0x78u, runtime),
                                   fraction, runtime);
        const auto x = Interpolate(ReadFloat(object + 0x68u, runtime),
                                   ReadFloat(object + 0x74u, runtime),
                                   fraction, runtime);
        runtime.SetEye(camera, {x, y, z}, 0u);
    }

    const auto scalar = Interpolate(ReadFloat(object + 0x80u, runtime),
                                     ReadFloat(record + 0x2cu, runtime),
                                     fraction, runtime);
    runtime.SetScalar(camera, scalar, flag);
    runtime.SetAngle(camera,
                     ReadFloat(object + 0x60u, runtime) +
                         fraction * ReadFloat(object + 0x88u, runtime),
                     flag);
    runtime.SetSecondAngle(camera,
                           ReadFloat(object + 0x5cu, runtime) +
                               fraction * ReadFloat(object + 0x8cu, runtime),
                           flag);
    return 1u;
}

}  // namespace pal4
