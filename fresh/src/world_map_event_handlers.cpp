#include "pal4/world_map_event_handlers.h"

#include <array>

namespace pal4 {
namespace {

void Bind(const std::uint32_t frame, const std::uint32_t window,
          const std::uint32_t event_address,
          const std::uint32_t function, IWorldMapEventRuntime& runtime) {
    const auto callback = runtime.Allocate(24u);
    if (callback != 0u) {
        runtime.WriteWord(callback + 16u, frame);
        runtime.WriteWord(callback + 8u, function);
        runtime.WriteWord(callback + 12u, 0u);
        runtime.WriteWord(callback, WorldMapEventHandlers::kCallbackVtable);
    }
    const auto event = runtime.ReadWord(event_address);
    const auto connection = runtime.Subscribe(window, event, callback);
    if (connection != 0u) {
        runtime.ReleaseConnection(connection);
    }
}

constexpr std::array<std::uint32_t, WorldMapEventHandlers::kButtonCount>
    kMouseOrder{0x78u, 0x44u, 0x7cu, 0x48u, 0x54u, 0x58u, 0x5cu,
                0x60u, 0x68u, 0x8cu, 0x80u, 0x6cu, 0x88u, 0x40u,
                0x90u, 0x70u, 0x4cu, 0x50u, 0x84u, 0x74u, 0x64u};

}  // namespace

std::uint32_t WorldMapEventHandlers::Initialize(
    const std::uint32_t frame, IWorldMapEventRuntime& runtime) {
    const auto name = runtime.BeginName();
    try {
        for (std::uint32_t index = 0u; index < kButtonCount; ++index) {
            runtime.FormatButtonName(name, runtime.ReadWord(frame + 0x18u),
                                     index + 1u);
            const auto window = runtime.FindWindow(frame, name);
            runtime.WriteWord(frame + 0x40u + index * 4u, window);
            runtime.WriteWord(window + 0x118u, index);
            Bind(frame, window, kClickedEvent, kOnLocationClick, runtime);
            Bind(frame, window, kClickedEvent, kOnLocationClick, runtime);
        }

        runtime.FormatRoleName(name, runtime.ReadWord(frame + 0x18u));
        const auto role = runtime.FindWindow(frame, name);
        runtime.WriteWord(frame + 0x238u, role);
        runtime.SetEnabled(role, false);
        Bind(frame, runtime.ReadWord(frame + 0x24u),
             kKeyDownEvent, kOnKeyDown, runtime);

        for (const auto offset : kMouseOrder) {
            runtime.SetupMouseHandlers(frame + 0x28u,
                                       runtime.ReadWord(frame + offset));
        }
    } catch (...) {
        (void)runtime.EndName(name);
        throw;
    }
    return runtime.EndName(name);
}

std::uint32_t WorldMapEventHandlers::ForwardMouseSlot0(
    const std::uint32_t frame, IWorldMapMouseDispatchRuntime& runtime) {
    const auto mouse = frame + 0x28u;
    const auto vtable = runtime.ReadWord(mouse);
    const auto function = runtime.ReadWord(vtable);
    return runtime.CallVirtual(mouse, function);
}

}  // namespace pal4
