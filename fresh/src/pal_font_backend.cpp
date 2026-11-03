#include "pal4/pal_font_backend.h"

namespace pal4 {

std::uint32_t PalFontBackendRuntimeStub::CurrentD3DDevice() { return 0u; }
std::int32_t PalFontBackendRuntimeStub::CreateFromDescription(
    std::uint32_t /*device*/,
    std::int32_t /*height*/, std::int32_t /*width*/,
    std::int32_t /*weight*/, std::int32_t /*sixth*/,
    std::int32_t /*fourth*/,
    const std::array<std::uint8_t, 4>& /*options*/,
    std::uint32_t /*face_name*/,
    std::uint32_t /*font_object*/) { return 0; }

bool PalFontBackend::CreateFromDescription(
    const std::uint32_t font_object,
    const PalFontDescription& description,
    IPalFontBackendRuntime& runtime) {
    return runtime.CreateFromDescription(
               runtime.CurrentD3DDevice(), description.height,
               description.width, description.weight,
               description.sixth, description.fourth,
               description.options, description.face_name,
               font_object) == 0;
}

std::uint32_t RenderStateObjectRuntimeStub::CallSlot(
    std::uint32_t /*object*/, std::uint32_t /*byte_offset*/) { return 0u; }
std::uint32_t RenderStateObjectRuntimeStub::DrawRectangleSlot(
    std::uint32_t /*object*/, std::uint32_t /*first*/,
    std::int32_t /*command*/, std::int32_t /*sentinel*/,
    const std::array<std::int32_t, 4>& /*rectangle*/,
    std::int32_t /*last*/, std::int32_t /*penultimate*/) { return 0u; }

std::uint32_t RenderStateObject::GetTextureSlot40(
    const std::uint32_t object, IRenderStateObjectRuntime& runtime) {
    return runtime.CallSlot(object, 0x40u);
}

std::uint32_t RenderStateObject::GetTextureSlot44(
    const std::uint32_t object, IRenderStateObjectRuntime& runtime) {
    return runtime.CallSlot(object, 0x44u);
}

std::uint32_t RenderStateObject::Release(
    std::uint32_t& object, IRenderStateObjectRuntime& runtime) {
    if (object == 0u) {
        return 0u;
    }
    const auto result = runtime.CallSlot(object, 8u);
    object = 0u;
    return result;
}

std::uint32_t RenderStateObject::DrawRectangle(
    const std::uint32_t object, const std::int32_t command,
    const std::array<std::int32_t, 4>& rectangle,
    const std::int32_t penultimate, const std::int32_t last,
    IRenderStateObjectRuntime& runtime) {
    return runtime.DrawRectangleSlot(object, 0u, command, -1,
                                     rectangle, last, penultimate);
}

}  // namespace pal4
