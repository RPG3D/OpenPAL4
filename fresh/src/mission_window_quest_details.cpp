#include "pal4/mission_window_quest_details.h"

namespace pal4 {

void MissionWindowQuestDetails::Update(
    const MissionWindowState& state, std::uint32_t quest,
    IMissionWindowQuestDetailsRuntime& runtime) {
    if (quest == 0u) return;
    runtime.SetVisible(state.Read(0x84u), false);
    const auto description = runtime.QuestDescription(quest, 0x38u);
    const auto utf8 = runtime.ConvertToUtf8(description);
    runtime.SetText(state.Read(0x5cu), utf8);
    const auto image = runtime.LoadQuestImage(quest);
    runtime.SetImage(state.Read(0x68u), image);
}

}  // namespace pal4
