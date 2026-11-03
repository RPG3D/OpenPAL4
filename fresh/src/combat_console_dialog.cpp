#include "pal4/combat_console.h"
#include "pal4/preview_text_codec.h"

#include <string>

namespace pal4 {
namespace {

void AppendNameRecords(const std::uint32_t console,
                       ICombatConsoleDialogIdIterator& names,
                       std::string& text,
                       ICombatConsoleDialogRuntime& runtime) {
    while (!names.AtEnd()) {
        const auto id = names.CurrentId();
        if (id > 0) text += runtime.ConsoleStatusMarkup(console,id);
        names.Advance();
    }
}

void DisplayScenario(const std::uint32_t console, const std::uint32_t event,
                     ICombatConsoleDialogRuntime& runtime) {
    const auto parent = runtime.ReadWord(console + 36u);
    const auto config_position = runtime.GameConfigPosition();
    const auto frame_position = runtime.FrameCoordinates(config_position);
    const auto local_position = runtime.ScreenToWindow(parent, frame_position);
    const auto offset = runtime.AbsoluteToRelative(parent, {32.0f, 32.0f});
    const CombatConsolePosition position{
        local_position.x + offset.x, local_position.y + offset.y};
    const auto panel = runtime.ReadWord(console + 260u);
    runtime.SetVisible(panel, true);
    runtime.SetPosition(panel, position);

    const auto actor = runtime.ActorFromEvent(event);
    auto text = runtime.ReadDialogText(actor + 1032u);
    text += "<br/>";
    auto character_names = runtime.CharacterNameIds(actor);
    AppendNameRecords(console, *character_names, text, runtime);
    auto speaker_names = runtime.SpeakerNameIds(actor);
    AppendNameRecords(console, *speaker_names, text, runtime);

    runtime.SetVisible(panel, true);
    runtime.SetText(runtime.ReadWord(console + 264u),
                    PreviewTextCodec::DecodeUtf8(
                        runtime.ConvertToUtf8(text)));
    CombatConsoleWindow::UpdateDialogLayout(console, runtime);
}

void DisplayCombat(const std::uint32_t console, const std::uint32_t actor,
                   ICombatConsoleDialogRuntime& runtime) {
    const auto system = runtime.CombatSystem();
    const auto camera = runtime.CurrentCombatCamera();
    auto position = runtime.ActorScreenPosition(actor, camera);
    const auto panel = runtime.ReadWord(console + 260u);
    runtime.SetVisible(panel, true);
    position.y -= 30.0f;
    runtime.SetPosition(
        panel, runtime.ScreenToWindow(runtime.ReadWord(console + 36u),
                                      position));

    const auto total = runtime.DialogTotal(actor, 0u);
    const auto current = runtime.DialogCurrentIndex(actor, 7u, total);
    const auto text =
        "<colour red=255 green=248 blue=192><font name=dialog_simsun>" +
        std::to_string(current) + "/" + std::to_string(total) +
        "</font></colour>";
    runtime.SetVisible(panel, true);
    runtime.SetText(runtime.ReadWord(console + 264u),
                    PreviewTextCodec::DecodeUtf8(
                        runtime.ConvertToUtf8(text)));
    CombatConsoleWindow::UpdateDialogLayout(console, runtime);
    runtime.SetXPosition(
        panel, runtime.GetXPosition(panel) - runtime.GetWidth(panel) * 0.5f);
    (void)system;
}

}  // namespace

std::string ICombatConsoleDialogRuntime::ConsoleStatusMarkup(std::uint32_t console,std::int32_t id){
    if(id<=0)return {};
    const auto begin=ReadWord(console+296u);
    if(!begin)return {};
    const auto end=ReadWord(console+300u);
    return end>=begin && static_cast<std::uint32_t>(id)<(end-begin)/16u
        ?ReadTextRecord(begin+static_cast<std::uint32_t>(id)*16u):std::string{};
}

bool CombatConsoleWindow::DisplayDialog(
    const std::uint32_t console, const std::uint32_t event,
    ICombatConsoleDialogRuntime& runtime) {
    const auto actor = runtime.ActorFromEvent(event);
    const auto type_actor = runtime.ActorFromEvent(event);
    if (runtime.ReadWord(type_actor + 804u) == 2u) {
        const auto system = runtime.CombatSystem();
        if (runtime.ReadWord(system + 508u) != 5u) return false;
        DisplayScenario(console, event, runtime);
        return true;
    }

    const auto system = runtime.CombatSystem();
    if (!runtime.IsCombatEnding(system)) return false;
    if (runtime.ReadWord(runtime.CombatSystem() + 508u) != 5u)
        return false;
    DisplayCombat(console, actor, runtime);
    return true;
}

}  // namespace pal4
