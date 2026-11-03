#include "support/native_system_property_probe.h"
#include "pal4/native_trade_window_layout.h"
#include "pal4/native_cegui_ui_session.h"
#include "pal4/native_cegui_window_pointer_events.h"
#include "pal4/native_cegui_widget_images.h"
#include "pal4/trade_window.h"
#include "../native/ui/cegui_exports_native.h"
#include <array>
#include <iostream>
#include <set>
#include <string>
#include <vector>
namespace pal4::test {
namespace {
void Check(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
unsigned Address(const void* p){return reinterpret_cast<unsigned>(p);}
}
void VerifyNativeTradeWindowLayout(const std::filesystem::path& root,NativeCeguiUiSession& ui){
    const auto module=GetModuleHandleA("CEGUIBase.dll");
    const auto wm=detail::Export<void* (__cdecl*)()>(module,"?getSingletonPtr@WindowManager@CEGUI@@SAPAV12@XZ")();
    const auto find=[&](std::string_view name){detail::NativeString text(module,name);
        return Address(detail::Export<void* (__thiscall*)(const void*,const void*)>(module,"?getWindow@WindowManager@CEGUI@@QBEPAVWindow@2@ABVString@2@@Z")(wm,text.Data()));};
    const auto count=detail::Export<unsigned (__thiscall*)(const void*)>(module,"?getColumnCount@MultiColumnList@CEGUI@@QBEIXZ");
    const auto rows=detail::Export<unsigned (__thiscall*)(const void*)>(module,"?getRowCount@MultiColumnList@CEGUI@@QBEIXZ");
    const auto fire=detail::Export<void (__thiscall*)(void*,const void*,void*,const void*)>(module,"?fireEvent@EventSet@CEGUI@@UAEXABVString@2@AAVEventArgs@2@0@Z");
    std::array<unsigned,0x224u/4u> manager{};
    for(unsigned cycle=0;cycle<2u;++cycle){
        NativeTradeWindowLayout* active{};std::vector<unsigned> methods;unsigned retired{};bool guarded{};
        NativeTradeWindowLayout layout(ui,root,Address(manager.data()),[&](unsigned method,unsigned event){
            Check(active && active->InCallback() && event,"Trade callbacks borrow active layout and original event");
            if(guarded)active->Close();methods.push_back(method);return true;
        },[&]{++retired;});active=&layout;
        Check(layout.Word(0u)==TradeWindow::kVtable && layout.Word(0x19cu)==0x843d68u && layout.Word(0x1a8u)==0x843d50u &&
            layout.Word(0x1a0u)==layout.FrameAddress() && layout.Word(0x1acu)==layout.FrameAddress() &&
            layout.Word(0x128u)==0xffffffffu && !layout.Word(0x12cu),"original Trade frame and two input component identities");
        Check(std::string(reinterpret_cast<const char*>(layout.Word(0x100u)))=="updateTradeItem" &&
            std::string(reinterpret_cast<const char*>(layout.Word(0x110u)))=="regUpdateEvent","Trade timer names use live owned host text");
        Check(!count(reinterpret_cast<const void*>(layout.Word(0x28u))) && !rows(reinterpret_cast<const void*>(layout.Word(0x28u))),"actual original list starts with no rows or columns");
        struct Control{unsigned offset;const char* name;};
        constexpr std::array controls{
            Control{0x28u,"List"},Control{0x2cu,"EditDescription"},Control{0x30u,"EditProp"},Control{0x34u,"StaticMoney"},
            Control{0x38u,"StaticNeedMoney"},Control{0x3cu,"BtnOk"},Control{0x40u,"BtnCancel"},Control{0x44u,"BtnLeftArrow"},
            Control{0x48u,"BtnRightArrow"},Control{0x4cu,"BtnHelp"},Control{0x50u,"StaticGoodsType"},Control{0xd4u,"WindowRoleState"},
            Control{0xd8u,"daoju"},Control{0xdcu,"StaticBuy"},Control{0xe0u,"StaticSell"},Control{0xe4u,"ImgGoodsType"}};
        std::set<unsigned> controls_found;
        for(const auto& control:controls){const auto w=layout.Word(control.offset);Check(w && w==find(std::string("TradeWindow/")+control.name),"Trade control matches actual original XML name/offset");controls_found.insert(w);}
        Check(controls_found.size()==controls.size(),"all original Trade controls are distinct");
        constexpr std::array<const char*,8> role_names{"ImageRole","Icon","StaticRoleWu","StaticRoleFang","StaticRoleSu","StaticRoleYun","StaticRoleLing","StaticLevel"};
        for(unsigned role=1;role<=4u;++role)for(unsigned field=0;field<8u;++field)
            Check(layout.Word(0x54u+(role-1u)*0x20u+field*4u)==find(std::string("TradeWindow/")+role_names[field]+std::to_string(role)),"all32 role display windows resolve to original XML");
        constexpr std::array<const char*,15> images{"wuqi","toubu","shenti","zhubu","peidai","xinzeng","zi","zi1","zi2","zi3","zi4","xinzeng","rongzhu","xinzeng","duanye"};
        for(unsigned index=0;index<images.size();++index)Check(layout.Word(0x160u+index*4u)==NativeCeguiWidgetImages(module).Image("zi0",images[index]),"Trade image field resolves original zi0 image");
        constexpr std::array<std::array<unsigned,2>,5> events{{{0x8b0a48u,0x48bea0u},{0x8b0a4cu,0x48c5c0u},{0x8b185cu,0x48cc20u},{0x8b1860u,0x48cc80u},{0x8b1864u,0x48cce0u}}};
        for(const auto& event:events)Check(layout.Event(event[0],{1u,2u}) && methods.back()==event[1],"five frame descriptors dispatch exact original method");
        Check(!layout.Event(0xdeadbeefu,{0u,0u}),"unknown Trade frame event is not consumed");
        layout.BindParent(true);
        const auto click=[&](unsigned w){(void)NativeCeguiWindowPointerEvents(module).HandleMouseAtCenter(w);layout.RethrowEventError();};
        constexpr std::array<std::array<unsigned,2>,5> buttons{{{0x44u,0x48d290u},{0x48u,0x48d310u},{0x3cu,0x48d390u},{0x40u,0x48d680u},{0x4cu,0x48d710u}}};
        for(const auto& button:buttons){const auto before=methods.size();click(layout.Word(button[0]));Check(methods.size()==before+1u && methods.back()==button[1],"real DLL Trade button reaches original method exactly once");}
        click(find("TradeWindow/BtnClose"));Check(methods.back()==0x48d680u,"original close and cancel share handler");
        // Fire real DLL list events explicitly: the empty list has no row to hover.
        std::array<unsigned,8> event_args{};event_args[2]=layout.Word(0x28u);
        for(const auto* symbol:{"?EventHoverRowChanged@MultiColumnList@CEGUI@@2VString@2@B","?EventMouseClick@Window@CEGUI@@2VString@2@B"}){
            detail::NativeString event_namespace(module,"");
            const auto before=methods.size();fire(reinterpret_cast<void*>(layout.Word(0x28u)),detail::Export<const void*>(module,symbol),event_args.data(),event_namespace.Data());
            Check(methods.size()==before+1u,"actual DLL list event subscription dispatches");
        }
        Check(methods[methods.size()-2u]==0x48d1d0u && methods.back()==0x48d1a0u,"Trade list hover/click method identities");
        guarded=true;bool rejected{};try{click(layout.Word(0x4cu));}catch(const std::logic_error&){rejected=true;}guarded=false;
        Check(rejected && layout.RootAddress() && !retired,"Trade DLL callback cannot destroy its owning layout");
        click(layout.Word(0x4cu));Check(methods.back()==0x48d710u,"deferred Trade error drains and callback can recover");
        layout.BindParent(false);layout.Close();layout.Close();Check(!layout.RootAddress() && retired==1u,"Trade layout retires callbacks/windows once before DLL shutdown");
        bool closed{};try{(void)layout.Event(0x8b0a48u,{0u,0u});}catch(const std::logic_error&){closed=true;}Check(closed,"retired Trade frame rejects dispatch");
    }
    std::cout<<"PASS: original TradeWindow layout,48 controls including32 role windows,15 images,5 frame descriptors/8 DLL subscriptions,callback retirement/error recovery/recreation; business/effect services not composed\n";
}
}
