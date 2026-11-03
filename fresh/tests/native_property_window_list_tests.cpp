#include "support/native_cegui_frame_session.h"
#include "support/native_desktop_observation.h"
#include "support/native_property_window_layout.h"
#include "pal4/native_cegui_ui_session.h"
#include "pal4/native_property_window_list.h"
#include "pal4/native_property_window_details.h"
#include "pal4/native_inventory_property_reader.h"
#include "pal4/native_inventory_session.h"
#include "pal4/native_game_database.h"
#include "pal4/native_cegui_text_conversion.h"
#include "../native/ui/cegui_exports_native.h"
#include <rtfsyst.h>
#include <algorithm>
#include <array>
#include <iostream>
#include <numeric>
#include <stdexcept>
#include <map>

namespace {
using pal4::detail::Export;
void Check(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
void* Ptr(std::uint32_t value){return reinterpret_cast<void*>(value);}
struct Filesystem {bool ready{};~Filesystem(){if(ready)RtFSManagerClose();}};
struct Expected {std::uint32_t id,sequence,count;std::string name,description,properties;};
void Run(const std::filesystem::path& root){
    pal4::test::NativeFrameSession frame;Check(frame.Start(800,600),"Property SDK frame");
    Check(!IsWindowVisible(frame.window),"Property frame stays hidden");
    Filesystem files;files.ready=RtFSManagerOpen(8)!=FALSE;Check(files.ready,"Property filesystem");
    pal4::NativeCeguiUiSession ui;Check(ui.Start(root,800,600),"Property UI");
    pal4::test::NativePropertyWindowLayout layout(ui,root);
    const auto module=layout.module,look=layout.look;
    auto& state=layout.state;const auto list=state.Read(88u);
    pal4::NativeGameDatabase database;Check(database.Load(root),"Property real database");
    pal4::NativeInventorySession inventory;(void)inventory.Start();inventory.BindDatabase(database);
    unsigned observed{};inventory.BindItemObserver([&](auto){++observed;});
    pal4::NativeInventoryPropertyReader reader(inventory);
    for(unsigned category=0;category<5u;++category)Check(reader.Count(category)==0,"empty Property category");
    Check(reader.RecentSequences().empty(),"empty recent snapshot");
    unsigned panels{},clears{},model_updates{};bool fail_panel{},fail_model{};
    pal4::NativePropertyWindowDetails* detail_endpoint{};
    std::uint32_t model_sequence{};
    pal4::NativePropertyWindowListServices services;
    services.item_panel=[&](auto& s,bool visible){Check(&s==&state && visible,"Property panel endpoint identity");
        if(fail_panel)throw std::runtime_error("injected panel failure");++panels;
        for(auto offset:{140u,144u,148u})Export<void (__thiscall*)(void*,bool)>(module,"?setVisible@Window@CEGUI@@QAEX_N@Z")(Ptr(s.Read(offset)),visible);};
    services.clear_details=[&](auto& s){Check(&s==&state && detail_endpoint,"Property detail endpoint identity");
        ++clears;detail_endpoint->Clear();};
    const auto count=[&]{return Export<std::uint32_t (__thiscall*)(const void*)>(module,"?getRowCount@MultiColumnList@CEGUI@@QBEIXZ")(Ptr(list));};
    const auto raster=[&]{Check(frame.Begin() && ui.Render(),"Property render");return frame.ReadPixels();};
    std::array<std::vector<Expected>,5> acquired;
    std::map<std::uint32_t,Expected> by_sequence;
    const auto& table=database.Tables().at("Property");
    const auto id_field=std::find_if(table.fields.begin(),table.fields.end(),[](const auto& f){return f.name=="ID";});
    const auto name_field=std::find_if(table.fields.begin(),table.fields.end(),[](const auto& f){return f.name=="PA_NAME";});
    const auto desc_field=std::find_if(table.fields.begin(),table.fields.end(),[](const auto& f){return f.name=="PA_DESCRIPTION";});
    const auto prop_field=std::find_if(table.fields.begin(),table.fields.end(),[](const auto& f){return f.name=="PA_PROPERTY";});
    Check(id_field!=table.fields.end() && name_field!=table.fields.end() && desc_field!=table.fields.end() && prop_field!=table.fields.end(),"Property source fields");
    {
        pal4::NativePropertyWindowDetails details(module,state,inventory,[&](auto& image,std::uint32_t entry){
            Check(&image==&state,"Property model image owner");
            if(fail_model)throw std::runtime_error("injected preview endpoint failure");
            ++model_updates;model_sequence=0u;
            if(entry){
                model_sequence=*reinterpret_cast<const std::uint32_t*>(entry+4u);
                const auto& expected=by_sequence.at(model_sequence);
                const auto definition=*reinterpret_cast<const std::uint32_t*>(entry);
                Check(*reinterpret_cast<const std::uint32_t*>(definition)==expected.id,"preview endpoint receives real inventory entry");
            }
            return 1u;
        });
        detail_endpoint=&details;
        pal4::NativePropertyWindowList binding(module,look,state,inventory,std::move(services));
        binding.Populate(0u);details.RethrowEventError();
        Check(count()==0u && clears==1u && panels==1u && state.Read(0x74u)==0u && model_sequence==0u,"empty Property clears actual detail endpoint");
        const auto blank=raster();
        for(const auto& row:table.rows){
            const auto id=std::get<std::uint32_t>(row[id_field-table.fields.begin()]);
            const auto definition=inventory.PropertyAddress(id);Check(definition!=0u,"original Property definition");
            const auto category=*reinterpret_cast<const std::uint32_t*>(definition+4u);Check(category<5u,"original category");
            const auto quantity=static_cast<std::uint32_t>(acquired[category].size()%97u+1u);
            const auto entry=inventory.AddItem(id,quantity);Check(entry!=0u,"real property addition");
            const auto sequence=*reinterpret_cast<const std::uint32_t*>(entry+4u);
            Expected expected{id,sequence,quantity,std::get<std::string>(row[name_field-table.fields.begin()]),
                std::get<std::string>(row[desc_field-table.fields.begin()]),std::get<std::string>(row[prop_field-table.fields.begin()])};
            acquired[category].push_back(expected);by_sequence.emplace(sequence,expected);
            const auto snapshot=reader.FindSequence(sequence);
            Check(snapshot && snapshot->id==id && snapshot->count==quantity && snapshot->name==expected.name,
                "property reader preserves original ID/sequence/name/count distinction");
        }
        Check(observed==table.rows.size(),"each real addition keeps observer endpoint");
        const auto item_at=[&](unsigned row,unsigned column){const std::array<unsigned,2> grid{row,column};
            return Export<void* (__thiscall*)(const void*,const void*)>(module,
                "?getItemAtGridReference@MultiColumnList@CEGUI@@QBEPAVListboxItem@2@ABUMCLGridRef@2@@Z")(Ptr(list),grid.data());};
        const auto check_rows=[&](const std::vector<Expected>& expected){
            Check(count()==expected.size() && binding.OwnedItemCount()==expected.size()*2u,"two owned DLL items per Property row");
            const auto before_models=model_updates;
            for(unsigned row=0;row<expected.size();++row){
                auto* name=item_at(row,0u);auto* quantity=item_at(row,1u);
                Check(Export<unsigned (__thiscall*)(const void*)>(module,"?getID@ListboxItem@CEGUI@@QBEIXZ")(name)==expected[row].sequence,"list item ID stores sequence");
                const auto text=[&](void* item){const auto string=Export<const void* (__thiscall*)(const void*)>(module,
                    "?getText@ListboxItem@CEGUI@@QBEABVString@2@XZ")(item);return std::string(Export<const char* (__thiscall*)(const void*)>(module,"?c_str@String@CEGUI@@QBEPBDXZ")(string));};
                Check(pal4::ConvertGameTextToUtf8(expected[row].name)==std::optional<std::string>(text(name)) &&
                    text(quantity)==std::to_string(expected[row].count),"actual Chinese Property name and signed quantity text");
                Check(!Export<bool (__thiscall*)(const void*)>(module,"?isAutoDeleted@ListboxItem@CEGUI@@QBE_NXZ")(name),"Property host item ownership");
                Export<void (__thiscall*)(void*,std::int32_t)>(module,"?setHoverRowIndex@MultiColumnList@CEGUI@@QAEXH@Z")(Ptr(list),row);
                details.RethrowEventError();
                const auto window_text=[&](std::uint32_t window){const auto value=Export<const void* (__thiscall*)(const void*)>(module,
                    "?getText@Window@CEGUI@@QBEABVString@2@XZ")(Ptr(window));
                    return std::string(Export<const char* (__thiscall*)(const void*)>(module,"?c_str@String@CEGUI@@QBEPBDXZ")(value));};
                Check(pal4::ConvertGameTextToUtf8(expected[row].description)==std::optional<std::string>(window_text(state.Read(96u))) &&
                    pal4::ConvertGameTextToUtf8(expected[row].properties)==std::optional<std::string>(window_text(state.Read(100u))),
                    "actual DLL hover updates original description/property text");
                Check(state.Read(0x74u)==expected[row].sequence && model_sequence==expected[row].sequence,
                    "hover updates preview endpoint and current sequence together");
            }
            Check(expected.empty() || model_updates-before_models>=expected.size()-1u,"real hover changes invoke preview endpoint");
        };
        unsigned nonzero_scrolls{};
        for(unsigned category=0;category<5u;++category){
            Check(reader.Count(category)==acquired[category].size(),"live reader category count");
            auto expected=acquired[category];std::reverse(expected.begin(),expected.end());
            if(!expected.empty()){
                const auto chosen=acquired[category].front();
                *reinterpret_cast<std::uint32_t*>(inventory.Address()+88u+category*4u)=chosen.sequence;
                expected.erase(std::find_if(expected.begin(),expected.end(),[&](const auto& item){return item.sequence==chosen.sequence;}));
                expected.insert(expected.begin(),chosen);
                Check(reader.Featured(category)->sequence==chosen.sequence,"featured item is sequence-based");
            }
            binding.Populate(category);details.RethrowEventError();check_rows(expected);
            const auto pixels=raster();
            if(!expected.empty())Check(std::inner_product(pixels.begin(),pixels.end(),blank.begin(),std::size_t{},std::plus<>(),[](auto a,auto b){return a!=b?1u:0u;})>20u,"Property DLL rows reach raster");
            if(expected.size()>3u){
                const std::array<unsigned,2> grid{2u,0u};
                Export<void (__thiscall*)(void*,const void*,bool)>(module,"?setItemSelectState@MultiColumnList@CEGUI@@QAEXABUMCLGridRef@2@_N@Z")(Ptr(list),grid.data(),true);
                const auto bar=*reinterpret_cast<const std::uint32_t*>(list+1204u);
                Export<void (__thiscall*)(void*,float)>(module,"?setScrollPosition@Scrollbar@CEGUI@@QAEXM@Z")(Ptr(bar),17.0f);
                const auto get_scroll=Export<float (__thiscall*)(const void*)>(module,"?getScrollPosition@Scrollbar@CEGUI@@QBEMXZ");
                const auto before=get_scroll(Ptr(bar));if(before>0.0f)++nonzero_scrolls;
                binding.Populate(category,true);
                Check(Export<int (__thiscall*)(void*)>(module,"?getHoverRowIndex@MultiColumnList@CEGUI@@QAEHXZ")(Ptr(list))==2,"Property refresh preserves selected row");
                Check(get_scroll(Ptr(bar))==before,"Property refresh preserves real scrollbar position");
            }
        }
        Check(nonzero_scrolls>0u,"populated original Property category has a nonzero preserved scroll offset");
        const auto recent=reader.RecentSequences();auto expected=std::vector<Expected>{};
        for(auto it=recent.rbegin();it!=recent.rend();++it)expected.push_back(by_sequence.at(*it));
        binding.Populate(5u);check_rows(expected);
        Check(!expected.empty() && expected.size()<=6u,"original recent snapshot limited to acquisition history");
        const auto removed=expected.front();(void)inventory.RemoveItem(removed.id,99);
        // The DLL can still hover a row whose inventory entry was just removed.
        // Resolve its sequence again, clear text/model and never reuse old addresses.
        Export<void (__thiscall*)(void*,std::int32_t)>(module,"?setHoverRowIndex@MultiColumnList@CEGUI@@QAEXH@Z")(Ptr(list),0);
        details.UpdateHovered();details.RethrowEventError();
        Check(state.Read(0x74u)==0u && model_sequence==0u,"removed hover entry clears preview and current sequence");
        binding.Populate(5u);Check(count()+1u==expected.size() && !reader.FindSequence(removed.sequence),"removed item disappears from recent live view");
        fail_panel=true;bool failed{};try{binding.Populate(0u);}catch(const std::runtime_error&){failed=true;}
        Check(failed,"Property panel failures are observable");fail_panel=false;binding.Populate(0u);
        details.RethrowEventError();fail_model=true;
        Export<void (__thiscall*)(void*,std::int32_t)>(module,"?setHoverRowIndex@MultiColumnList@CEGUI@@QAEXH@Z")(Ptr(list),1);
        bool model_failed{};try{details.RethrowEventError();}catch(const std::runtime_error&){model_failed=true;}
        Check(model_failed,"preview errors cross actual DLL callback boundary explicitly");
        fail_model=false;details.UpdateHovered();details.RethrowEventError();
        inventory.Reset();binding.Populate(0u);Check(count()==0u && binding.OwnedItemCount()==0u && clears>=2u,"inventory reset clears rows/details/owners");
        details.RethrowEventError();Check(state.Read(0x74u)==0u && model_sequence==0u,"inventory reset also clears live detail/preview state");
        Check(acquired[0].size()>=2u,"original cure records support subscriber retirement test");
        for(unsigned index=0;index<2u;++index){
            auto restored=acquired[0][index];const auto entry=inventory.AddItem(restored.id,restored.count);
            restored.sequence=*reinterpret_cast<const std::uint32_t*>(entry+4u);by_sequence[restored.sequence]=std::move(restored);
        }
        binding.Populate(0u);details.RethrowEventError();details.Clear();
        const auto retired_models=model_updates;
        details.Close();details.Close();
        Export<void (__thiscall*)(void*,std::int32_t)>(module,"?setHoverRowIndex@MultiColumnList@CEGUI@@QAEXH@Z")(Ptr(list),1);
        details.RethrowEventError();Check(model_updates==retired_models && state.Read(0x74u)==0u,
            "retired DLL hover subscriber stops invoking borrowed preview service");
        std::cout<<"Property details: all246 descriptions/properties via real hover, live preview requests, removed-entry clearing and callback failure recovery pass\n";
    }
    Check(count()==0u,"Property binding detaches items before layout teardown");
    Check(ui.BindDialogRootParent(state.Read(36u),false),"detach Property layout");
    std::cout<<"PASS: original Property rows="<<table.rows.size()<<" five categories, sequence priority/recent order, real DLL text/raster, refresh, mutation, failure recovery and teardown\n";
}
}
int main(int argc,char** argv)try{std::cout<<std::unitbuf;Check(argc==2,"provide original game directory");
    pal4::test::NativeDesktopObservation desktop;Run(argv[1]);desktop.Check("Property native list");return 0;
}catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}
