#include "pal4/render_system_object.h"

namespace pal4 {

std::uint32_t RenderSystemObjectState::ReadDword(
    const std::uint32_t offset) const {
    return static_cast<std::uint32_t>(bytes_.at(offset)) |
           (static_cast<std::uint32_t>(bytes_.at(offset + 1u)) << 8u) |
           (static_cast<std::uint32_t>(bytes_.at(offset + 2u)) << 16u) |
           (static_cast<std::uint32_t>(bytes_.at(offset + 3u)) << 24u);
}

void RenderSystemObjectState::WriteDword(const std::uint32_t offset,
                                          const std::uint32_t value) {
    for (std::uint32_t index = 0u; index < 4u; ++index) {
        bytes_.at(offset + index) =
            static_cast<std::uint8_t>(value >> (8u * index));
    }
}

std::uint32_t RenderSystemObject::Create(
    IRenderSystemObjectRuntime& runtime) {
    const std::uint32_t address =
        runtime.Allocate(RenderSystemObjectState::kBytes);
    return address == 0u ? 0u : Construct(address, runtime);
}

std::uint32_t RenderSystemObject::Construct(
    const std::uint32_t address, IRenderSystemObjectRuntime& runtime) {
    RenderSystemObjectState& state = runtime.Access(address);
    runtime.ClearObject(address);
    runtime.InitializeString(address + kStringOffset);
    state.WriteDword(0x74u, 0u);
    state.WriteDword(0x70u, 0u);
    state.WriteDword(0u, kManagerFactory);
    state.WriteDword(4u, kManagerDestructor);
    state.WriteDword(8u, kBufferInitializer);
    runtime.InitializeSecondaryBase(address);
    runtime.ClearCollection(address);
    for (std::uint32_t offset = 0x2cu; offset <= 0x64u; offset += 4u) {
        state.WriteDword(offset, 0u);
    }
    return address;
}

void RenderSystemObject::Destruct(
    const std::uint32_t address, IRenderSystemObjectRuntime& runtime) {
    RenderSystemObjectState& state = runtime.Access(address);
    runtime.ClearCollection(address);
    state.WriteDword(0x2cu, 0u);
    runtime.WriteByte(state.ReadDword(kStringOffset), 0u);
    state.WriteDword(kStringOffset + 4u, 0u);
    runtime.CleanupString(address + kStringOffset);
    runtime.CleanupBase(address);
}

void RenderSystemObject::Destroy(
    const std::uint32_t address, IRenderSystemObjectRuntime& runtime) {
    if (address == 0u) {
        return;
    }
    Destruct(address, runtime);
    runtime.Free(address);
}

}  // namespace pal4
