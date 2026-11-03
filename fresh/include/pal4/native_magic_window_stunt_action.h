#pragma once
#include "pal4/native_magic_window_cast_action.h"
#include <memory>
namespace pal4 {
class NativeCeguiUiSession;class NativeEventHandlerArray;class NativeInventorySession;
class NativeBaseObjectMemory;
// Original Stunt popup -> use_stunt -> RoleSelector -> requirements/cost/effect.
// All borrowed page/game/party owners must outlive Close and in-flight calls.
class NativeMagicWindowStuntAction final {
public:
    NativeMagicWindowStuntAction(NativeCeguiUiSession&,NativeEventHandlerArray&,
        NativeMagicWindowLayout&,PlayerManagerObjectState&,NativeBaseObjectMemory&,
        const NativeMagicWindowRecords&,const NativeGameDatabase&,NativeInventorySession&,
        NativeRoleSelector&,NativePlayerSkillRuntime&,NativeMagicCastServices);
    ~NativeMagicWindowStuntAction();
    NativeMagicWindowStuntAction(const NativeMagicWindowStuntAction&)=delete;
    NativeMagicWindowStuntAction& operator=(const NativeMagicWindowStuntAction&)=delete;
    bool ShowMenu(std::uint32_t event_payload,std::uint32_t stunt_key);
    bool Use(std::uint32_t stunt_key);
    bool CanUse(std::uint32_t stunt_key);
    std::uint32_t MenuButton()const;
    bool MenuVisible()const;
    void RethrowEventError();
    void Close();
private:
    class Runtime;std::shared_ptr<Runtime> runtime_;
};
}
