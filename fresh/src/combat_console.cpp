#include "pal4/combat_console.h"
#include "pal4/script_game_event_subscriptions.h"

#include <array>
#include <bit>
#include <string_view>

namespace pal4 {
namespace {

constexpr std::array<std::string_view, 28> kIconMarkup{{
    "<image set=kuang0 name=ding/>",
    "<image set=kuang0 name=feng/>",
    "<image set=kuang0 name=jing/>",
    "<image set=kuang0 name=mian/>",
    "<image set=kuang0 name=luan/>",
    "<image set=kuang0 name=kuang/>",
    "<image set=kuang0 name=shui/>",
    "<image set=kuang0 name=huo/>",
    "<image set=kuang0 name=lei/>",
    "<image set=kuang0 name=fen/>",
    "<image set=kuang0 name=tu/>",
    "<image set=kuang0 name=jin/>",
    "<image set=kuang0 name=bi/>",
    "<image set=kuang0 name=jie/>",
    "<image set=kuang0 name=ying/>",
    "<image set=kuang0 name=ning/>",
    "<image set=kuang0 name=bi2/>",
    "<image set=kuang0 name=sheng/>",
    "<image set=kuang0 name=wu1/>",
    "<image set=kuang0 name=fang1/>",
    "<image set=kuang0 name=su1/>",
    "<image set=kuang0 name=yun1/>",
    "<image set=kuang0 name=ling1/>",
    "<image set=kuang0 name=wu2/>",
    "<image set=kuang0 name=fang2/>",
    "<image set=kuang0 name=su2/>",
    "<image set=kuang0 name=yun2/>",
    "<image set=kuang0 name=ling2/>",
}};

void WriteFloat(ICombatConsoleConstructionRuntime& runtime, const std::uint32_t address,
                const float value) {
    runtime.WriteWord(address, std::bit_cast<std::uint32_t>(value));
}

}  // namespace

std::uint32_t CombatConsoleWindow::Construct(
    const std::uint32_t console, const std::string_view layout,
    ICombatConsoleConstructionRuntime& runtime) {
    runtime.LoadLayout(console, layout, 0u, 0u);
    runtime.WriteWord(console + 40u, kBaseVtable);
    for (std::uint32_t index = 0u; index < 19u; ++index) {
        runtime.WriteWord(console + 60u + index * 8u, 0u);
        runtime.WriteWord(console + 64u + index * 8u, 0u);
    }
    runtime.WriteWord(console + 268u, 0u);
    runtime.WriteWord(console + 272u, 0u);
    runtime.SetTextErrorFlag(console + 276u, 0u);
    runtime.SetText(console + 276u, "ResetCombatIcon");
    runtime.WriteByte(console + 292u, 0u);
    runtime.WriteWord(console + 296u, 0u);
    runtime.WriteWord(console + 300u, 0u);
    runtime.WriteWord(console + 304u, 0u);
    runtime.WriteWord(console, kVtable);
    runtime.WriteWord(console + 40u, kFieldVtable);

    (void)InitializeMainWindow(console, runtime);
    const auto main_window = runtime.ReadWord(console + 212u);
    const auto position = runtime.GetPosition(main_window, 1u);
    const auto width = runtime.GetWidth(main_window, 1u);
    WriteFloat(runtime, console + 44u, position.x - 10.0f);
    WriteFloat(runtime, console + 48u, position.y - 15.0f);
    WriteFloat(runtime, console + 52u, position.x + width - 55.0f);
    WriteFloat(runtime, console + 56u, position.y - 15.0f);

    runtime.AppendLegacyText(console + 292u, "");
    for (std::uint32_t index = 0u; index < kIconMarkup.size(); ++index) {
        if (index < 21u)
            runtime.AppendLegacyText(console + 292u, kIconMarkup[index]);
        else
            runtime.AppendArrayText(console + 292u, kIconMarkup[index]);
    }
    return console;
}

std::uint32_t CombatConsoleWindow::Destruct(
    const std::uint32_t console, ICombatConsoleRuntime& runtime) {
    runtime.WriteWord(console, kVtable);
    runtime.WriteWord(console + 40u, kFieldVtable);
    const auto begin = runtime.ReadWord(console + 296u);
    const auto end = runtime.ReadWord(console + 300u);
    for (auto record = begin; record != end; record += 16u)
        runtime.SetTextErrorFlag(record, 1u);
    runtime.Free(begin);
    runtime.WriteWord(console + 296u, 0u);
    runtime.WriteWord(console + 300u, 0u);
    runtime.WriteWord(console + 304u, 0u);

    const auto text = runtime.ReadWord(console + 280u);
    if (text != 0u) {
        const auto references = runtime.ReadByte(text - 1u);
        if (references == 0u || references == 0xffu)
            runtime.Free(text - 1u);
        else
            runtime.WriteByte(text - 1u,
                              static_cast<std::uint8_t>(references - 1u));
    }
    runtime.WriteWord(console + 280u, 0u);
    runtime.WriteWord(console + 284u, 0u);
    runtime.WriteWord(console + 288u, 0u);
    return runtime.DestroyBase(console);
}

std::uint32_t CombatConsoleWindow::SetVisible(
    const std::uint32_t console, const bool visible,
    const std::uint32_t parent,
    ICombatConsoleVisibilityServices& runtime) {
    runtime.SetParent(console, visible, parent);
    std::uint32_t result = 0u;
    if (visible) {
        runtime.SetChildVisible(runtime.ReadWord(console + 216u), false);
        runtime.RegisterGameEvents(console);
        runtime.SetChildVisible(runtime.ReadWord(console + 260u), false);
        result = runtime.AddHandler(runtime.FrameManager() + 308u,
                                    console + 40u);
    } else {
        runtime.CleanupGameEvents(console);
        runtime.RemoveHandler(runtime.FrameManager() + 308u,
                              console + 40u);
        const auto manager = runtime.FrameManager();
        result = runtime.RemoveTimer(runtime.ReadWord(manager + 416u),
                                     console + 276u);
    }
    runtime.WriteWord(console + 308u, 0u);
    runtime.WriteWord(console + 312u, 0xc0200000u);
    return result;
}

void ICombatConsoleVisibilityRuntime::RegisterGameEvents(std::uint32_t owner) {
    (void)ScriptGameEventSubscriptions::Register(owner,*this);
}
void ICombatConsoleVisibilityRuntime::CleanupGameEvents(std::uint32_t owner) {
    (void)ScriptGameEventSubscriptions::Cleanup(owner,*this);
}

}  // namespace pal4
