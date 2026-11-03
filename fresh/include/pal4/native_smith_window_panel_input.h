#pragma once
#include <cstdint>
#include <functional>
#include <memory>
namespace pal4 {
class NativeSmithWindowLayout;class NativeSmithWindowTabs;class NativeSmithWindowConfirmation;class NativeEventHandlerArray;
// Borrows actual page/activity/dialog controllers. trade_selected must dispatch
// the existing NativeSmithWindowCraftActions, not mutate inventory directly.
class NativeSmithWindowPanelInput final {
public:
    NativeSmithWindowPanelInput(NativeSmithWindowLayout&,NativeSmithWindowTabs&,NativeSmithWindowConfirmation&,NativeEventHandlerArray&,std::function<bool()> trade_selected);
    ~NativeSmithWindowPanelInput();
    NativeSmithWindowPanelInput(const NativeSmithWindowPanelInput&)=delete;
    NativeSmithWindowPanelInput& operator=(const NativeSmithWindowPanelInput&)=delete;
    std::uint8_t Predicate(std::uint32_t member,std::uint32_t slot,std::uint32_t event);
private:
    class Runtime;std::shared_ptr<Runtime> runtime_;
};
}
