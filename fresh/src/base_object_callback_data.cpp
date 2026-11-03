#include "pal4/base_object_callback_data.h"

#include <bit>

namespace pal4 {
namespace {

float RaiseY(float value) {
    return static_cast<float>(static_cast<double>(value) +
                              static_cast<double>(0.30000001192092896f));
}

}  // namespace

std::uint32_t BaseObjectCallbackData::SetProperty1544(
    BaseObjectState& state, std::uint32_t target_address,
    std::uint32_t property) {
    state.Write(0x608u, property);
    return target_address;
}

std::uint32_t BaseObjectCallbackData::SetPositionAndTransformData(
    BaseObjectCallbackRecord& record, std::uint32_t target_address,
    const BaseObjectVector3& first,
    const BaseObjectVector3& second) {
    record.Write(0x60cu, std::bit_cast<std::uint32_t>(first.x));
    record.Write(0x610u, std::bit_cast<std::uint32_t>(RaiseY(first.y)));
    record.Write(0x624u, 0xdcf01414u);
    record.Write(0x614u, std::bit_cast<std::uint32_t>(first.z));
    record.Write(0x630u, std::bit_cast<std::uint32_t>(second.x));
    record.Write(0x634u, std::bit_cast<std::uint32_t>(RaiseY(second.y)));
    record.Write(0x648u, 0xdcf01414u);
    record.Write(0x638u, std::bit_cast<std::uint32_t>(second.z));
    return target_address;
}

bool BaseObjectCallbackData::CheckCollision(
    const BaseObjectVector3& first,
    const BaseObjectVector3& second, std::uint32_t argument,
    IBaseObjectCollisionRuntime& runtime) {
    if (runtime.ProcessCollision(first, second, argument)) return false;
    return !runtime.ProcessCollision(second, first, argument);
}

}  // namespace pal4
