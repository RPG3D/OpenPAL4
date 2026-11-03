#include "support/native_cegui_frame_session.h"
#include "support/native_desktop_observation.h"
#include "pal4/native_cegui_ui_session.h"
#include "pal4/native_mission_window_list.h"
#include "pal4/native_mission_window_details.h"
#include "pal4/native_mission_window_interaction.h"
#include "pal4/native_mission_window.h"
#include "pal4/native_event_handler_array.h"
#include "pal4/native_cegui_window_pointer_events.h"
#include "pal4/native_mission_record_catalog.h"
#include "pal4/native_numbered_list_data.h"
#include "pal4/native_game_database.h"
#include "pal4/native_pal_game_iv_texture_storage.h"
#include "pal4/native_cegui_text_conversion.h"
#include "../native/ui/cegui_exports_native.h"
#include <rtfsyst.h>
#include <algorithm>
#include <array>
#include <iostream>
#include <numeric>
#include <stdexcept>

namespace {
using pal4::detail::Export;
void Check(bool value,const std::string& why){if(!value)throw std::runtime_error(why);}
void* Ptr(std::uint32_t value){return reinterpret_cast<void*>(value);}
struct Filesystem { bool ready{}; ~Filesystem(){if(ready)RtFSManagerClose();} };
struct Layout {
    HMODULE module;void* manager;void* root{};
    ~Layout(){if(root)Export<void (__thiscall*)(void*,void*)>(module,
        "?destroyWindow@WindowManager@CEGUI@@QAEXPAVWindow@2@@Z")(manager,root);}
};
void Run(const std::filesystem::path& root){
    pal4::test::NativeFrameSession frame;
    Check(frame.Start(800,600),"Mission frame startup");
    Check(!IsWindowVisible(frame.window),"Mission probe remains hidden on selected secondary");
    Filesystem filesystem;filesystem.ready=RtFSManagerOpen(8)!=FALSE;
    Check(filesystem.ready,"Mission filesystem startup");
    pal4::NativeCeguiUiSession ui;Check(ui.Start(root,800,600),ui.LastError());
    Check(ui.LoadImageset(root,"gamedata\\ui\\fonts\\fontPuzzle_0.imageset"),ui.LastError());
    Check(ui.InitializeWidgetRendering(root),ui.LastError());
    for(const auto* name:{"kuang_0","kuang2_0","zi_0","gy_0","jingquedianji_0","maimai_0","MultiList_0",
                          "biaoti_0","biaoti2_0","biaoti3_0","OiramLook"}){
        const auto path=std::string("gamedata\\ui\\imagesets\\")+name+".imageset";
        Check(ui.LoadImageset(root,path.c_str()),ui.LastError());
    }
    Check(ui.LoadOriginalMainLayout(),ui.LastError());
    Check(ui.HideMainLayout(),ui.LastError());
    const auto module=GetModuleHandleA("CEGUIBase.dll"),look=GetModuleHandleA("OIRAMLOOK.dll");
    Layout layout{module,Export<void* (__cdecl*)()>(module,"?getSingletonPtr@WindowManager@CEGUI@@SAPAV12@XZ")()};
    pal4::detail::NativeString path(module,"gamedata\\ui\\layouts\\MissionWindow.xml"),prefix(module,"MissionWindow/"),empty(module,"");
    layout.root=Export<void* (__thiscall*)(void*,const void*,const void*,const void*,void*,void*)>(module,
        "?loadWindowLayout@WindowManager@CEGUI@@QAEPAVWindow@2@ABVString@2@00P6A_NPAV32@AAV42@2PAX@Z3@Z")(
            layout.manager,path.Data(),prefix.Data(),empty.Data(),nullptr,nullptr);
    Check(layout.root!=nullptr,"load actual MissionWindow layout");
    pal4::MissionWindowState state;state.Write(0x24u,reinterpret_cast<std::uint32_t>(layout.root));
    for(const auto [name,offset]:std::array<std::pair<const char*,std::uint32_t>,13>{{
        {"List",0x54u},{"BtnMission",0x60u},{"BtnScenario",0x64u},{"PanelScenario",0x6cu},
        {"PanelPicture",0x70u},{"ListWindow",0x74u},{"PanelDesc",0x78u},{"zj",0x7cu},{"ImageLabel",0x80u},
        {"ImgMission",0x68u},{"EditDescription",0x5cu},{"ImgDisusedMask",0x84u},{"ListStory",0x58u}}}){
        pal4::detail::NativeString key(module,std::string("MissionWindow/")+name);
        const auto window=Export<void* (__thiscall*)(const void*,const void*)>(module,
            "?getWindow@WindowManager@CEGUI@@QBEPAVWindow@2@ABVString@2@@Z")(layout.manager,key.Data());
        Check(window!=nullptr,"resolve original Mission control");state.Write(offset,reinterpret_cast<std::uint32_t>(window));
    }
    Check(ui.BindDialogRootParent(state.Read(0x24u),true),ui.LastError());
    pal4::NativeGameDatabase database;Check(database.Load(root),"original Mission database");
    pal4::NativePalGameIVTextureStorage game([](std::uint32_t){});(void)game.Construct();
    const auto stories=game.StoryObjects();
    pal4::NativeMissionRecordCatalog records(database);
    const auto list=state.Read(0x54u);
    const auto count=[&]{return Export<std::uint32_t (__thiscall*)(const void*)>(module,
        "?getRowCount@MultiColumnList@CEGUI@@QBEIXZ")(Ptr(list));};
    const auto render=[&]{Check(frame.Begin(),"Mission camera begin");Check(ui.Render(),ui.LastError());return frame.ReadPixels();};
    {
        pal4::NativeMissionWindowList binding(module,look,state,database,game);
        pal4::NativeMissionWindowDetails details(ui,state,database);
        pal4::NativeMissionWindowInteraction* interaction{};
        pal4::NativeEventHandlerArray activity(
            [&](auto member,auto slot){Check(interaction && interaction->OwnsActivity(member),"Mission callback has live native owner");
                return interaction->DispatchActiveCallback(member,slot);},
            [&](auto member,auto slot,auto event){Check(interaction && interaction->OwnsActivity(member),"Mission key has live native owner");
                return interaction->DispatchActivePredicate(member,slot,event);});
        unsigned detail_updates{},story_clicks{};bool fail_callback{};
        pal4::NativeMissionWindowInteraction events(module,state,activity,[&]{
            if(fail_callback)throw std::runtime_error("injected Mission detail callback failure");
            ++detail_updates;details.UpdateHovered();},[&](auto event){++story_clicks;details.HandleStoryClick(event);});
        interaction=&events;
        const auto select=[&](std::int32_t row){
            Export<void (__thiscall*)(void*,std::int32_t)>(module,"?setHoverRowIndex@MultiColumnList@CEGUI@@QAEXH@Z")(Ptr(list),row);
            events.RethrowEventError();};
        binding.Clear();std::vector<std::uint8_t> empty_pixels;
        for(unsigned pass=0;pass<3u;++pass){
            const auto level=pass==1u?INT32_MAX:-1;
            for(const auto pointer:stories)*reinterpret_cast<std::int32_t*>(pointer)=level;
            binding.Populate();
            const auto expected=records.Assigned(game);
            std::size_t row{};
            for(const auto& record:expected)if(record){
                const std::array<std::uint32_t,2> grid{static_cast<std::uint32_t>(row),0u};
                const auto item=Export<void* (__thiscall*)(const void*,const void*)>(module,
                    "?getItemAtGridReference@MultiColumnList@CEGUI@@QBEPAVListboxItem@2@ABUMCLGridRef@2@@Z")(Ptr(list),grid.data());
                Check(item!=nullptr,"real Mission row stores actual DLL item");
                Check(Export<std::uint32_t (__thiscall*)(const void*)>(module,"?getID@ListboxItem@CEGUI@@QBEIXZ")(item)==record->id,
                    "actual DLL task ID follows Story selection");
                const auto text=Export<const void* (__thiscall*)(const void*)>(module,"?getText@ListboxItem@CEGUI@@QBEABVString@2@XZ")(item);
                const auto utf8=Export<const char* (__thiscall*)(const void*)>(module,"?c_str@String@CEGUI@@QBEPBDXZ")(text);
                Check(pal4::ConvertGameTextToUtf8(record->title)==std::optional<std::string>(utf8),"actual DLL Chinese task name");
                Check(*(static_cast<const unsigned char*>(item)+0x1a5u)==(record->availability>0),"original signed STORY_SHOW reaches item+1A5");
                Check(!Export<bool (__thiscall*)(const void*)>(module,"?isAutoDeleted@ListboxItem@CEGUI@@QBE_NXZ")(item),
                    "host-owned DLL item cannot be freed by the list allocator");
                ++row;
            }
            Check(count()==row && binding.OwnedItemCount()==row,"real rows and host item ownership agree");
            const auto pixels=render();Check(pixels.size()==800u*600u*4u,"read actual Mission raster");
            if(pass==0u){Check(row==0u,"locked original missions produce an empty list");empty_pixels=pixels;}
            if(pass==1u){
                const auto changed=std::inner_product(pixels.begin(),pixels.end(),empty_pixels.begin(),std::size_t{},
                    std::plus<>(),[](auto a,auto b){return a!=b?1u:0u;});
                Check(row>0u && changed>20u,"original DLL measures and draws populated task rows");
                std::cout<<"Mission populated rows="<<row<<" changed raster bytes="<<changed<<'\n';
                std::size_t selected_row{},pictures{};
                const auto before_details=ui.TextureLoads();
                for(const auto& selected:expected)if(selected){
                    const auto before_hover=detail_updates;
                    Export<void (__thiscall*)(void*,std::int32_t)>(module,"?setHoverRowIndex@MultiColumnList@CEGUI@@QAEXH@Z")(
                        Ptr(list),static_cast<std::int32_t>(selected_row++));
                    events.RethrowEventError();
                    if(selected_row>1u)Check(detail_updates>before_hover,"actual DLL HoverRowChanged invokes native details callback");
                    const auto& rows=records.Records();
                    const auto full=std::find_if(rows.begin(),rows.end(),[&](const auto& record){return record.id==selected->id;});
                    Check(full!=rows.end(),"hovered ID resolves original Mission record");
                    const auto text=Export<const void* (__thiscall*)(const void*)>(module,"?getText@Window@CEGUI@@QBEABVString@2@XZ")(Ptr(state.Read(0x5cu)));
                    const auto utf8=Export<const char* (__thiscall*)(const void*)>(module,"?c_str@String@CEGUI@@QBEPBDXZ")(text);
                    Check(pal4::ConvertGameTextToUtf8(full->description)==std::optional<std::string>(utf8),"hover updates actual Chinese description control");
                    const auto before_cached=ui.TextureLoads();
                    details.UpdateHovered();
                    Check(ui.TextureLoads()==before_cached,"repeated hover reuses the real quest imageset");
                    const auto image=ui.LoadMissionImage(*full);
                    Check(ui.LastError().empty(),ui.LastError());
                    Check(image!=0u || full->picture.empty(),"quest image load returns actual image or intentional empty");
                    const auto attached=Export<const void* (__thiscall*)(const void*)>(module,
                        "?getImage@StaticImage@CEGUI@@QBEPBVImage@2@XZ")(Ptr(state.Read(0x68u)));
                    Check(attached==Ptr(image),"actual picture control borrows selected quest image");
                    if(image)++pictures;
                    (void)render();
                }
                Check(pictures>0u && ui.TextureLoads()>before_details,"original packed quest PNGs decoded through real renderer");
                auto missing=records.Records().front();missing.picture="pal4_missing_mission_image_test";
                Check(!ui.LoadMissionImage(missing) && !ui.LastError().empty(),"missing packed quest picture reports failure");
                pal4::detail::NativeString missing_set(module,"Pal4QuestImage"+missing.picture);
                const auto image_manager=Export<void* (__cdecl*)()>(module,"?getSingletonPtr@ImagesetManager@CEGUI@@SAPAV12@XZ")();
                Check(!Export<bool (__thiscall*)(const void*,const void*)>(module,
                    "?isImagesetPresent@ImagesetManager@CEGUI@@QBE_NABVString@2@@Z")(image_manager,missing_set.Data()),
                    "failed quest texture is removed from the imageset cache");
                details.UpdateHovered();Check(ui.LastError().empty(),"valid quest hover recovers after missing image");
                Check(ui.LoadSaveThumbnail(0,root)!=0u && ui.LastError().empty(),"shared image DLL boundary still reads filesystem save thumbnail");
                details.UpdateHovered();Check(ui.LastError().empty(),"packed quest cache remains valid after filesystem thumbnail");
                std::cout<<"Mission details descriptions="<<selected_row<<" pictures="<<pictures<<'\n';
                // CEGUI generates Click separately from the move/button-up
                // helper used by the game's navigation code. Enter through the
                // real list DLL click method to exercise its EventMouseClick.
                pal4::CeguiMouseEvent click{};click.window=list;
                click.vtable=reinterpret_cast<std::uint32_t>(Export<const void*>(module,"??_7MouseEventArgs@CEGUI@@6B@"));
                click.position=pal4::NativeCeguiWindowPointerEvents(module).AbsolutePosition(list);
                click.position.x+=20.0f;click.position.y+=40.0f;
                Export<void (__thiscall*)(void*,void*)>(module,"?onMouseClicked@MultiColumnList@CEGUI@@MAEXAAVMouseEventArgs@2@@Z")(
                    Ptr(list),&click);
                events.RethrowEventError();
                const auto address=reinterpret_cast<std::uint32_t>(state.bytes.data());
                Check(activity.GetLast()==address+0x40u,"actual DLL list click selects original task-list activity member");
                const auto key=[&](std::uint32_t code){
                    std::array<std::uint32_t,5> event{};event[4]=code;
                    return activity.DispatchKeyDown(activity.GetLast(),reinterpret_cast<std::uint32_t>(event.data()));};
                const auto hover=[&]{return Export<std::int32_t (__thiscall*)(void*)>(module,
                    "?getHoverRowIndex@MultiColumnList@CEGUI@@QAEHXZ")(Ptr(list));};
                select(0);Check(key(200u)==1u && hover()==0,"Up at first original row does not wrap");
                Check(key(208u)==1u && hover()==1,"native activity Down reaches actual DLL next row");
                Check(key(200u)==1u && hover()==0,"native activity Up reaches actual DLL previous row");
                select(static_cast<std::int32_t>(row-1u));Check(key(208u)==1u && hover()==row-1u,"Down at last row does not wrap");
                select(0);Check(key(99u)==0u,"unrelated task-list key propagates");
                Check(key(28u)==1u && activity.GetLast()==address+0x48u,"Enter enters original scroll activity");
                const auto visible=[&](std::uint32_t w){return Export<bool (__thiscall*)(const void*,bool)>(module,
                    "?isVisible@Window@CEGUI@@QBE_N_N@Z")(Ptr(w),false);};
                Check(visible(state.Read(0x84u)),"Enter shows actual disused mask");
                const auto bar=*reinterpret_cast<const std::uint32_t*>(state.Read(0x58u)+1204u);
                // A real scrollbar range fixture verifies15-pixel stepping;
                // populated StoryDesc/rich-text chapter rows remain separate.
                Export<void (__thiscall*)(void*,float)>(module,"?setDocumentSize@Scrollbar@CEGUI@@QAEXM@Z")(Ptr(bar),1000.0f);
                Export<void (__thiscall*)(void*,float)>(module,"?setPageSize@Scrollbar@CEGUI@@QAEXM@Z")(Ptr(bar),100.0f);
                Export<void (__thiscall*)(void*,float)>(module,"?setScrollPosition@Scrollbar@CEGUI@@QAEXM@Z")(Ptr(bar),45.0f);
                const auto position=[&]{return Export<float (__thiscall*)(const void*)>(module,"?getScrollPosition@Scrollbar@CEGUI@@QBEMXZ")(Ptr(bar));};
                Check(key(200u)==1u && position()==30.0f,"scroll Up applies original minus15 to actual DLL scrollbar");
                Check(key(208u)==1u && position()==45.0f,"scroll Down applies original plus15 to actual DLL scrollbar");
                Check(key(1u)==1u && activity.GetLast()==address+0x40u && !visible(state.Read(0x84u)),"first Esc returns from scroll activity and hides mask");
                Check(key(1u)==1u && activity.Size()==0u,"second Esc removes task-list activity");
                // The original list consumes even unrelated keys when empty.
                binding.Clear();(void)activity.Insert(address+0x40u);
                Check(key(99u)==1u,"empty actual task list consumes the key as original470180 does");
                (void)activity.Remove(address+0x40u);
                std::cout<<"Mission input: DLL hover/click, actual activity Up/Down/Enter/Esc and empty-list behavior pass\n";
            }
        }
        binding.Clear();Check(count()==0u && binding.OwnedItemCount()==0u,"clear detaches rows before DLL item destruction");
        for(const auto pointer:stories)*reinterpret_cast<std::int32_t*>(pointer)=INT32_MAX;
        binding.Populate();
        Check(count()>0u && binding.OwnedItemCount()==count(),"leave attached DLL rows for destructor cleanup probe");
        fail_callback=true;
        Export<void (__thiscall*)(void*,std::int32_t)>(module,"?setHoverRowIndex@MultiColumnList@CEGUI@@QAEXH@Z")(Ptr(list),1);
        bool callback_error{};try{events.RethrowEventError();}catch(const std::runtime_error&){callback_error=true;}
        Check(callback_error,"native callback failure stays behind the DLL boundary until rethrown");
        fail_callback=false;select(2);
        const auto story=state.Read(0x58u);
        while(Export<std::uint32_t (__thiscall*)(const void*)>(module,"?getColumnCount@MultiColumnList@CEGUI@@QBEIXZ")(Ptr(story)))
            Export<void (__thiscall*)(void*,std::uint32_t)>(module,"?removeColumn@MultiColumnList@CEGUI@@QAEXI@Z")(Ptr(story),0u);
        const auto heading=database.FindTableTextById("GameString",193u);Check(heading.has_value(),"original Story column heading");
        const auto heading_utf8=pal4::ConvertGameTextToUtf8(*heading);Check(heading_utf8.has_value(),"Story heading encoding");
        pal4::detail::NativeString story_heading(module,*heading_utf8,pal4::detail::NativeStringEncoding::Utf8);
        Export<void (__thiscall*)(void*,const void*,std::uint32_t,float)>(module,
            "?addColumn@MultiColumnList@CEGUI@@QAEXABVString@2@IM@Z")(Ptr(story),story_heading.Data(),0u,0.89999998f);
        const auto chapters=pal4::NativeNumberedListData(module,database).GetGameRecords();
        Check(!chapters.empty(),"original StoryDesc chapter rows");
        Export<void (__thiscall*)(void*,bool)>(module,"?setVisible@Window@CEGUI@@QAEX_N@Z")(Ptr(list),false);
        std::vector<std::uint8_t> empty_chapters;
        const std::array limits{INT32_MIN,chapters.front().threshold,chapters.front().threshold+1,INT32_MAX};
        std::size_t chapter_count{};
        for(const auto limit:limits){
            *reinterpret_cast<std::int32_t*>(game.GlobalAddress(0x90dec0u))=limit;
            binding.PopulateChapters();
            std::vector<pal4::NumberedGameRecord> expected;
            for(const auto& chapter:chapters)if(chapter.threshold<limit)expected.push_back(chapter);
            const auto story_count=Export<std::uint32_t (__thiscall*)(const void*)>(module,
                "?getRowCount@MultiColumnList@CEGUI@@QBEIXZ")(Ptr(story));
            Check(count()==expected.size() && story_count==expected.size() && binding.OwnedItemCount()==2u*expected.size(),
                "strict original Story limit yields paired native title/rich rows");
            for(std::size_t i=0;i<expected.size();++i){
                const std::array<std::uint32_t,2> grid{static_cast<std::uint32_t>(i),0u};
                for(const auto target:{list,story}){
                    const auto item=Export<void* (__thiscall*)(const void*,const void*)>(module,
                        "?getItemAtGridReference@MultiColumnList@CEGUI@@QBEPAVListboxItem@2@ABUMCLGridRef@2@@Z")(Ptr(target),grid.data());
                    Check(item!=nullptr,"paired chapter DLL item exists");
                    Check(Export<std::uint32_t (__thiscall*)(const void*)>(module,"?getID@ListboxItem@CEGUI@@QBEIXZ")(item)==
                        static_cast<std::uint32_t>(expected[i].threshold),"chapter title/rich item IDs stay aligned");
                    const auto text=Export<const void* (__thiscall*)(const void*)>(module,"?getText@ListboxItem@CEGUI@@QBEABVString@2@XZ")(item);
                    const auto utf8=Export<const char* (__thiscall*)(const void*)>(module,"?c_str@String@CEGUI@@QBEPBDXZ")(text);
                    Check(pal4::ConvertGameTextToUtf8(target==list?expected[i].first_text:expected[i].second_text)==std::optional<std::string>(utf8),
                        "actual DLL chapter title/description text matches original StoryDesc");
                }
            }
            const auto pixels=render();
            if(limit==INT32_MIN)empty_chapters=pixels;
            if(limit==INT32_MAX){
                const auto changed=std::inner_product(pixels.begin(),pixels.end(),empty_chapters.begin(),std::size_t{},
                    std::plus<>(),[](auto a,auto b){return a!=b?1u:0u;});
                Check(!expected.empty() && changed>20u,"actual RichTextFrame parser/layout and item rendering reach the GPU");
                chapter_count=expected.size();
                std::cout<<"Chapter title/rich pairs="<<expected.size()<<" changed raster bytes="<<changed<<'\n';
            }
        }
        binding.Populate();Check(state.bytes[0x88u]==1u && binding.OwnedItemCount()==count()+chapter_count,
            "return to task tab preserves hidden original chapter rows until replacement");
        binding.PopulateChapters();Check(binding.OwnedItemCount()==chapter_count*2u,"chapter reentry retires both former item owners");
        const auto story_hover=[&]{return Export<std::int32_t (__thiscall*)(void*)>(module,
            "?getHoverRowIndex@MultiColumnList@CEGUI@@QAEHXZ")(Ptr(story));};
        const auto main_hover=[&]{return Export<std::int32_t (__thiscall*)(void*)>(module,
            "?getHoverRowIndex@MultiColumnList@CEGUI@@QAEHXZ")(Ptr(list));};
        const auto story_bar=*reinterpret_cast<const std::uint32_t*>(story+1204u);
        const auto story_position=[&]{return Export<float (__thiscall*)(const void*)>(module,
            "?getScrollPosition@Scrollbar@CEGUI@@QBEMXZ")(Ptr(story_bar));};
        for(std::size_t row=0;row<chapter_count;++row){
            const auto before=detail_updates;select(static_cast<std::int32_t>(row));
            Check(story_hover()==row && detail_updates>before,"DLL title hover automatically synchronizes matching rich chapter");
            if(row==0u || row==chapter_count/2u || row+1u==chapter_count)(void)render();
        }
        const auto member=reinterpret_cast<std::uint32_t>(state.bytes.data());
        const auto chapter_key=[&](std::uint32_t code){
            std::array<std::uint32_t,5> event{};event[4]=code;
            return activity.DispatchKeyDown(activity.GetLast(),reinterpret_cast<std::uint32_t>(event.data()));};
        select(0);(void)activity.Insert(member+0x40u);
        Check(chapter_key(208u)==1u && main_hover()==1 && story_hover()==1,"chapter Down routes through actual input and synchronizes body");
        Check(chapter_key(28u)==1u && activity.GetLast()==member+0x48u,"chapter Enter selects populated rich-body scroll input");
        const auto before_scroll=story_position();
        Check(before_scroll>=15.0f && chapter_key(200u)==1u && story_position()==before_scroll-15.0f,
            "real populated chapter scrollbar moves up15");
        Check(chapter_key(208u)==1u && story_position()==before_scroll,"real populated chapter scrollbar moves back down15");
        Check(chapter_key(1u)==1u && activity.GetLast()==member+0x40u,"chapter Escape restores title-list input");
        const auto body_click=[&]{
            pal4::CeguiMouseEvent event{};event.window=story;
            event.vtable=reinterpret_cast<std::uint32_t>(Export<const void*>(module,"??_7MouseEventArgs@CEGUI@@6B@"));
            event.position=pal4::NativeCeguiWindowPointerEvents(module).AbsolutePosition(story);
            event.position.x+=20.0f;event.position.y+=40.0f;
            Export<void (__thiscall*)(void*,void*)>(module,"?onMouseClicked@MultiColumnList@CEGUI@@MAEXAAVMouseEventArgs@2@@Z")(Ptr(story),&event);
            events.RethrowEventError();};
        Export<void (__thiscall*)(void*)>(module,"?clearAllSelections@MultiColumnList@CEGUI@@QAEXXZ")(Ptr(story));
        const auto before_empty_click=main_hover();body_click();
        Check(main_hover()==before_empty_click,"unselected real chapter body click leaves title selection unchanged");
        const std::array<std::uint32_t,2> chosen{5u,0u};
        Export<void (__thiscall*)(void*,const void*,bool)>(module,
            "?setItemSelectState@MultiColumnList@CEGUI@@QAEXABUMCLGridRef@2@_N@Z")(Ptr(story),chosen.data(),true);
        select(0);
        Export<void (__thiscall*)(void*,std::uint32_t)>(module,"?scrollRowToTheListTop@MultiColumnList@CEGUI@@QAEXI@Z")(Ptr(story),chosen[0]);
        Export<void (__thiscall*)(void*,float)>(module,"?setScrollPosition@Scrollbar@CEGUI@@QAEXM@Z")(Ptr(story_bar),story_position()+17.0f);
        const auto preserved=story_position();
        const auto before_clicks=story_clicks;
        body_click();
        Check(story_clicks==before_clicks+1u && main_hover()==chosen[0] && story_hover()==chosen[0] && story_position()==preserved,
            "actual DLL body click reverses title selection and restores body scroll after synchronous hover callbacks");
        std::cout<<"Chapter synchronization: "<<chapter_count<<" actual title hover updates, populated scroll keys and reverse DLL click preserve position\n";
        events.Close();Check(!events.OwnsActivity(member+0x40u) && activity.Size()==0u,"close removes borrowed chapter activity ownership");
        const auto retired_updates=detail_updates,retired_clicks=story_clicks;select(3);body_click();
        Check(detail_updates==retired_updates && story_clicks==retired_clicks,"retired title/body subscribers stop calling host endpoints");
        Export<void (__thiscall*)(void*,bool)>(module,"?setVisible@Window@CEGUI@@QAEX_N@Z")(Ptr(list),true);
    }
    Check(count()==0u,"binding destructor resets borrowed list before releasing items");
    Check(Export<std::uint32_t (__thiscall*)(const void*)>(module,"?getRowCount@MultiColumnList@CEGUI@@QBEIXZ")(
        Ptr(state.Read(0x58u)))==0u,"binding destructor also detaches all native RichTextFrame item owners");
    Check(ui.BindDialogRootParent(state.Read(0x24u),false),ui.LastError());
    // Retire the earlier borrowed fixture before loading the persistent owner
    // under the original names. Repeated owner lifetimes must release them.
    Export<void (__thiscall*)(void*,void*)>(module,
        "?destroyWindow@WindowManager@CEGUI@@QAEXPAVWindow@2@@Z")(layout.manager,layout.root);
    layout.root=nullptr;
    for(const auto pointer:stories)*reinterpret_cast<std::int32_t*>(pointer)=INT32_MAX;
    *reinterpret_cast<std::int32_t*>(game.GlobalAddress(0x90dec0u))=INT32_MAX;
    for(unsigned lifetime=0;lifetime<2u;++lifetime){
        std::cout<<"Persistent Mission lifetime="<<lifetime<<" construct\n";
        pal4::NativeMissionWindow* owner{};
        pal4::NativeEventHandlerArray activity(
            [&](auto member,auto slot){Check(owner && owner->OwnsActivity(member),"persistent Mission callback owner");
                return owner->DispatchActiveCallback(member,slot);},
            [&](auto member,auto slot,auto event){Check(owner && owner->OwnsActivity(member),"persistent Mission input owner");
                return owner->DispatchActivePredicate(member,slot,event);});
        std::vector<std::array<std::uint32_t,3>> notifications;
        unsigned tutorials{};
        pal4::NativeMissionWindowServices services;
        services.event=[&](auto key,auto first,auto second){notifications.push_back({key,first,second});};
        services.tutorial=[&](auto key){Check(key==6u,"Mission tutorial6");++tutorials;};
        pal4::NativeMissionWindow persistent(ui,root,database,game,activity,0x1000u,std::move(services));
        owner=&persistent;
        std::cout<<"Persistent Mission constructed\n";
        const auto& image=persistent.State();
        const auto address=persistent.FrameAddress();
        const auto row_count=[&](std::uint32_t w){return Export<std::uint32_t (__thiscall*)(const void*)>(module,
            "?getRowCount@MultiColumnList@CEGUI@@QBEIXZ")(Ptr(w));};
        const auto key=[&](std::uint32_t code){std::array<std::uint32_t,5> event{};event[4]=code;
            const auto address=reinterpret_cast<std::uint32_t>(event.data());
            return activity.Contains(address)?1u:activity.DispatchKeyDown(activity.GetLast(),address);};
        persistent.SetVisible(true);
        std::cout<<"Persistent Mission visible\n";
        Check(tutorials==1u && activity.Size()==2u && activity.GetLast()==address+0x40u,
            "persistent Mission show inserts navigation and task input");
        Check(image.bytes[0x88u]==1u && row_count(image.Read(0x54u))==28u,
            "persistent Mission show populates real task rows");
        Check(notifications.size()==3u && notifications[0]==std::array<std::uint32_t,3>{0x8b1394u,1u,0u} &&
            notifications[1]==std::array<std::uint32_t,3>{0x8b13a0u,1u,image.Read(0x24u)} &&
            notifications[2]==std::array<std::uint32_t,3>{0x8b13a4u,12u,0u},"original Mission visibility event sequence");
        Check(key(1u)==1u && activity.GetLast()==address+0x28u,"task Escape returns to native tab navigation");
        Check(key(208u)==1u && image.Read(0x3cu)==1u,"native tabs Down selects chapter button");
        Check(key(28u)==1u && image.bytes[0x88u]==0u && row_count(image.Read(0x58u))==128u &&
            activity.GetLast()==address+0x40u,"tab Enter fires real DLL button and populates chapters");
        Check(key(28u)==1u && activity.GetLast()==address+0x48u,"persistent chapter Enter selects body input");
        Check(key(1u)==1u && activity.GetLast()==address+0x40u,"body Escape returns to title input");
        Check(key(1u)==1u && activity.GetLast()==address+0x28u,"title Escape returns to tab input");
        Check(key(2u)==1u && image.bytes[0x88u]==1u && activity.GetLast()==address+0x40u,
            "native tab shortcut1 opens mission list");
        Check(key(1u)==1u && key(3u)==1u && image.bytes[0x88u]==0u,
            "native tab shortcut2 opens chapter list");
        Check(frame.Begin() && ui.Render(),"persistent Mission renders real window");
        Check(key(1u)==1u && key(1u)==1u && activity.Size()==0u &&
            notifications.back()==std::array<std::uint32_t,3>{0x8a80acu,1u,0u},
            "tab Escape hides Mission then restores toolbar");
        persistent.SetVisible(true);Check(activity.Size()==2u && tutorials==2u && image.bytes[0x88u]==1u,
            "persistent Mission reopens task mode without recreating layout");
        persistent.RethrowEventError();persistent.Close();persistent.Close();
        Check(activity.Size()==0u && !persistent.OwnsActivity(address+0x28u),"persistent Mission close removes all activity");
    }
    std::cout<<"Persistent Mission: repeated lifecycle, real tab buttons, navigation, shortcuts, visibility and close pass\n";
    std::cout<<"PASS: real Mission layout/list/DLL items, Chinese text, live Story refresh and teardown\n";
}
}
int main(int argc,char** argv)try{
    std::cout<<std::unitbuf;
    Check(argc==2,"provide original game directory");pal4::test::NativeDesktopObservation desktop;
    Run(argv[1]);desktop.Check("Mission native list");return 0;
}catch(const std::exception& error){std::cerr<<"FAIL: "<<error.what()<<'\n';return 1;}
