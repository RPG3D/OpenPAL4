#include "pal4/native_magic_window_layout.h"
#include "pal4/native_cegui_ui_session.h"
#include "pal4/native_game_database.h"
#include "support/native_cegui_frame_session.h"
#include "support/native_desktop_observation.h"
#include "support/native_cegui_test_click.h"
#include "support/native_magic_window_presentation_probe.h"
#include "../native/ui/cegui_exports_native.h"
#include <rtfsyst.h>
#include <array>
#include <iostream>
#include <set>
#include <vector>
namespace {
using pal4::detail::Export;
void Require(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
void* Ptr(unsigned a){return reinterpret_cast<void*>(a);}
void Run(const std::filesystem::path& root,bool presentation,bool interaction,bool points,bool casts,bool stunts){
    pal4::test::NativeFrameSession frame;Require(frame.Start(800,600) && !IsWindowVisible(frame.window),"magic hidden secondary-screen GPU host");
    struct Files{bool ready=RtFSManagerOpen(8)!=FALSE;~Files(){if(ready)RtFSManagerClose();}} files;
    Require(files.ready,"magic filesystem");pal4::NativeCeguiUiSession ui;Require(ui.Start(root,800,600),"magic original UI");
    Require(ui.LoadImageset(root,"gamedata\\ui\\fonts\\fontPuzzle_0.imageset") && ui.InitializeWidgetRendering(root) &&
        ui.LoadImageset(root,"gamedata\\ui\\imagesets\\OiramLook.imageset") && ui.LoadOriginalMainLayout() && ui.HideMainLayout(),"magic original fonts/widgets");
    pal4::NativeGameDatabase db;Require(db.Load(root),"magic original database");
    if(presentation){pal4::test::VerifyNativeMagicWindowPresentation(ui,root,db,frame);return;}
    if(interaction){pal4::test::VerifyNativeMagicWindowInteraction(ui,root,db,frame);return;}
    if(points){pal4::test::VerifyNativeMagicWindowPointAction(ui,root,db,frame);return;}
    if(casts){pal4::test::VerifyNativeMagicWindowCastAction(ui,root,db,frame);return;}
    if(stunts){pal4::test::VerifyNativeMagicWindowCastAction(ui,root,db,frame,true);return;}
    const auto module=GetModuleHandleA("CEGUIBase.dll");
    const auto wm=Export<void* (__cdecl*)()>(module,"?getSingletonPtr@WindowManager@CEGUI@@SAPAV12@XZ")();
    const auto present=Export<bool (__thiscall*)(const void*,const void*)>(module,"?isWindowPresent@WindowManager@CEGUI@@QBE_NABVString@2@@Z");
    pal4::detail::NativeString root_name(module,"magicWindow/Root"),empty(module,"");
    const auto fire=Export<void (__thiscall*)(void*,const void*,void*,const void*)>(module,"?fireEvent@EventSet@CEGUI@@UAEXABVString@2@AAVEventArgs@2@0@Z");
    const auto colour_argb=[&](unsigned w,const char* getter){alignas(4) std::array<std::byte,24> colour{};
        Export<void* (__thiscall*)(const void*,void*)>(module,getter)(Ptr(w),colour.data());
        return Export<unsigned (__thiscall*)(const void*)>(module,"?getARGB@colour@CEGUI@@QBEIXZ")(colour.data());};
    for(unsigned pass=0u;pass<3u;++pass){
        Require(!present(wm,root_name.Data()),"magic previous layout destroyed before recreation");
        pal4::NativeMagicWindowLayout* current{};bool fail{},retire{};
        struct Call{unsigned owner,handler,window;};std::vector<Call> calls;
        pal4::NativeMagicWindowLayout layout(ui,root,db,0x1000u,[&](unsigned owner,unsigned handler,unsigned argument){
            if(fail)throw std::runtime_error("magic event error sentinel");
            if(retire){current->Close();return true;}
            unsigned window{};if(handler!=pal4::MagicWindow::kCallbackHandler)window=*reinterpret_cast<const unsigned*>(argument+8u);
            calls.push_back({owner,handler,window});return true;});current=&layout;
        const auto& panel=layout.Panel();const auto object=layout.FrameAddress();
        Require(layout.RootAddress() && present(wm,root_name.Data()),"magic original layout registered");
        Require(layout.State().slot_order==std::array<unsigned,6>{5u,0u,1u,2u,3u,4u} &&
            layout.State().category_position==1u && layout.State().selection_owner==object && layout.State().list_owner==object,
            "magic shared constructor order and self links");
        unsigned texts{};
        for(const auto& [index,text]:db.Properties().at("AiCommandType").entries)if(index>=0 && index<7){
                Require(layout.State().stunt_text[index]==text,"magic original AiCommandType text copied");++texts;}
        Require(texts==7u,"magic original database provides all seven constructor texts");
        std::set<unsigned> controls;
        for(const auto w:panel.windows)if(w)Require(controls.insert(w).second,"magic distinct original controls");
        Require(controls.size()==61u,"magic all61 original controls bound");
        const auto check_index=[&](unsigned w,unsigned index){Require(reinterpret_cast<unsigned>(Export<void* (__thiscall*)(const void*)>(module,
            "?getUserData@Window@CEGUI@@QBEPAXXZ")(Ptr(w)))==index,"magic row/category native userData index");};
        for(unsigned row=0u;row<6u;++row){
            check_index(panel.At(0xa8u+row*4u),row);check_index(panel.At(0xc0u+row*4u),row);check_index(panel.At(0xf0u+row*4u),row);
            Require(*reinterpret_cast<const unsigned*>(panel.At(0xc0u+row*4u)+1196u)==0u,"magic name clears original linked handler field");
            for(const auto* getter:{"?getNormalTextColour@ButtonBase@CEGUI@@QBE?AVcolour@2@XZ","?getDisabledTextColour@ButtonBase@CEGUI@@QBE?AVcolour@2@XZ",
                "?getHoverTextColour@ButtonBase@CEGUI@@QBE?AVcolour@2@XZ","?getPushedTextColour@ButtonBase@CEGUI@@QBE?AVcolour@2@XZ"})
                Require(colour_argb(panel.At(0xc0u+row*4u),getter)==0xff000000u,"magic four name states use opaque black DLL colour");
            Require(colour_argb(panel.At(0xf0u+row*4u),"?getNormalTextColour@ButtonBase@CEGUI@@QBE?AVcolour@2@XZ")==
                colour_argb(panel.At(0xf0u+row*4u),"?getHoverTextColour@ButtonBase@CEGUI@@QBE?AVcolour@2@XZ"),"magic point hover preserves normal colour");
        }
        const auto sound_sink=*reinterpret_cast<const unsigned*>(panel.At(0x108u)+1196u);
        Require(sound_sink && sound_sink!=object+0x58u,"magic stunt links owned native sound ABI rather than original EXE vtable");
        for(unsigned row=0u;row<7u;++row){check_index(panel.At(0x108u+row*4u),row);
            Require(*reinterpret_cast<const unsigned*>(panel.At(0x108u+row*4u)+1196u)==sound_sink,"magic all stunt buttons share owned sound sink");}
        {
            struct SoundRecorder{HMODULE module;unsigned count{};explicit SoundRecorder(HMODULE m):module(m){}
                virtual void Sound(const void* name){Require(Export<bool (__cdecl*)(const void*,const void*)>(module,"??8CEGUI@@YA_NABVString@0@0@Z")(
                    name,Export<const void*>(module,"?SoundClicked@PushButton@CEGUI@@2VString@2@B")),"magic sound remaps Deny to Clicked");++count;}} recorder(module);
            auto* manager_sink=reinterpret_cast<unsigned*>(static_cast<std::byte*>(wm)+36u);const auto previous=*manager_sink;
            struct Restore{unsigned* word;unsigned previous;~Restore(){*word=previous;}} restore{manager_sink,previous};
            *manager_sink=reinterpret_cast<unsigned>(&recorder);
            const auto sound=reinterpret_cast<void (__thiscall*)(void*,const void*)>((*reinterpret_cast<void***>(sound_sink))[0]);
            sound(Ptr(sound_sink),Export<const void*>(module,"?SoundClicked@PushButton@CEGUI@@2VString@2@B"));layout.RethrowEventError();
            Require(!recorder.count,"magic click sound ignores non-denied token like original45ABF0");
            sound(Ptr(sound_sink),Export<const void*>(module,"?SoundClickDeny@PushButton@CEGUI@@2VString@2@B"));layout.RethrowEventError();
            Require(recorder.count==1u,"magic sound forwards remapped token to existing WindowManager sink once");
        }
        Require(layout.Word(0x34u)-layout.Word(0x30u)==24u,"magic category navigation owns six DLL controls");
        const auto emit=[&](unsigned window,const char* symbol,unsigned expected_owner,unsigned expected_handler){
            alignas(4) std::array<unsigned,16> event{};
            Export<void (__thiscall*)(void*,void*)>(module,"??0WindowEventArgs@CEGUI@@QAE@PAVWindow@1@@Z")(event.data(),Ptr(window));
            const auto before=calls.size();fire(Ptr(window),Export<const void*>(module,symbol),event.data(),empty.Data());
            Export<void (__thiscall*)(void*)>(module,"??1WindowEventArgs@CEGUI@@UAE@XZ")(event.data());layout.RethrowEventError();
            Require(calls.size()==before+1u && calls.back().owner==expected_owner && calls.back().handler==expected_handler && calls.back().window==window,
                "magic original DLL event reaches correct rewritten owner/handler exactly once");};
        const char* click="?EventClicked@PushButton@CEGUI@@2VString@2@B";
        const char* move="?EventMouseMove@Window@CEGUI@@2VString@2@B";
        const char* mouse="?EventMouseClick@Window@CEGUI@@2VString@2@B";
        for(unsigned row=0u;row<6u;++row){
            emit(panel.At(0xa8u+row*4u),click,object,0x459ae0u);
            emit(panel.At(0xc0u+row*4u),move,object,0x459bb0u);
            emit(panel.At(0xf0u+row*4u),click,object,0x459780u);
            emit(panel.At(0xa8u+row*4u),move,object+0x28u,0x4af760u);
            emit(panel.At(0xa8u+row*4u),"?EventMouseButtonUp@Window@CEGUI@@2VString@2@B",object+0x28u,0x4af770u);
        }
        for(unsigned row=0u;row<7u;++row){emit(panel.At(0x108u+row*4u),mouse,object,0x459c00u);emit(panel.At(0x108u+row*4u),move,object,0x459bd0u);}
        emit(panel.At(0x80u),click,object,0x4596e0u);emit(panel.At(0x84u),click,object,0x459730u);
        layout.DispatchCommand("use_stunt",7000u);Require(calls.back().handler==0x45a500u && calls.back().owner==object,"magic owned use_stunt command");
        Require(ui.BindDialogRootParent(layout.RootAddress(),true) && frame.Begin() && ui.Render(),"magic original DLL layout GPU draw");
        const auto pixels=frame.ReadPixels();unsigned lit{};for(std::size_t i=0u;i+3u<pixels.size();i+=4u)if(pixels[i] || pixels[i+1u] || pixels[i+2u])++lit;
        Require(lit>1000u,"magic actual layout draws nonempty GPU pixels");
        for(unsigned category=0u;category<6u;++category){
            const auto count_clicks=[&]{return std::count_if(calls.begin(),calls.end(),[&](const auto& call){
                return call.owner==object && call.handler==0x459ae0u && call.window==panel.At(0xa8u+category*4u);});};
            const auto before=count_clicks();pal4::test::ClickNativeCeguiWindow(ui,panel.At(0xa8u+category*4u));layout.RethrowEventError();
            Require(count_clicks()==before+1u,"magic actual mouse hit/down/up clicks each original category exactly once");
        }
        const auto visible=Export<void (__thiscall*)(void*,bool)>(module,"?setVisible@Window@CEGUI@@QAEX_N@Z");
        visible(Ptr(panel.At(0x15cu)),false);visible(Ptr(panel.At(0x164u)),true);
        for(unsigned row=0u;row<7u;++row){const auto button=panel.At(0x108u+row*4u);
            visible(Ptr(button),true);Export<void (__thiscall*)(void*,bool)>(module,"?setEnabled@Window@CEGUI@@QAEX_N@Z")(Ptr(button),true);
            const auto count_clicks=[&]{return std::count_if(calls.begin(),calls.end(),[&](const auto& call){
                return call.owner==object && call.handler==0x459c00u && call.window==button;});};
            const auto before=count_clicks();pal4::test::ClickNativeCeguiWindow(ui,button);layout.RethrowEventError();
            Require(count_clicks()==before+1u,"magic actual mouse click uses native stunt member link without calling original EXE vtable");
        }
        fail=true;bool caught{};try{emit(panel.At(0x80u),click,object,0x4596e0u);}catch(const std::runtime_error& error){caught=std::string_view(error.what())=="magic event error sentinel";}
        Require(caught,"magic subscriber retains exceptions outside DLL boundary");fail=false;
        retire=true;caught=false;try{emit(panel.At(0x84u),click,object,0x459730u);}catch(const std::logic_error&){caught=true;}
        Require(caught && present(wm,root_name.Data()),"magic callback cannot destroy live event layout");retire=false;
        retire=true;caught=false;try{layout.DispatchCommand("use_stunt",7000u);}catch(const std::logic_error&){caught=true;}
        Require(caught && present(wm,root_name.Data()),"magic command cannot destroy its own layout while dispatching");retire=false;
        layout.Close();layout.Close();Require(!layout.RootAddress() && !present(wm,root_name.Data()),"magic Close removes original XML and subscribers idempotently");
    }
    std::cout<<"PASS: Magic original constructor/panel61 controls, AiCommandType7 texts,34 page and12 navigation DLL events, real six-category/seven-stunt mouse clicks, native deny-to-click sound ABI, colours, GPU, callback/command errors and repeated destruction\n";
}
}
int main(int argc,char** argv){try{Require(argc==2 || (argc==3 && (std::string_view(argv[2])=="--presentation" || std::string_view(argv[2])=="--interaction" || std::string_view(argv[2])=="--points" || std::string_view(argv[2])=="--casts" || std::string_view(argv[2])=="--stunts")),"original root argument");
    SetUnhandledExceptionFilter([](EXCEPTION_POINTERS* e)->LONG{HMODULE module{};char path[MAX_PATH]{};const auto ip=e->ExceptionRecord->ExceptionAddress;
        GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,reinterpret_cast<const char*>(ip),&module);
        GetModuleFileNameA(module,path,MAX_PATH);
        std::cerr<<"Magic exception "<<std::hex<<e->ExceptionRecord->ExceptionCode<<" module="<<path<<" offset="<<(reinterpret_cast<unsigned>(ip)-reinterpret_cast<unsigned>(module))
            <<" access="<<e->ExceptionRecord->ExceptionInformation[0]<<" address="<<e->ExceptionRecord->ExceptionInformation[1]<<" eax="<<e->ContextRecord->Eax<<" ecx="<<e->ContextRecord->Ecx<<std::endl;
        return EXCEPTION_EXECUTE_HANDLER;});
    pal4::test::NativeDesktopObservation desktop;
    Run(argv[1],argc==3 && std::string_view(argv[2])=="--presentation",argc==3 && std::string_view(argv[2])=="--interaction",argc==3 && std::string_view(argv[2])=="--points",argc==3 && std::string_view(argv[2])=="--casts",argc==3 && std::string_view(argv[2])=="--stunts");desktop.Check("native magic layout preserves desktop focus and cursor");return 0;
}catch(const std::exception& error){std::cerr<<"FAIL: "<<error.what()<<'\n';return 1;}}
