#include "pal4/world_map_fly_callback.h"

namespace pal4 {
namespace {

void ReleaseText(const std::uint32_t data,
                 IWorldMapFlyCallbackRuntime& runtime) {
    if (data == 0u) return;
    const auto reference = runtime.ReadByte(data - 1u);
    if (reference == 0u || reference == 0xffu) {
        static_cast<void>(runtime.Free(data - 1u));
    } else {
        runtime.WriteByte(data - 1u,
                          static_cast<std::uint8_t>(reference - 1u));
    }
}

}  // namespace

std::uint32_t WorldMapFlyCallback::Construct(
    const std::uint32_t object, const std::uint32_t owner,
    const std::uint32_t function, const std::uint32_t adjustment,
    const std::uint32_t source, IWorldMapFlyCallbackRuntime& runtime) {
    const auto text = object + 0x10u;
    runtime.WriteByte(text, runtime.ReadByte(source));
    runtime.WriteWord(text + 4u, 0u);
    runtime.WriteWord(text + 8u, 0u);
    runtime.WriteWord(text + 0xcu, 0u);
    runtime.WriteWord(object + 0xcu, adjustment);
    runtime.WriteWord(object, kVtable);
    runtime.WriteWord(object + 4u, owner);
    runtime.WriteWord(object + 8u, function);

    bool release_attempted = false;
    try {
        const auto length = runtime.ReadWord(source + 8u);
        if (text == source) {
            runtime.EraseText(text, length, 0xffffffffu);
            runtime.EraseText(text, 0u, 0u);
        } else {
            std::uint32_t shared_data = 0u;
            bool can_share = false;
            if (length != 0u) {
                shared_data = runtime.ReadWord(source + 4u);
                if (shared_data == 0u) shared_data = runtime.EmptyText();
                can_share = runtime.ReadByte(shared_data - 1u) < 0xfeu;
            }
            if (can_share) {
                runtime.SetErrorFlag(text, 1u);
                runtime.WriteWord(text + 4u, shared_data);
                runtime.WriteWord(text + 8u, length);
                runtime.WriteWord(text + 0xcu,
                                  runtime.ReadWord(source + 0xcu));
                runtime.WriteByte(shared_data - 1u,
                    static_cast<std::uint8_t>(
                        runtime.ReadByte(shared_data - 1u) + 1u));
            } else if (runtime.AllocateText(text, length, 1u)) {
                const auto destination = runtime.ReadWord(text + 4u);
                const auto source_data = runtime.ReadWord(source + 4u);
                const auto data = source_data != 0u ? source_data :
                                  runtime.EmptyText();
                runtime.Copy(destination, data, length);
                runtime.WriteWord(text + 8u, length);
                runtime.WriteByte(destination + length, 0u);
            }
        }
        release_attempted = true;
        ReleaseText(runtime.ReadWord(source + 4u), runtime);
    } catch (...) {
        if (!release_attempted) {
            ReleaseText(runtime.ReadWord(source + 4u), runtime);
        }
        static_cast<void>(Destroy(object, runtime));
        throw;
    }
    return object;
}

std::uint32_t WorldMapFlyCallback::Destroy(
    const std::uint32_t object, IWorldMapFlyCallbackRuntime& runtime) {
    const auto data = runtime.ReadWord(object + 0x14u);
    std::uint32_t result = data;
    if (data != 0u) {
        const auto reference = runtime.ReadByte(data - 1u);
        result = data - 1u;
        if (reference == 0u || reference == 0xffu) {
            result = runtime.Free(result);
        } else {
            runtime.WriteByte(result,
                static_cast<std::uint8_t>(reference - 1u));
        }
    }
    runtime.WriteWord(object + 0x14u, 0u);
    runtime.WriteWord(object + 0x18u, 0u);
    runtime.WriteWord(object + 0x1cu, 0u);
    runtime.WriteWord(object, kBaseVtable);
    return result;
}

std::uint32_t WorldMapFlyCallback::Delete(
    const std::uint32_t object, const std::uint8_t flags,
    IWorldMapFlyCallbackRuntime& runtime) {
    static_cast<void>(Destroy(object, runtime));
    if ((flags & 1u) != 0u) {
        static_cast<void>(runtime.Free(object));
    }
    return object;
}

}  // namespace pal4
