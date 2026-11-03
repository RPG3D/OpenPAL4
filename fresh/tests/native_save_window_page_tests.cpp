#include "pal4/native_save_window_page.h"
#include "pal4/native_save_window.h"
#include "pal4/native_event_handler_array.h"
#include "pal4/native_common_dialog_layout.h"
#include "support/native_cegui_test_click.h"
#include "support/native_cegui_frame_session.h"
#include "support/native_desktop_observation.h"
#include "pal4/native_cegui_ui_session.h"
#include "pal4/native_cegui_window_pointer_events.h"
#include "pal4/native_game_database.h"
#include "pal4/native_game_save_summary.h"
#include "pal4/dialog_manager_lifecycle.h"
#include "../native/ui/cegui_exports_native.h"
#include <rtfsyst.h>
#include <bit>
#include <iostream>
#include <stdexcept>
namespace {
void Check(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
void Run(const std::filesystem::path& root){
    using pal4::detail::Export;pal4::test::NativeFrameSession frame;Check(frame.Start(800,600) && !IsWindowVisible(frame.window),"SaveWindow hidden SDK frame");
    struct Filesystem{bool ready=RtFSManagerOpen(8)!=FALSE;~Filesystem(){if(ready)RtFSManagerClose();}} fs;Check(fs.ready,"SaveWindow filesystem");
    pal4::NativeCeguiUiSession ui;Check(ui.Start(root,800,600),"SaveWindow original UI");
    Check(ui.LoadImageset(root,"gamedata\\ui\\fonts\\fontPuzzle_0.imageset") && ui.InitializeWidgetRendering(root),"SaveWindow fonts/widgets");
    Check(ui.LoadImageset(root,"gamedata\\ui\\imagesets\\OiramLook.imageset") && ui.LoadOriginalMainLayout() && ui.HideMainLayout() && ui.ShowOriginalSaveLayout(root),"SaveWindow original layout");
    pal4::NativeGameDatabase database;Check(database.Load(root),"SaveWindow original database");
    const auto module=GetModuleHandleA("CEGUIBase.dll");
    const auto find=[&](std::string name){const auto wm=Export<void* (__cdecl*)()>(module,"?getSingletonPtr@WindowManager@CEGUI@@SAPAV12@XZ")();
        pal4::detail::NativeString text(module,"saveWindow/"+name);return reinterpret_cast<std::uint32_t>(Export<void* (__thiscall*)(const void*,const void*)>(module,
            "?getWindow@WindowManager@CEGUI@@QBEPAVWindow@2@ABVString@2@@Z")(wm,text.Data()));};
    pal4::SaveWindowState state;state.Write(36u,ui.SaveFrameAddress());
    for(unsigned i=0;i<10u;++i){const auto w=find("BtnLoad"+std::to_string(i));state.Write(0x2cu+i*4u,w);
        Export<void (__thiscall*)(void*,void*)>(module,"?setUserData@Window@CEGUI@@QAEXPAX@Z")(reinterpret_cast<void*>(w),reinterpret_cast<void*>(i));}
    state.Write(0x54u,find("List"));state.Write(0x5cu,find("ImgSelectMask"));state.Write(0x60u,find("StaticMask"));
    const auto list=reinterpret_cast<void*>(state.Read(0x54u)),mask=reinterpret_cast<void*>(state.Read(0x5cu));
    pal4::DialogManagerState globals;globals.SetWord(44u,74u);globals.SetWord(48u,std::bit_cast<std::uint32_t>(23.5f));
    pal4::NativeSaveWindowPage page(ui,state,database,root,globals);pal4::NativeGameSaveSummaryReader reader;
    Check(frame.Begin() && ui.Render(),"SaveWindow empty list baseline");const auto baseline=frame.ReadPixels();
    Check(!baseline.empty(),"SaveWindow baseline GPU readback");
    const auto origin=pal4::NativeCeguiWindowPointerEvents(module).AbsolutePosition(state.Read(0x54u));
    const auto width=*reinterpret_cast<const float*>(state.Read(0x54u)+268u),height=*reinterpret_cast<const float*>(state.Read(0x54u)+272u);
    unsigned loaded{},pixels{};
    for(unsigned repeat=0;repeat<2u;++repeat)for(unsigned p=0;p<10u;++p){page.Populate(static_cast<int>(p),repeat!=0u);
        Check(Export<unsigned (__thiscall*)(const void*)>(module,"?getRowCount@MultiColumnList@CEGUI@@QBEIXZ")(list)==10u,"actual original SaveWindow has ten rows");
        Check(Export<void* (__thiscall*)(const void*)>(module,"?getParent@Window@CEGUI@@QBEPAV12@XZ")(mask)==reinterpret_cast<void*>(state.Read(0x2cu+p*4u)) &&
            reinterpret_cast<std::uint32_t>(Export<void* (__thiscall*)(const void*)>(module,"?getUserData@Window@CEGUI@@QBEPAXXZ")(mask))==p,"SaveWindow mask parent and original+118 page tag");
        for(unsigned row=0;row<10u;++row){const std::array<unsigned,2> grid{row,0u};auto* item=Export<void* (__thiscall*)(const void*,const void*)>(module,
                "?getItemAtGridReference@MultiColumnList@CEGUI@@QBEPAVListboxItem@2@ABUMCLGridRef@2@@Z")(list,grid.data());
            Check(item!=nullptr,"SaveWindow native numbered row exists");const auto address=reinterpret_cast<std::uint32_t>(item);
            const auto slot=*reinterpret_cast<const unsigned*>(address+440u);Check(slot==p*10u+row,"SaveWindow actual row retains absolute slot");
            const auto summary=reader.ReadSlot(root,static_cast<std::int32_t>(slot));loaded+=summary.has_value()?1u:0u;
            Check(*reinterpret_cast<const unsigned*>(address+492u)==(summary?std::bit_cast<std::array<unsigned,13>>(summary->header)[12]:0u),"original occupied/empty row metadata");}
        Check(frame.Begin() && ui.Render(),"SaveWindow actual numbered rows render");const auto image=frame.ReadPixels();Check(image.size()==baseline.size(),"SaveWindow GPU readback");
        unsigned changed{};for(int y=std::max(0,static_cast<int>(origin.y)+12);y<std::min(600,static_cast<int>(origin.y+height)-12);++y)
            for(int x=std::max(0,static_cast<int>(origin.x)+12);x<std::min(800,static_cast<int>(origin.x+width)-12);++x){
                const auto i=static_cast<std::size_t>(y*800+x)*4u;
                changed+=(image[i]!=baseline[i] || image[i+1]!=baseline[i+1] || image[i+2]!=baseline[i+2])?1u:0u;}
        pixels=std::max(pixels,changed);
    }
    Check(loaded && pixels>100u,"SaveWindow original occupied saves and GPU pixels");
    const auto hover=Export<void* (__thiscall*)(const void*)>(module,"?getHoverItem@MultiColumnList@CEGUI@@QBEPAVListboxItem@2@XZ")(list);
    Check(hover && *reinterpret_cast<const unsigned*>(reinterpret_cast<std::uint32_t>(hover)+440u)==94u,"reset restores original global slot remainder");
    globals.SetWord(44u,0xffffffffu);page.Populate(3,true);
    const auto first=Export<void* (__thiscall*)(const void*)>(module,"?getHoverItem@MultiColumnList@CEGUI@@QBEPAVListboxItem@2@XZ")(list);
    Check(first && *reinterpret_cast<const unsigned*>(reinterpret_cast<std::uint32_t>(first)+440u)==30u,"negative global remainder selects first row");
    bool rejected{};try{page.Populate(10,false);}catch(const std::out_of_range&){rejected=true;}Check(rejected,"invalid save page rejects before mutation");
    page.Close();page.Close();Check(ui.SaveRowCount()==0u,"SaveWindow rows retire before native wrappers");
    rejected=false;try{page.Populate(0,false);}catch(const std::logic_error&){rejected=true;}Check(rejected,"closed save-page callbacks reject");
    Check(ui.HideSaveLayout(),"SaveWindow hide after row owner retirement");
    globals.SetWord(44u,74u);globals.SetWord(48u,0u);
    pal4::NativeSaveWindow* controller{};
    pal4::NativeEventHandlerArray active([&](auto member,auto slot){if(member==0xabcdu)return 1u;
        Check(controller && member==controller->InputAddress(),"SaveWindow active callback owner");return controller->Callback(slot);},
        [&](auto member,auto slot,auto event)->std::uint8_t{if(member==0xabcdu)return 0u;
            Check(controller && member==controller->InputAddress(),"SaveWindow active key owner");return controller->Predicate(slot,event);});
    (void)active.Insert(0xabcdu);pal4::NativeCommonDialogLayout confirmation(module,ui,root);
    unsigned state_id=2u,captures{},returns{},saves{},settings{};std::int32_t expected_slot{};std::vector<std::string> order;
    pal4::NativeSaveWindowServices services;
    services.current_state=[&]{return std::pair{0x1234u,state_id};};services.capture_raster=[&]{++captures;};
    services.event=[&](auto key,auto a,auto b){Check(key==0x8afdfcu && !a && !b,"SaveWindow actual property return event");++returns;};
    services.sound=[](auto,auto){};services.save_settings=[&]{++settings;order.emplace_back("settings");};
    services.save_slot=[&](auto slot){std::cout<<"SaveWindow confirmation slot="<<slot<<" global="<<globals.Word(44u)<<std::endl;
        Check(slot==expected_slot && globals.Word(44u)==static_cast<unsigned>(expected_slot),"SaveWindow backend receives current slot after globals");++saves;order.emplace_back("slot");};
    std::array<std::uint32_t,0x224u/4u> manager{};
    pal4::NativeSaveWindow owned(ui,root,root,database,globals,active,confirmation,reinterpret_cast<std::uint32_t>(manager.data()),std::move(services));controller=&owned;
    const auto modal=[&]{return Export<bool (__thiscall*)(const void*)>(module,"?getModalState@Window@CEGUI@@QBE_NXZ")(reinterpret_cast<void*>(confirmation.RootAddress()));};
    const auto key=[&](unsigned code){std::array<unsigned,5> event{};event[4]=code;
        const auto result=active.DispatchKeyDown(active.GetLast(),reinterpret_cast<unsigned>(event.data()));owned.RethrowError();return result;};
    const auto draw=[&]{Check(frame.Begin() && ui.Render(),"native SaveWindow controller frame");(void)frame.ReadPixels();};
    owned.SetVisible(true);Check(owned.Visible() && active.GetLast()==owned.InputAddress() && !captures && !modal(),"System SaveWindow opens without capture or hover confirmation");
    Check(key(205u) && reinterpret_cast<unsigned>(Export<void* (__thiscall*)(const void*)>(module,"?getUserData@Window@CEGUI@@QBEPAXXZ")(reinterpret_cast<void*>(owned.State().Read(0x5cu))))==8u,
        "native SaveWindow global right key changes original page tag");
    Check(key(203u) && key(208u) && !modal(),"page/hover keyboard movement never opens save confirmation");
    // Reset page7 to global row4, then move to slot75 before confirmation.
    owned.SetVisible(false);owned.SetVisible(true);Check(key(208u) && key(28u) && modal(),"Enter opens actual modal save confirmation");
    const auto current_hover=Export<void* (__thiscall*)(const void*)>(module,"?getHoverItem@MultiColumnList@CEGUI@@QBEPAVListboxItem@2@XZ")(reinterpret_cast<void*>(owned.State().Read(84u)));
    std::cout<<"SaveWindow selected slot before No="<<(current_hover?*reinterpret_cast<const unsigned*>(reinterpret_cast<unsigned>(current_hover)+440u):999u)<<std::endl;draw();
    pal4::test::ClickNativeCeguiWindow(ui,confirmation.Window("BtNo"));owned.RethrowError();Check(!modal() && !saves && !settings && globals.Word(44u)==74u,"cancel leaves backend and globals untouched");
    const auto after_cancel_hover=Export<void* (__thiscall*)(const void*)>(module,"?getHoverItem@MultiColumnList@CEGUI@@QBEPAVListboxItem@2@XZ")(reinterpret_cast<void*>(owned.State().Read(84u)));
    Check(after_cancel_hover!=nullptr,"SaveWindow current hover after DLL modal return");
    expected_slot=static_cast<std::int32_t>(*reinterpret_cast<const unsigned*>(reinterpret_cast<unsigned>(after_cancel_hover)+440u));
    Check(key(28u) && modal(),"save confirmation reopens");draw();pal4::test::ClickNativeCeguiWindow(ui,confirmation.Window("BtYes"));owned.RethrowError();
    Check(saves==1u && settings==1u && order==std::vector<std::string>{"settings","slot"} && !modal(),"actual Yes saves settings then slot and refreshes native rows");
    Check(key(1u) && !owned.Visible() && active.GetLast()==0xabcdu && returns==2u,"Escape detaches SaveWindow and notifies property only in System");
    state_id=1u;owned.SetVisible(true);Check(captures==1u,"outside System opening captures actual scene boundary");owned.SetVisible(false);Check(returns==2u,"outside System hide sends no property return");
    state_id=2u;owned.SetVisible(true);Check(key(28u) && modal(),"pending prompt before owner retirement");
    confirmation.Show(U"Foreign replacement",4u,{},[](auto,auto){});owned.Close();owned.Close();Check(modal(),"SaveWindow Close preserves a foreign replacement prompt");
    Check(active.GetLast()==0xabcdu && !owned.Visible() && !owned.State().Read(36u),"native SaveWindow releases owned layout/activity/rows");
    confirmation.Close();confirmation.ReleaseSelectionCallback();(void)active.Remove(0xabcdu);active.ReleaseStorage();
    std::cout<<"PASS: native SaveWindow layout/global keys, no hover confirmation, actual Yes/No, backend ordering, System return and replacement-safe cleanup\n";
    std::cout<<"PASS: original SaveWindow all100 slots twice, occupied/empty metadata, page tags/mask, reset and native row GPU/cleanup; pixels="<<pixels<<'\n';
}
}
int main(int argc,char** argv)try{Check(argc==2,"provide original game directory");const pal4::test::NativeDesktopObservation desktop;Run(argv[1]);desktop.Check("SaveWindow native page");return 0;}
catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}
