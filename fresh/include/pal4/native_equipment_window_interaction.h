#pragma once
#include "pal4/equipment_window.h"
#include <functional>
#include <memory>
#include <string_view>
namespace pal4 {
class NativeCeguiUiSession;class NativeInventorySession;class NativeGameDatabase;
class NativeEquipmentWindowLayout;class NativeEquipmentWindowPresentation;
class NativeEquipmentItemDetails;class NativeEquipmentWindowActions;class NativePlayerScriptRefreshRuntime;
class NativeEventHandlerArray;class NativeMenuWindow;struct PlayerManagerObjectState;
struct NativeEquipmentInteractionServices {
    std::function<void(std::string_view,std::uint32_t,std::uint32_t)> event;
    std::function<void(std::uint32_t)> tutorial;
    std::function<void(std::uint32_t)> model_update;
    std::function<void(bool)> model_visible;
    std::function<void(std::string_view,std::uint32_t)> sound;
};
// Borrows page components; owns the selection popup and slot-input adapter.
// Layout's handler forwards to FromDll; global activities route Callback/Predicate.
class NativeEquipmentWindowInteraction final {
public:
    NativeEquipmentWindowInteraction(NativeCeguiUiSession&,NativeEquipmentWindowLayout&,
        NativeInventorySession&,PlayerManagerObjectState&,const NativeGameDatabase&,
        NativeEquipmentWindowPresentation&,NativeEquipmentItemDetails&,NativeEquipmentWindowActions&,
        NativePlayerScriptRefreshRuntime&,NativeEventHandlerArray&,std::uint32_t manager,
        NativeEquipmentInteractionServices);
    ~NativeEquipmentWindowInteraction();
    NativeEquipmentWindowInteraction(const NativeEquipmentWindowInteraction&)=delete;
    NativeEquipmentWindowInteraction& operator=(const NativeEquipmentWindowInteraction&)=delete;
    void SetVisible(bool,std::uint32_t parent=0u);
    bool FromDll(std::uint32_t owner,std::uint32_t handler,std::uint32_t argument);
    bool OwnsActivity(std::uint32_t)const noexcept;
    std::uint32_t Callback(std::uint32_t member,std::uint32_t slot);
    std::uint8_t Predicate(std::uint32_t member,std::uint32_t slot,std::uint32_t event);
    NativeMenuWindow& Menu();
    void RethrowEventError();
    void Close();
private:
    class Runtime;
    std::unique_ptr<Runtime> runtime_;
};
}
