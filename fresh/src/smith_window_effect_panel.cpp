#include "pal4/smith_window.h"

#include <array>
#include <bit>
#include <cstdint>

namespace pal4 {
namespace {

const std::array<float, 3> kEffectPosition{
    std::bit_cast<float>(0xc26eae14u),
    std::bit_cast<float>(0x4348dc29u),
    std::bit_cast<float>(0x43952148u),
};
constexpr std::array<float, 3> kEffectScale{2.2f, 2.2f, 2.2f};

}  // namespace

std::uint8_t SmithWindow::ShowEffectPanel(
    const std::uint32_t object,
    ISmithWindowEffectPanelRuntime& runtime) {
    const auto shown = runtime.ReadByte(object + 0x80u);
    if (shown != 0u) return shown;

    runtime.SetVisible(runtime.ReadWord(object + 0x84u), true);
    runtime.SetVisible(runtime.ReadWord(object + 0x88u), true);
    runtime.SetVisible(runtime.ReadWord(object + 0x8cu), true);
    runtime.WriteByte(object + 0x80u, 1u);
    runtime.InsertFrameRecord(runtime.UIFrameManager() + 0x134u,
                              object + 0xe8u);
    const auto handle = object + 0xa4u;
    runtime.HandleEffectEvent(handle, 1u);
    const auto created = runtime.CreateEffect(
        runtime.UIFrameManager(), "LH", kEffectPosition,
        0xffffffffu, 0u);
    runtime.WriteWord(handle, created[0]);
    runtime.WriteWord(handle + 4u, created[1]);
    runtime.ResetTemporaryEffect();
    return runtime.UpdateEffectProperties(handle, kEffectScale, 0u);
}

}  // namespace pal4
