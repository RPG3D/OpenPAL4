#include "pal4/mission_window_input.h"

namespace pal4 {
std::uint8_t MissionWindowInput::TabKey(MissionWindowState& state,
    std::uint32_t navigation, std::uint32_t event,
    IUiNavigationInputRuntime& input, IMissionWindowTabRuntime& runtime) {
    const auto code = input.ReadWord(event + 16u);
    if (code == 1u) {
        runtime.HideMissionWindow(state);
        runtime.DispatchTabEvent(0x8a80acu, 1u, 0u);
        return 1u;
    }
    if (code == 28u) {
        const auto begin = input.ReadWord(navigation + 8u);
        const auto index = input.ReadWord(navigation + 20u);
        input.MouseAtCenter(input.ReadWord(begin + 4u * index));
        return 1u;
    }
    return UiNavigationList::HandleVerticalKey(navigation, event, input);
}

std::uint8_t MissionWindowInput::TabShortcut(MissionWindowState& state,
    std::uint32_t code, IMissionWindowCallbacksRuntime& runtime) {
    if (code == 2u) return MissionWindowCallbacks::OpenMission(state, runtime);
    if (code == 3u) return MissionWindowCallbacks::OpenQuest(state, runtime);
    return 0u;
}

std::uint8_t MissionWindowInput::ListKey(MissionWindowState& state,std::uint32_t code,
    IMissionWindowInputRuntime& runtime){
    if(code==1u){
        runtime.RemoveActiveMember(state,0x40u);
        MissionWindowVisibility::SetCharacterPanel(state,false,runtime);
        return 1u;
    }
    const auto list=state.Read(0x54u);
    const auto rows=runtime.RowCount(list);
    if(!rows)return 1u;
    if(code==200u){
        if(runtime.HasHoverItem(list) && runtime.HoverRow(list)>0)runtime.MoveHover(list,false);
        return 1u;
    }
    if(code==208u){
        if(runtime.HasHoverItem(list) && static_cast<std::uint32_t>(runtime.HoverRow(list))<rows-1u)
            runtime.MoveHover(list,true);
        return 1u;
    }
    if(code!=28u)return 0u;
    runtime.UpdateDetailsPanel(state);
    runtime.InsertActiveMember(state,0x48u);
    runtime.SetAlwaysOnTop(list,false);
    runtime.SetVisible(state.Read(0x84u),true);
    runtime.HandleMouseEvent(state.Read(0x84u));
    return 1u;
}
std::uint8_t MissionWindowInput::ChapterScrollKey(MissionWindowState& state,std::uint32_t code,
    IMissionWindowInputRuntime& runtime){
    if(code==1u){
        runtime.SetAlwaysOnTop(state.Read(0x54u),true);
        runtime.SetVisible(state.Read(0x84u),false);
        runtime.RemoveActiveMember(state,0x48u);
        return 1u;
    }
    if(code!=200u && code!=208u)return 0u;
    const auto list=state.Read(0x58u);
    runtime.SetStoryScrollPosition(list,runtime.StoryScrollPosition(list)+(code==200u?-15.0f:15.0f));
    runtime.RequestRedraw(list);
    return 1u;
}
}
