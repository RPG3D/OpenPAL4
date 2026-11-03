#pragma once
#include "pal4/mission_window_callbacks.h"
#include "pal4/mission_window_visibility.h"
#include "pal4/ui_navigation_list.h"

namespace pal4 {
class IMissionWindowTabRuntime {
public:
    virtual ~IMissionWindowTabRuntime() = default;
    virtual void HideMissionWindow(MissionWindowState&) = 0;
    virtual void DispatchTabEvent(std::uint32_t key, std::uint32_t first,
                                  std::uint32_t second) = 0;
};
class IMissionWindowInputRuntime : public IMissionWindowListCallbackRuntime,
                                   public IMissionWindowPanelRuntime {
public:
    virtual void SetVisible(std::uint32_t window,bool visible) override=0;
    virtual void RemoveActiveMember(MissionWindowState&,std::uint32_t offset)=0;
    virtual std::uint32_t RowCount(std::uint32_t list)=0;
    virtual bool HasHoverItem(std::uint32_t list)=0;
    virtual std::int32_t HoverRow(std::uint32_t list)=0;
    virtual void MoveHover(std::uint32_t list,bool down)=0;
    virtual float StoryScrollPosition(std::uint32_t list)=0;
    virtual void SetStoryScrollPosition(std::uint32_t list,float)=0;
    virtual void RequestRedraw(std::uint32_t list)=0;
};
class MissionWindowInput final {
public:
    // IDA4700D0. navigation is the live owner+0x28 member; event retains
    // the original scan code at +16. Reuse the shared vertical navigation.
    static std::uint8_t TabKey(MissionWindowState&, std::uint32_t navigation,
        std::uint32_t event, IUiNavigationInputRuntime&, IMissionWindowTabRuntime&);
    // IDA470150. code is the original event+16 scan code.
    static std::uint8_t TabShortcut(MissionWindowState&, std::uint32_t code,
        IMissionWindowCallbacksRuntime&);
    // IDA470180/4702C0. code is the original event+16 scan code.
    static std::uint8_t ListKey(MissionWindowState&,std::uint32_t code,IMissionWindowInputRuntime&);
    static std::uint8_t ChapterScrollKey(MissionWindowState&,std::uint32_t code,IMissionWindowInputRuntime&);
};
}
