#include "pal4/mission_window_lifecycle.h"

namespace pal4 {
namespace {

struct WindowBinding {
    std::string_view path;
    std::uint32_t offset;
    bool by_name;
};

constexpr std::array<WindowBinding, 13> kBindings{{
    {"MissionWindow/List", 0x54u, true},
    {"MissionWindow/ListStory", 0x58u, false},
    {"MissionWindow/ImgMission", 0x68u, true},
    {"MissionWindow/PanelPicture", 0x70u, false},
    {"MissionWindow/PanelDesc", 0x78u, false},
    {"MissionWindow/PanelScenario", 0x6cu, true},
    {"MissionWindow/ListWindow", 0x74u, true},
    {"MissionWindow/EditDescription", 0x5cu, true},
    {"MissionWindow/BtnMission", 0x60u, true},
    {"MissionWindow/BtnScenario", 0x64u, true},
    {"MissionWindow/zj", 0x7cu, false},
    {"MissionWindow/ImageLabel", 0x80u, false},
    {"MissionWindow/ImgDisusedMask", 0x84u, false},
}};

void ConfigureList(IMissionWindowLifecycleRuntime& runtime,
                   std::uint32_t list) {
    runtime.SetUserColumnSizing(list, false);
    runtime.SetUserColumnDragging(list, false);
    runtime.SetUserSortControl(list, false);
}

}  // namespace

MissionWindowState& MissionWindowLifecycle::Construct(
    MissionWindowState& state, std::uint32_t name,
    IMissionWindowLifecycleRuntime& runtime) {
    runtime.LoadLayout(state, name, 0u, 0u);
    bool member_ready = false;
    try {
        runtime.InitializeMember(state);
        member_ready = true;
        state.Write(0x28u, kInitialMemberVtable);
        state.Write(0x40u, 0x8439fcu);
        state.Write(0x48u, 0x8439e4u);
        state.bytes[0x88u] = 1u;
        state.Write(0u, kObjectVtable);
        state.Write(0x28u, kMemberVtable);

        for (const auto& binding : kBindings) {
            state.Write(binding.offset,
                        runtime.FindWindow(binding.path, binding.by_name));
        }
        runtime.SetAlwaysOnTop(state.Read(0x84u), true);

        const auto owner = runtime.ObjectAddress(state);
        runtime.Subscribe(state.Read(0x60u), MissionWindowEvent::Clicked,
                          0x46fdd0u, owner, 24u, kCallbackVtable);
        runtime.Subscribe(state.Read(0x64u), MissionWindowEvent::Clicked,
                          0x46fe10u, owner, 24u, kCallbackVtable);
        runtime.Subscribe(state.Read(0x54u),
                          MissionWindowEvent::HoverRowChanged,
                          0x46fe60u, owner, 24u, kCallbackVtable);
        runtime.Subscribe(state.Read(0x54u), MissionWindowEvent::MouseClick,
                          0x46fe70u, owner, 24u, kCallbackVtable);

        ConfigureList(runtime, state.Read(0x54u));
        const auto story_list = state.Read(0x58u);
        runtime.SetListHeaderByte(story_list, 1239u, 0u);
        ConfigureList(runtime, story_list);
        while (runtime.ColumnCount(story_list) != 0u) {
            runtime.RemoveColumn(story_list, 0u);
        }
        runtime.ResetList(story_list);
        const auto title = runtime.TriggerTitle(193u);
        const auto utf8 = runtime.ConvertToUtf8(title);
        runtime.AddColumn(story_list, utf8, 0u, 0.89999998f);
        runtime.SetListHeaderByte(story_list, 1239u, 0u);
        runtime.Subscribe(story_list, MissionWindowEvent::MouseClick,
                          0x46ffa0u, owner, 24u, kCallbackVtable);

        runtime.SetEnabled(state.Read(0x5cu), false);
        runtime.SetupMouseHandlers(state, state.Read(0x60u));
        runtime.SetupMouseHandlers(state, state.Read(0x64u));
        state.Write(0x44u, owner);
        state.Write(0x50u, story_list);
        state.Write(0x4cu, owner);
    } catch (...) {
        if (member_ready) runtime.DestroyMember(state);
        runtime.DestroyDialogBase(state);
        throw;
    }
    return state;
}

}  // namespace pal4
