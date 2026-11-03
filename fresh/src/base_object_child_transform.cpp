#include "pal4/base_object_child_transform.h"

#include <bit>

namespace pal4 {
namespace {

float ReadFloat(const BaseObjectState& state, std::uint32_t offset) {
    return std::bit_cast<float>(state.Read(offset));
}

void WriteFloat(BaseObjectState& state, std::uint32_t offset, float value) {
    state.Write(offset, std::bit_cast<std::uint32_t>(value));
}

}  // namespace

double BaseObjectChildTransform::GetDoubleProperty(
    const BaseObjectState& state) {
    return GetDoublePropertyAtThis(state,0x1d8u);
}
double BaseObjectChildTransform::GetDoublePropertyAtThis(
    const BaseObjectState& state,std::uint32_t this_offset) {
    return static_cast<double>(ReadFloat(state,this_offset+0xdcu))*2.0;
}

std::uint32_t BaseObjectChildTransform::Update(
    BaseObjectState& state, std::uint32_t object_address,
    IBaseObjectChildTransformRuntime& runtime) {
    const auto parent = state.Read(0u);
    const auto frame = runtime.ReadDword(parent + 4u);
    const auto transform = runtime.GetObjectTransform(frame);
    const auto vector = runtime.ReadVector3(transform + 48u);
    WriteFloat(state, 0x1e0u, vector[0]);
    WriteFloat(state, 0x1e4u, vector[1]);
    WriteFloat(state, 0x1e8u, vector[2]);
    const auto displacement =
        static_cast<double>(ReadFloat(state, 0xd8u)) * 2.0 *
        static_cast<double>(0.9f);
    WriteFloat(state, 0x1e4u,
               static_cast<float>(static_cast<double>(vector[1]) +
                                  displacement));
    return object_address + 0x1e0u;
}

std::uint32_t BaseObjectChildTransform::GetParent(
    const BaseObjectState& state) {
    return state.Read(0u);
}

}  // namespace pal4
