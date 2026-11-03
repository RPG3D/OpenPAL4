#include "pal4/native_common_dialog_layout.h"
#include "pal4/native_cegui_ui_session.h"
#include "pal4/yes_no_dialog.h"
#include "support/native_cegui_test_click.h"
#include "support/native_desktop_observation.h"
#include <array>
#include <iostream>
#include <stdexcept>
namespace {void Require(bool value,const char* text){if(!value)throw std::runtime_error(text);}}
void VerifyNativeYesNoDialog(pal4::NativeCeguiUiSession& ui,const std::filesystem::path& root){
    using pal4::detail::Export;const auto dll=GetModuleHandleA("CEGUIBase.dll");
    const pal4::test::NativeDesktopObservation desktop;unsigned sounds{};
    const auto sound=[&](std::string_view name,unsigned repeat){Require(name=="wa03.wav" && repeat==1u,"YesNo original sound");++sounds;};
    const auto parent=[&](const pal4::NativeCommonDialogLayout& d){return Export<void* (__thiscall*)(const void*)>(dll,"?getParent@Window@CEGUI@@QBEPAV12@XZ")(reinterpret_cast<void*>(d.RootAddress()));};
    const auto modal=[&](){return Export<void* (__thiscall*)(const void*)>(dll,"?getModalTarget@System@CEGUI@@QBEPAVWindow@2@XZ")(reinterpret_cast<void*>(ui.SystemAddress()));};
    pal4::detail::NativeString empty(dll,"");
    const auto key=[&](pal4::NativeCommonDialogLayout& d,unsigned code){
        std::array<unsigned,5> event{};Export<void (__thiscall*)(void*,void*)>(dll,"??0WindowEventArgs@CEGUI@@QAE@PAVWindow@1@@Z")(event.data(),reinterpret_cast<void*>(d.RootAddress()));event[4]=code;
        Export<void (__thiscall*)(void*,const void*,void*,const void*)>(dll,"?fireEvent@EventSet@CEGUI@@UAEXABVString@2@AAVEventArgs@2@0@Z")(reinterpret_cast<void*>(d.RootAddress()),Export<const void*>(dll,"?EventKeyDown@Window@CEGUI@@2VString@2@B"),event.data(),empty.Data());
        Export<void (__thiscall*)(void*)>(dll,"??1WindowEventArgs@CEGUI@@UAE@XZ")(event.data());d.RethrowEventError();};
    for(unsigned pass=0;pass<2u;++pass){
        pal4::NativeCommonDialogLayout common(dll,ui,root);
        pal4::NativeCommonDialogLayout d(dll,ui,root,pal4::NativeCommonDialogLayout::Kind::yes_no);
        Require(d.FrameAddress() && d.RootAddress()!=common.RootAddress() && !d.Visible() && !parent(d),"CommonDialog2 is independent and constructed detached");
        Require(*reinterpret_cast<const unsigned*>(d.FrameAddress())==pal4::YesNoDialog::kFrameVtable &&
            *reinterpret_cast<const unsigned*>(d.Window("BtYes")+280u)==1u &&
            *reinterpret_cast<const unsigned*>(d.Window("BtNo")+280u)==0u,"original YesNo vtable and distinct Yes1/No0 UserData values");
        const auto show=[&](){d.ShowYesNo("<colour red=255 green=187 blue=0 alpha=255>" "\xCF\xE0\xB9\xAB" " Quest?</colour>",sound);Require(d.Visible() && parent(d) && modal()==reinterpret_cast<void*>(d.RootAddress()) && d.LastSelection()==0u,"YesNo Show attaches original modal and clears result");};
        show();
        const auto text=Export<const void* (__thiscall*)(const void*)>(dll,"?getText@Window@CEGUI@@QBEABVString@2@XZ")(reinterpret_cast<void*>(d.Window("Message")));
        const auto utf8=Export<const char* (__thiscall*)(const void*)>(dll,"?c_str@String@CEGUI@@QBEPBDXZ")(text);
        Require(utf8 && std::string_view(utf8).find("\xE7\x9B\xB8\xE5\x85\xAC")!=std::string_view::npos,"original quest GBK text converts to actual DLL UTF8 message");
        pal4::test::ClickNativeCeguiWindow(ui,d.Window("BtYes"));d.RethrowEventError();
        Require(!d.Visible() && !parent(d) && !modal() && d.LastSelection()==1u,"actual original DLL Yes stores1 and clears modal/parent");
        show();pal4::test::ClickNativeCeguiWindow(ui,d.Window("BtNo"));d.RethrowEventError();Require(!d.Visible() && !parent(d) && !modal() && !d.LastSelection(),"actual original DLL No stores0 and closes");
        show();key(d,1u);Require(!d.Visible() && !modal() && !d.LastSelection(),"actual original root Esc stores No0 and closes");
        show();key(d,28u);Require(!d.Visible() && d.LastSelection()==1u,"actual original root Enter selects initial Yes");
        show();key(d,205u);key(d,28u);Require(!d.Visible() && !d.LastSelection(),"actual original root Right/Enter selects No");
        show();show();d.Close();Require(!d.Visible() && !parent(d) && !modal(),"replacing visible YesNo remains modal until explicit close");
        bool wrong{};try{d.Show(U"wrong",3u,{},sound);}catch(const std::logic_error&){wrong=true;}Require(wrong,"typed YesNo rejects common-dialog callback layout");
        common.Show(U"common",4u,{},[](std::string_view name,unsigned repeat){Require(name=="wa04.wav" && repeat==1u,"ordinary warning keeps its original sound");});Require(common.Visible() && modal()==reinterpret_cast<void*>(common.RootAddress()),"ordinary CommonDialog remains usable alongside independent YesNo");common.Close();
    }
    Require(sounds==14u,"YesNo uses supplied silent audio policy");desktop.Check("YesNo original DLL regression");
    std::cout<<"PASS: CommonDialog2 original Yes1/No0, text/modal, root Esc/Enter/Right, replacement and independent CommonDialog lifetime\n";
}
