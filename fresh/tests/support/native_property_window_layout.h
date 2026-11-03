#pragma once
#include "pal4/native_cegui_ui_session.h"
#include "pal4/property_window_lifecycle.h"
#include "../../native/ui/cegui_exports_native.h"
#include <array>
#include <stdexcept>
namespace pal4::test {
// Real original PropertyWindow layout shared by DLL-only and full-plugin probes.
struct NativePropertyWindowLayout {
    HMODULE module{},look{};
    void* manager{};void* root_window{};
    PropertyWindowState state;
    static void Check(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
    static void* Ptr(std::uint32_t a){return reinterpret_cast<void*>(a);}
    NativePropertyWindowLayout(NativeCeguiUiSession& ui,const std::filesystem::path& root,bool main_layout_loaded=false){
        using detail::Export;
        try {
            if(!ui.LoadImageset(root,"gamedata\\ui\\fonts\\fontPuzzle_0.imageset"))
                throw std::runtime_error("Property puzzle font: "+ui.LastError());
            Check(ui.InitializeWidgetRendering(root),"Property widgets");
            for(const auto* set:{"OiramLook","kuang2_0","zi_0","gy_0","jingquedianji_0","maimai_0","MultiList_0"}){
                const auto path=std::string("gamedata\\ui\\imagesets\\")+set+".imageset";
                Check(ui.LoadImageset(root,path.c_str()),"Property original imageset");
            }
            // Start creates a desktop before the original main layout/fonts.
            // Composed callers explicitly declare that they already loaded it.
            if(!main_layout_loaded)Check(ui.LoadOriginalMainLayout() && ui.HideMainLayout(),"Property main fonts");
            module=GetModuleHandleA("CEGUIBase.dll");look=GetModuleHandleA("OIRAMLOOK.dll");
            manager=Export<void* (__cdecl*)()>(module,"?getSingletonPtr@WindowManager@CEGUI@@SAPAV12@XZ")();
            pal4::detail::NativeString path(module,"gamedata\\ui\\layouts\\PropertyWindow.xml"),prefix(module,"PropertyWindow/"),empty(module,"");
            root_window=Export<void* (__thiscall*)(void*,const void*,const void*,const void*,void*,void*)>(module,
                "?loadWindowLayout@WindowManager@CEGUI@@QAEPAVWindow@2@ABVString@2@00P6A_NPAV32@AAV42@2PAX@Z3@Z")(
                    manager,path.Data(),prefix.Data(),empty.Data(),nullptr,nullptr);
            Check(root_window!=nullptr,"original Property layout");
            state.Write(36u,reinterpret_cast<std::uint32_t>(root_window));
            for(const auto [name,offset]:std::array<std::pair<const char*,std::uint32_t>,14>{{
                {"PropCure",64u},{"Attack",68u},{"Auxiliary",72u},{"PropMaterial",76u},{"PropScenario",80u},{"NewProp",84u},
                {"PropertyList",88u},{"PropertyText",92u},{"PropertyDescription",96u},{"PropertyProp",100u},{"PropertyClassImage",104u},
                {"PropertyWindow",140u},{"PropertyWindow2",144u},{"PropertyWindow3",148u}}}){
                pal4::detail::NativeString key(module,std::string("PropertyWindow/")+name);
                const auto window=Export<void* (__thiscall*)(const void*,const void*)>(module,
                    "?getWindow@WindowManager@CEGUI@@QBEPAVWindow@2@ABVString@2@@Z")(manager,key.Data());
                Check(window!=nullptr,"Property original control");state.Write(offset,reinterpret_cast<std::uint32_t>(window));
            }
            const auto list=state.Read(88u);
            const auto columns=Export<std::uint32_t (__thiscall*)(const void*)>(module,"?getColumnCount@MultiColumnList@CEGUI@@QBEIXZ");
            while(columns(Ptr(list)))Export<void (__thiscall*)(void*,std::uint32_t)>(module,"?removeColumn@MultiColumnList@CEGUI@@QAEXI@Z")(Ptr(list),0u);
            for(unsigned column=0;column<2u;++column)Export<void (__thiscall*)(void*,const void*,std::uint32_t,float)>(module,
                "?addColumn@MultiColumnList@CEGUI@@QAEXABVString@2@IM@Z")(Ptr(list),empty.Data(),column,column?0.4f:0.5f);
            for(auto symbol:{"?setUserColumnSizingEnabled@MultiColumnList@CEGUI@@QAEX_N@Z",
                "?setUserColumnDraggingEnabled@MultiColumnList@CEGUI@@QAEX_N@Z","?setUserSortControlEnabled@MultiColumnList@CEGUI@@QAEX_N@Z"})
                Export<void (__thiscall*)(void*,bool)>(module,symbol)(Ptr(list),false);
            for(auto offset:{96u,100u})Export<void (__thiscall*)(void*,bool)>(module,"?setEnabled@Window@CEGUI@@QAEX_N@Z")(Ptr(state.Read(offset)),false);
            Check(ui.BindDialogRootParent(state.Read(36u),true),"Property root attachment");
        }catch(...){Destroy();throw;}
    }
    NativePropertyWindowLayout(const NativePropertyWindowLayout&)=delete;
    ~NativePropertyWindowLayout(){Destroy();}
    void Destroy(){if(root_window){detail::Export<void (__thiscall*)(void*,void*)>(module,
        "?destroyWindow@WindowManager@CEGUI@@QAEXPAVWindow@2@@Z")(manager,root_window);root_window=nullptr;}}
};
}
