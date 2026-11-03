#include "pal4/role_selector_window.h"
#include "pal4/native_role_selector.h"
#include "support/native_cegui_frame_session.h"
#include "support/native_desktop_observation.h"
#include "pal4/native_cegui_ui_session.h"
#include "pal4/native_cegui_window_pointer_events.h"
#include "pal4/native_game_database.h"
#include "pal4/native_role_record_image.h"
#include "pal4/native_player_slot_copy_runtime.h"
#include "pal4/native_player_hud_stats_data.h"
#include "pal4/native_player_script_property_runtime.h"
#include "pal4/player_manager_slot.h"
#include "pal4/native_event_handler_array.h"
#include "../native/ui/cegui_exports_native.h"
#include <rtfsyst.h>
#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstring>
#include <iostream>
#include <numeric>
#include "support/native_property_use_probe.h"
namespace {
using pal4::detail::Export;
void Check(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
void* Ptr(std::uint32_t a){return reinterpret_cast<void*>(a);}
std::uint32_t Address(const void* p){return reinterpret_cast<std::uint32_t>(p);}
struct Filesystem{bool ready{};~Filesystem(){if(ready)RtFSManagerClose();}};
}
int main(int argc,char** argv)try{
    std::cout<<std::unitbuf;Check(argc==2,"provide original game directory");
    const pal4::test::NativeDesktopObservation desktop;
    pal4::test::NativeFrameSession frame;Check(frame.Start(800,600) && !IsWindowVisible(frame.window),"Role selector hidden SDK frame");
    Filesystem fs;fs.ready=RtFSManagerOpen(8)!=FALSE;Check(fs.ready,"Role selector SDK filesystem");
    pal4::NativeCeguiUiSession ui;Check(ui.Start(argv[1],800,600),"Role selector DLL UI");
    Check(ui.LoadImageset(argv[1],"gamedata\\ui\\fonts\\fontPuzzle_0.imageset") && ui.InitializeWidgetRendering(argv[1]) &&
        ui.LoadImageset(argv[1],"gamedata\\ui\\imagesets\\OiramLook.imageset") && ui.LoadOriginalMainLayout() && ui.HideMainLayout(),"Role selector original font/desktop");
    const auto module=GetModuleHandleA("CEGUIBase.dll");
    pal4::NativeGameDatabase database;Check(database.Load(argv[1]),"Role selector original database");
    pal4::NativeRoleRecordCatalog roles(database);pal4::PlayerManagerObjectState players{};pal4::NativePlayerSlotCopyRuntime copier;
    constexpr std::array<unsigned,4> priorities{2u,0u,3u,1u};
    std::array<unsigned,4> hp{},mp{};
    for(unsigned i=0;i<4u;++i){const auto player=Address(&players.slots[i]);
        (void)pal4::PlayerManagerSlotDataCopier::Copy(player,roles.Address(i),copier);
        pal4::GameObjectPropertyState properties{};std::memcpy(&properties,Ptr(player+0x654u),sizeof(properties));
        properties.words[0]=pal4::PlayerManagerSlot::kScriptVtable;
        const auto* original=reinterpret_cast<const std::uint32_t*>(roles.Address(i));hp[i]=original[1u];mp[i]=original[3u];
        Check(hp[i]>0u && mp[i]>0u,"original role maximum HP/MP");
        (void)pal4::GameObjectPropertyAccess::SetIndexed86(properties,0u,hp[i]);
        (void)pal4::GameObjectPropertyAccess::SetIndexed86(properties,2u,mp[i]);
        (void)pal4::GameObjectPropertyAccess::Set167And83(properties,priorities[i]);
        std::memcpy(Ptr(player+0x654u),&properties,sizeof(properties));
        (void)pal4::NativePlayerScriptPropertyRuntime::Set(player+0x654u,68u,7u,hp[i]/2u);
        (void)pal4::NativePlayerScriptPropertyRuntime::Set(player+0x654u,68u,9u,mp[i]/3u);
        (void)pal4::NativePlayerScriptPropertyRuntime::Set(player+0x654u,68u,8u,20u+i);
        Check(players.Word((i*0xb14u+0xa48u)/4u)==i,"original Role copier retains canonical identity");
    }
    pal4::NativeEventHandlerArray activity([](auto,auto){return 1u;},[](auto,auto,auto)->std::uint8_t{return 0u;});
    (void)activity.Insert(0xabcdu);std::array<std::uint32_t,90> manager{};
    pal4::NativeRoleSelector selector(ui,argv[1],players,activity,Address(manager.data()));
    const auto owner=selector.FrameAddress();
    const auto word=[](std::uint32_t a){return *reinterpret_cast<const std::uint32_t*>(a);};
    Check(word(owner)==pal4::RoleSelectorWindow::kVtable && word(owner+40u)==pal4::RoleSelectorWindow::kNavigationVtable,"Role selector typed original layout");
    const auto render=[&]{Check(frame.Begin() && ui.Render(),"Role selector actual DLL render");return frame.ReadPixels();};
    const auto blank=render();
    auto* system=Ptr(ui.SystemAddress());
    const auto key=[&](unsigned code){(void)render();(void)Export<bool (__thiscall*)(void*,unsigned)>(module,"?injectKeyDown@System@CEGUI@@QAE_NI@Z")(system,code);
        (void)Export<bool (__thiscall*)(void*,unsigned)>(module,"?injectKeyUp@System@CEGUI@@QAE_NI@Z")(system,code);selector.RethrowEventError();};
    const auto click=[&](std::uint32_t window){(void)render();const auto origin=pal4::NativeCeguiWindowPointerEvents(module).AbsolutePosition(window);
        const auto width=*reinterpret_cast<const float*>(window+268u),height=*reinterpret_cast<const float*>(window+272u);
        auto* sheet=Export<void* (__thiscall*)(const void*)>(module,"?getGUISheet@System@CEGUI@@QBEPAVWindow@2@XZ")(system);
        pal4::CeguiPoint point{};bool found{};for(unsigned y=1;y<16u && !found;++y)for(unsigned x=1;x<16u && !found;++x){
            point={origin.x+width*x/16.0f,origin.y+height*y/16.0f};found=Export<void* (__thiscall*)(const void*,const pal4::CeguiPoint*)>(module,
                "?getChildAtPosition@Window@CEGUI@@QBEPAV12@ABVVector2@2@@Z")(sheet,&point)==Ptr(window);}
        Check(found,"Role selector real DLL hit-test point");
        (void)Export<bool (__thiscall*)(void*,float,float)>(module,"?injectMousePosition@System@CEGUI@@QAE_NMM@Z")(system,point.x,point.y);
        (void)Export<bool (__thiscall*)(void*,int)>(module,"?injectMouseButtonDown@System@CEGUI@@QAE_NW4MouseButton@2@@Z")(system,0);
        (void)Export<bool (__thiscall*)(void*,int)>(module,"?injectMouseButtonUp@System@CEGUI@@QAE_NW4MouseButton@2@@Z")(system,0);selector.RethrowEventError();};
    const auto text=[&](std::uint32_t window){const auto value=Export<const void* (__thiscall*)(const void*)>(module,"?getText@Window@CEGUI@@QBEABVString@2@XZ")(Ptr(window));
        return std::string(Export<const char* (__thiscall*)(const void*)>(module,"?c_str@String@CEGUI@@QBEPBDXZ")(value));};
    const auto modal=[&]{return Export<bool (__thiscall*)(const void*)>(module,"?getModalState@Window@CEGUI@@QBE_NXZ")(Ptr(word(owner+36u)));};
    std::vector<std::pair<unsigned,unsigned>> calls;
    const auto choice=[&](unsigned role,unsigned context){calls.emplace_back(role,context);};
    for(unsigned count=0;count<=4u;++count){
        for(unsigned i=0;i<4u;++i)players.Word((i*0xb14u+0xb08u)/4u)=i>=4u-count?1u:0u;
        selector.Show(0x1234u,choice);Check(selector.Visible() && modal() && selector.VisibleCardCount()==count,"Role selector active party count and DLL modality");
        std::vector<unsigned> expected;for(unsigned i=4u-count;i<4u;++i)expected.push_back(i);
        std::sort(expected.begin(),expected.end(),[&](auto a,auto b){return priorities[a]<priorities[b];});
        for(unsigned card=0;card<count;++card){const auto role=expected[card],child=owner+72u+card*52u;
            Check(selector.CardRole(card)==role && word(selector.CardButton(card)+280u)==card,"sorted card identity differs from fixed button slot");
            Check(text(word(child+4u))==std::to_string(hp[role]/2u)+"/"+std::to_string(hp[role]) &&
                text(word(child+8u))==std::to_string(mp[role]/3u)+"/"+std::to_string(mp[role]) && text(word(child+12u))==std::to_string(20u+role)+"/100","original role card HP/MP/rage text");
            const auto bar=word(child+16u);const auto width=Export<float (__thiscall*)(const void*)>(module,"?getWidth@Window@CEGUI@@QBEMXZ")(Ptr(bar));
            const auto initial=std::bit_cast<float>(word(child+36u));
            Check(std::fabs(width-initial*static_cast<float>(hp[role]/2u)/static_cast<float>(hp[role]))<0.01f,"actual DLL HP bar follows live player ratio");
            const auto slot=(count<=2u?1u:0u)+card;
            for(const auto [window,expected_position]:{std::pair{word(child),owner+280u+slot*8u},std::pair{selector.CardButton(card),owner+312u+slot*8u}}){
                std::array<float,2> actual{};Export<void* (__thiscall*)(const void*,void*)>(module,"?getPosition@Window@CEGUI@@QBE?AVVector2@2@XZ")(Ptr(window),actual.data());
                Check(std::fabs(actual[0]-std::bit_cast<float>(word(expected_position)))<0.01f &&
                    std::fabs(actual[1]-std::bit_cast<float>(word(expected_position+4u)))<0.01f,"role cards use original template slots for current party size");}
        }
        if(count){const auto pixels=render();Check(std::inner_product(pixels.begin(),pixels.end(),blank.begin(),std::size_t{},std::plus<>(),[](auto a,auto b){return a!=b?1u:0u;})>100u,"original role selector renders GPU pixels");
            const auto before=calls.size();key(205u);key(28u);Check(calls.size()==before+1u && calls.back()==std::pair{expected[count>1u?1u:0u],0x1234u} && selector.Visible(),"DLL keyboard selects canonical role and preserves selector for another item");
            click(selector.CardButton(count-1u));Check(calls.back()==std::pair{expected.back(),0x1234u},"actual DLL role click preserves context");}
        const auto before=calls.size();key(1u);Check(!selector.Visible() && !modal() && calls.size()==before,"Role selector Esc closes without choice");
        Check(activity.Size()==1u && activity.GetLast()==0xabcdu,"Role selector preserves underlying activity");
    }
    selector.Show(7u,[&](auto role,auto context){choice(role,context);selector.Hide();});click(selector.CardButton(0u));
    Check(!selector.Visible() && calls.back().second==7u,"role choice may hide selector during DLL dispatch");
    selector.Show(8u,choice);selector.Show(9u,choice);Check(modal(),"replacing selector request preserves original DLL modality");
    click(word(owner+348u));Check(!selector.Visible(),"Role selector close button");
    std::weak_ptr<int> copied_lifetime;
    {auto token=std::make_shared<int>(17);copied_lifetime=token;selector.Show(11u,[token](auto,auto){});}
    Check(!copied_lifetime.expired(),"selector owns a copied callback beyond caller scope");selector.Hide();Check(copied_lifetime.expired(),"selector Hide releases copied callback");
    selector.Show(12u,[&](auto,auto){selector.Show(13u,choice);});click(selector.CardButton(0u));Check(modal() && selector.Visible(),"role choice can replace its selector callback during dispatch");
    Check(word(owner+68u)==13u,"reentrant selector retains replacement context field");const auto replacement_before=calls.size();
    click(selector.CardButton(0u));std::cout<<"Role replacement calls="<<replacement_before<<"/"<<calls.size()<<" context="<<word(owner+68u)<<" last="<<calls.back().second<<std::endl;
    // System may classify this immediate same-button press as a multi-click,
    // which the original button does not turn into a second Clicked event.
    Check(calls.size()==replacement_before || calls.back().second==13u,"delivered replacement mouse choice retains new context");
    const auto before_enter=calls.size();key(28u);Check(calls.size()==before_enter+1u && calls.back().second==13u,"reentrant selector dispatch uses replacement context");selector.Hide();
    selector.Show(10u,[](auto,auto){throw std::runtime_error("role choice failure");});bool caught{};
    try{key(28u);}catch(const std::runtime_error&){caught=true;}Check(caught,"Role selector callback errors remain inside DLL boundary until rethrow");
    pal4::test::VerifyNativePropertyUse(ui,argv[1],database,players,selector,key);
    selector.Close();selector.Close();Check(!selector.Visible() && !word(owner+36u) && !word(owner+48u) && !word(owner+64u),"Role selector Close releases layout/navigation/copied callback");
    (void)activity.Remove(0xabcdu);activity.ReleaseStorage();desktop.Check("Role selector native modal/layout/input");
    std::cout<<"PASS: original DLL RoleSelector 0..4 party, canonical identities, stat text/bars/GPU, keyboard/mouse, callback lifetime and cleanup\n";return 0;
}catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}
