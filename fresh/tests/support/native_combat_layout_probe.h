#pragma once
#include "native_combat_selector_layout_probe.h"
#include "native_combat_action_window_probe.h"
#include "pal4/native_cegui_ui_session.h"
#include "pal4/native_ui_world_camera.h"
#include "pal4/native_combat_main_window.h"
#include "pal4/native_combat_sequence.h"
#include "native_combat_main_frame_probe.h"
#include "pal4/cegui_window_pointer_events.h"
#include "../../native/ui/cegui_exports_native.h"
#include <rwcore.h>
#include <array>
#include <iostream>
#include <stdexcept>
#include <cmath>
#include <limits>

namespace pal4::test {
inline unsigned VerifyNativeCombatLayouts(NativeCeguiUiSession& ui,const std::filesystem::path& root,
                                     unsigned width,unsigned height){
    const auto check=[](bool ok,const char* reason){if(!ok)throw std::runtime_error(reason);};
    const auto initial_renders=ui.RenderCalls();
    if(!ui.LoadOriginalCombatLayouts(root))throw std::runtime_error(ui.LastError());
    using K=NativeCombatLayout;
    constexpr std::array kinds{K::Main,K::Action,K::Role,K::Ending};
    std::array<std::uint32_t,4> roots{};
    for(std::size_t i=0;i<roots.size();++i){roots[i]=ui.CombatLayoutRoot(kinds[i]);check(roots[i]!=0u,"original Combat root missing");}
    check(ui.LoadOriginalCombatLayouts(root),"Combat layout reload is idempotent");
    const auto module=GetModuleHandleW(L"CEGUIBase.dll");
    const auto visible=reinterpret_cast<bool (__thiscall*)(const void*,bool)>(GetProcAddress(module,"?isVisible@Window@CEGUI@@QBE_N_N@Z"));
    const auto parent=reinterpret_cast<void* (__thiscall*)(const void*)>(GetProcAddress(module,"?getParent@Window@CEGUI@@QBEPAV12@XZ"));
    check(visible && parent,"Combat actual window visibility/parent exports");
    for(std::size_t i=0;i<roots.size();++i)
        check(ui.CombatLayoutRoot(kinds[i])==roots[i] && !visible(reinterpret_cast<void*>(roots[i]),true),"Combat preparation remains hidden without duplicate roots");
    for(const auto* name:{"StaticActionSequence","ImgJointAttackHint","guaishuliang","PanelMonsterInfo","EditMonsterInfo","StaticRole1","StaticRole2","StaticRole3",
                         "StaticMonster1","StaticMonster2","StaticMonster3","StaticMonster4","StaticMonster5","StaticMonster6"})
        check(ui.CombatLayoutControl(K::Main,name)!=0u,"original Combat main control");
    for(const auto* name:{"BtnArticle","BtnAttack","BtnFlee","BtnDefend","BtnMagic","BtnStunt"})
        check(ui.CombatLayoutControl(K::Action,name)!=0u,"original six Combat action controls");
    check(ui.CombatLayoutControl(K::Role,"ImageHp1") && ui.CombatLayoutControl(K::Role,"ImageHp3") &&
          ui.CombatLayoutControl(K::Ending,"PanelScoreMark") && ui.CombatLayoutControl(K::Ending,"PanelMoney"),"original role and ending controls");
    check(!ui.SetCombatLayoutVisible(K::Main,true,roots[1]) && !ui.SetCombatLayoutVisible(K::Action,true,roots[1]) &&
          !ui.SetCombatLayoutVisible(K::Role,true,1u),"Combat invalid parent rejects without pointer use");
    struct Hide {NativeCeguiUiSession& ui;~Hide(){for(auto kind:{K::Action,K::Role,K::Main,K::Ending})(void)ui.SetCombatLayoutVisible(kind,false);}} hide{ui};
    check(ui.SetCombatLayoutVisible(K::Main,true) && ui.SetCombatLayoutVisible(K::Action,true,roots[0]) &&
          ui.SetCombatLayoutVisible(K::Role,true,roots[0]),"actual Main parent applied to Action/Role");
    check(parent(reinterpret_cast<void*>(roots[1]))==reinterpret_cast<void*>(roots[0]) &&
          parent(reinterpret_cast<void*>(roots[2]))==reinterpret_cast<void*>(roots[0]),"actual CEGUI parent relationships");
    check(ui.SetCombatLayoutVisible(K::Main,false) && !visible(reinterpret_cast<void*>(roots[1]),false) &&
          visible(reinterpret_cast<void*>(roots[1]),true),"parent hide suppresses child without changing its local state");
    NativeUiWorldCamera camera;check(camera.Open(nullptr,width,height),"Combat UI offscreen camera");
    for(auto phase:{K::Main,K::Ending}){
        check(ui.SetCombatLayoutVisible(phase,true),"Combat layout render visibility");
        RwRGBA black{0u,0u,0u,255u};
        check(RwCameraClear(camera.Camera(),&black,rwCAMERACLEARIMAGE|rwCAMERACLEARZ) && RwCameraBeginUpdate(camera.Camera()),"Combat UI camera begin");
        struct End {RwCamera* p;~End(){if(p)RwCameraEndUpdate(p);}} end{camera.Camera()};
        const auto quads=ui.QuadCalls();
        if(!ui.Render())throw std::runtime_error(ui.LastError());
        check(ui.QuadCalls()>quads,"original Combat widgets emit actual CEGUI render quads");
        check(RwCameraEndUpdate(camera.Camera())!=nullptr,"Combat UI camera end");end.p=nullptr;
        check(ui.SetCombatLayoutVisible(phase,false),"Combat UI hidden after render probe");
    }
    {
        NativeCombatMainWindow window(ui,root);
        const auto reject=[&](auto&& f){bool rejected{};try{f();}catch(const std::logic_error&){rejected=true;}check(rejected,"Combat main owner rejection");};
        reject([&]{NativeCombatMainWindow duplicate(ui,root);});
        check(window.Address() && window.Root()==roots[0] && window.StatusMarkupCount()==29u &&
              window.ResetIconTimer()=="ResetCombatIcon" && window.StatusMarkup(0u).empty() &&
              window.StatusMarkup(1u)=="<image set=kuang0 name=ding/>" &&
              window.StatusMarkup(28u)=="<image set=kuang0 name=ling2/>","shared full constructor initializes actual owner and modern status strings");
        const auto get_position=reinterpret_cast<const void* (__thiscall*)(const void*,void*,std::uint32_t)>(GetProcAddress(module,"?getPosition@Window@CEGUI@@QBE?AVVector2@2@W4MetricsMode@2@@Z"));
        const auto get_width=reinterpret_cast<float (__thiscall*)(const void*,std::uint32_t)>(GetProcAddress(module,"?getWidth@Window@CEGUI@@QBEMW4MetricsMode@2@@Z"));
        check(get_position && get_width,"Combat metrics exports");
        const auto sequence=ui.CombatLayoutControl(K::Main,"StaticActionSequence");
        std::array<float,2> position{};get_position(reinterpret_cast<void*>(sequence),position.data(),1u);
        const auto bounds=window.SequenceBounds();
        check(std::fabs(bounds[0]-(position[0]-10.0f))<0.01f && std::fabs(bounds[1]-(position[1]-15.0f))<0.01f &&
              std::fabs(bounds[2]-(position[0]+get_width(reinterpret_cast<void*>(sequence),1u)-55.0f))<0.01f && bounds[3]==bounds[1],"shared constructor bounds use actual DLL pixel metrics");
        for(const auto* prefix:{"StaticRole","StaticMonster"})for(unsigned i=1u;i<=(std::string_view(prefix)=="StaticRole"?3u:6u);++i){
            const auto icon=ui.CombatLayoutControl(K::Main,std::string(prefix)+std::to_string(i));
            check(!visible(reinterpret_cast<void*>(icon),true) && !*reinterpret_cast<const std::uint32_t*>(icon+280u),"shared initializer hides and clears actual nine icons");
        }
        const auto autobreak=reinterpret_cast<bool (__thiscall*)(const void*)>(GetProcAddress(GetModuleHandleW(L"OIRAMLOOK.dll"),"?getAutoBreak@OLFormatEditbox@CEGUI@@QBE_NXZ"));
        check(autobreak && !autobreak(reinterpret_cast<void*>(ui.CombatLayoutControl(K::Main,"EditMonsterInfo"))),"actual OL edit autobreak disabled");
        {
            NativeCombatSequence sequence;
            sequence.players={{1u,0u,"101",0.f},{2u,2u,"103",64.f}};
            sequence.monsters={{3u,1u,"101",32.f},{4u,4u,"",0.f,true}};
            sequence.all_monsters={{3u,0u,"",0.f,false,true},{4u},{5u,0u,"",0.f,false,false,true}};
            window.RefreshSequence(sequence,2u);
            const auto role1=ui.CombatLayoutControl(K::Main,"StaticRole1"),role3=ui.CombatLayoutControl(K::Main,"StaticRole3");
            const auto monster2=ui.CombatLayoutControl(K::Main,"StaticMonster2"),monster5=ui.CombatLayoutControl(K::Main,"StaticMonster5");
            const auto bounds=window.SequenceBounds();
            const auto expect=[&](std::uint32_t w,float x){std::array<float,2> p{};get_position(reinterpret_cast<void*>(w),p.data(),1u);
                // The original DLL pixel-aligns the absolute setter (581.5->582).
                x=static_cast<float>(static_cast<int>(x+0.5f));
                if(std::abs(p[0]-x)>=0.01f || std::abs(p[1]-bounds[1])>=0.01f)
                    throw std::runtime_error("sequence pixel mismatch actual="+std::to_string(p[0])+","+std::to_string(p[1])+" expected="+std::to_string(x)+","+std::to_string(bounds[1]));};
            expect(role1,bounds[0]);expect(role3,bounds[2]);expect(monster2,(bounds[0]+bounds[2])*0.5f);
            check(!visible(reinterpret_cast<void*>(monster5),true) && visible(reinterpret_cast<void*>(role1),true) &&
                  *reinterpret_cast<const std::uint32_t*>(role1+280u)==1u && !*reinterpret_cast<const std::uint32_t*>(monster5+280u),"actual sequence hides state3 and sets icon flags");
            const auto get_image=detail::Export<const void* (__thiscall*)(const void*)>(module,"?getImage@StaticImage@CEGUI@@QBEPBVImage@2@XZ");
            check(get_image(reinterpret_cast<void*>(role1))!=nullptr,"sequence image resolves original atlas");
            const auto get_text=detail::Export<const void* (__thiscall*)(const void*)>(module,"?getText@Window@CEGUI@@QBEABVString@2@XZ");
            const auto cstr=detail::Export<const char* (__thiscall*)(const void*)>(module,"?c_str@String@CEGUI@@QBEPBDXZ");
            check(std::string_view(cstr(get_text(reinterpret_cast<void*>(ui.CombatLayoutControl(K::Main,"guaishuliang")))))=="2","count includes state0 or ready from full roster");
            check(ui.SetCombatLayoutVisible(K::Main,true),"populated sequence render visible");
            RwRGBA black{0u,0u,0u,255u};check(RwCameraClear(camera.Camera(),&black,rwCAMERACLEARIMAGE|rwCAMERACLEARZ) && RwCameraBeginUpdate(camera.Camera()),"populated sequence camera begin");
            struct EndSequence {RwCamera* camera;~EndSequence(){if(camera)RwCameraEndUpdate(camera);}} end_sequence{camera.Camera()};
            const auto quads=ui.QuadCalls();if(!ui.Render())throw std::runtime_error(ui.LastError());
            check(ui.QuadCalls()>quads,"populated original sequence generates actual render quads");
            check(RwCameraEndUpdate(camera.Camera())!=nullptr,"populated sequence camera end");end_sequence.camera=nullptr;
            check(ui.SetCombatLayoutVisible(K::Main,false),"populated sequence root hidden after probe");
            window.RefreshSequence(sequence,1u);check(!visible(reinterpret_cast<void*>(role1),true) && visible(reinterpret_cast<void*>(monster2),true),"original state1 suppresses Player icons");
            window.RefreshSequence(sequence,0u);check(visible(reinterpret_cast<void*>(role1),true) && !visible(reinterpret_cast<void*>(monster2),true),"original state0 suppresses Monster icons");
            sequence.players[0].slot=3u;reject([&]{window.RefreshSequence(sequence,2u);});sequence.players[0].slot=0u;
            sequence.players.push_back(sequence.players[0]);window.RefreshSequence(sequence,2u);
            sequence.players[0].progress=std::numeric_limits<float>::quiet_NaN();reject([&]{window.RefreshSequence(sequence,2u);});
            window.RefreshSequence({},2u);check(!visible(reinterpret_cast<void*>(role1),true),"empty next roster clears stale sequence visibility");
            std::cout<<"Native combat sequence UI: real atlas, pixel interpolation, state gates, death hide, full roster count and invalid input guards pass; synthetic actor queries\n";
        }
        unsigned clicks{};
        const auto fire=detail::Export<void (__thiscall*)(void*,const void*,void*,const void*)>(module,
            "?fireEvent@EventSet@CEGUI@@UAEXABVString@2@AAVEventArgs@2@0@Z");
        const auto click_event=detail::Export<const void*>(module,"?EventMouseClick@Window@CEGUI@@2VString@2@B");
        detail::NativeString event_namespace(module,"");
        const auto click=[&]{
            CeguiMouseEvent event{};event.vtable=reinterpret_cast<std::uint32_t>(detail::Export<const void*>(module,"??_7MouseEventArgs@CEGUI@@6B@"));event.window=roots[0];
            fire(reinterpret_cast<void*>(roots[0]),click_event,&event,event_namespace.Data());
        };
        window.BindClickHandler([&](std::uint32_t event){check(event!=0u,"actual Combat click payload");++clicks;reject([&]{window.Close();});reject([&]{window.BindClickHandler({});});return true;});
        check(ui.SetCombatLayoutVisible(K::Main,true),"Combat root visible for native click dispatch");
        click();window.RethrowClickError();check(clicks==1u,"actual DLL click reaches native owner once");
        window.BindClickHandler([](std::uint32_t)->bool{throw std::logic_error("intentional Combat root click failure");});
        click();reject([&]{window.RethrowClickError();});window.RethrowClickError();
        window.Close();check(!window.Address() && !window.Root() && !window.StatusMarkupCount(),"Combat native owner retires modern storage");
        click();check(clicks==1u,"closed Combat root connection is removed");
        NativeCombatMainWindow replacement(ui,root);replacement.BindClickHandler([&](std::uint32_t){++clicks;return true;});
        click();replacement.RethrowClickError();check(clicks==2u,"replacement owner has one actual DLL callback");replacement.Close();
        check(ui.SetCombatLayoutVisible(K::Main,false),"Combat main root hidden after owner probe");
        std::cout<<"Native Combat main window: shared full construction, actual9 icon flags/bounds/edit,29 modern status strings, real DLL clicks, callback guards/errors, disconnect and owner reuse pass\n";
    }
    VerifyNativeCombatMainFrame(ui,root);
    VerifyNativeCombatSelectorLayouts(ui,root);
    VerifyNativeCombatActionWindow(ui,root);
    std::cout<<"Native Combat UI layouts: original4 roots,14 main controls,6 action buttons, role/ending controls, idempotence, hidden state, parent guards and2 offscreen renders pass; resource preparation only\n";
    check(ui.RenderCalls()==initial_renders+3u,"Combat layout and sequence probes emit exactly three UI frames");
    return ui.RenderCalls()-initial_renders;
}
}
