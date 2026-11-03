#include "support/native_system_property_probe.h"
#include "pal4/native_shop_selector_layout.h"
#include "pal4/native_cegui_ui_session.h"
#include "pal4/native_cegui_text_conversion.h"
#include "pal4/native_cegui_window_pointer_events.h"
#include "pal4/native_game_database.h"
#include "pal4/native_shop_record_catalog.h"
#include "pal4/native_shop_selector_availability.h"
#include "pal4/native_trade_system_owner.h"
#include "pal4/native_pal_game_iv_texture_storage.h"
#include "pal4/native_shop_selector_input.h"
#include "pal4/native_event_handler_array.h"
#include "pal4/shop_ui_frame.h"
#include "../native/ui/cegui_exports_native.h"
#include <rwcore.h>
#include <array>
#include <algorithm>
#include <bit>
#include <cmath>
#include <iostream>
#include <vector>
#include <map>
namespace pal4::test {
namespace {
void VerifyShopCatalog(const NativeGameDatabase& database){
    const auto check=[](bool ok,const char* why){if(!ok)throw std::runtime_error(why);};
    NativeShopRecordCatalog catalog(database);
    const auto& shops=database.Tables().at("ShopProperty");const auto& goods=database.Tables().at("Good");
    const auto column=[](const NativeGameDbTable& t,std::string_view name){
        for(std::size_t i=0;i<t.fields.size();++i)if(t.fields[i].name==name)return i;
        throw std::runtime_error("Missing original shop fixture field");
    };
    check(catalog.Shops().size()==shops.rows.size() && catalog.Goods().size()==goods.rows.size() && !catalog.Shops().empty() && !catalog.Goods().empty(),
        "native shop catalog owns every original ShopProperty and Good record");
    const auto shop_id=column(shops,"ID"),shop_type=column(shops,"TYPE"),good_shop=column(goods,"SHOP_ID"),good_type=column(goods,"GOODS_TYPE");
    std::map<std::string,std::array<bool,3>> categories;
    for(std::size_t i=0;i<goods.rows.size();++i){const auto& row=goods.rows[i];const auto& record=catalog.Goods()[i];
        const auto id=std::get<std::string>(row[good_shop]);const auto type=std::get<unsigned>(row[good_type]);
        check(type<3u,"original Good categories0..2");categories[id][type]=true;
        for(const auto field:std::array<std::pair<const char*,unsigned>,5>{{{"ID",0u},{"GOODS_TYPE",1u},{"GOODS_ID",2u},{"OPEN_CONDITION",3u},{"OPEN_NUM",4u}}})
            check(record[field.second]==std::get<unsigned>(row[column(goods,field.first)]),"native Good scalar fields preserve actual database values");
        check(catalog.Text(reinterpret_cast<unsigned>(&record)+20u)==id,"native Good text preserves original SHOP_ID");
    }
    unsigned smith_allowed{};
    for(std::size_t i=0;i<shops.rows.size();++i){const auto& row=shops.rows[i];const auto& record=catalog.Shops()[i];
        const auto id=std::get<std::string>(row[shop_id]);const auto type=std::get<unsigned>(row[shop_type]);
        check(catalog.FindShop(id)==reinterpret_cast<unsigned>(&record) && record[13]==type && catalog.AllowsSmith(id)==(type==0u),
            "existing CRC lookup resolves every original ShopProperty and exact Smith TYPE0 gate");
        for(const auto field:std::array<std::pair<const char*,unsigned>,3>{{{"ID",4u},{"NAME",20u},{"POSITION",36u}}})
            check(catalog.Text(reinterpret_cast<unsigned>(&record)+field.second)==std::get<std::string>(row[column(shops,field.first)]),"all native ShopProperty text fields preserve original bytes");
        for(unsigned category=0;category<3u;++category)check(catalog.HasGoods(id,category)==categories[id][category],"shared TradeItemFinder matches actual Good categories for every original shop");
        check(!catalog.HasGoods(id,3u),"out-of-range goods category has no original records");smith_allowed+=type==0u;
    }
    check(!catalog.FindShop("__PAL4_missing_shop__") && !catalog.AllowsSmith("__PAL4_missing_shop__") && !catalog.HasGoods("__PAL4_missing_shop__",0u),"unknown shop has no catalog/Smith/category match");
    // Original selector tests category presence, independently of OPEN_* fields.
    auto one_shop=shops,one_good=goods;one_shop.rows.resize(1u);one_good.rows.resize(1u);
    const std::string embedded("A\0B",3u);one_shop.rows[0][shop_id]=embedded;one_shop.rows[0][shop_type]=0u;
    one_good.rows[0][good_shop]=embedded;one_good.rows[0][good_type]=2u;
    one_good.rows[0][column(goods,"OPEN_CONDITION")]=1u;one_good.rows[0][column(goods,"OPEN_NUM")]=99u;
    NativeShopRecordCatalog edge(one_shop,one_good);
    check(edge.FindShop(embedded) && !edge.FindShop("A") && edge.AllowsSmith(embedded) && edge.HasGoods(embedded,2u) && !edge.HasGoods(embedded,0u),
        "CRC/category adapters preserve embedded NUL length and original category-presence rule despite open conditions");
    one_shop.rows[0][shop_type]=4u;bool bad_shop{};try{NativeShopRecordCatalog bad(one_shop,one_good);}catch(const std::runtime_error&){bad_shop=true;}
    one_shop.rows[0][shop_type]=3u;one_good.rows[0][good_type]=3u;bool bad_good{};try{NativeShopRecordCatalog bad(one_shop,one_good);}catch(const std::runtime_error&){bad_good=true;}
    check(bad_shop && bad_good,"catalog enforces original ShopProperty0..3 and Good0..2 validation");
    one_good.rows[0][good_type]=0u;NativeShopRecordCatalog type_three(one_shop,one_good);
    check(type_three.FindShop(embedded) && !type_three.AllowsSmith(embedded),"valid TYPE3 shop remains excluded from Smith");
    std::cout<<"PASS: Native ShopProperty="<<catalog.Shops().size()<<" Good="<<catalog.Goods().size()<<" smith_type0="<<smith_allowed<<" original records/CRC/category gates verified\n";
}
}
void VerifyNativeShopSelectorLayout(const std::filesystem::path& root,
    NativeCeguiUiSession& ui,const NativeGameDatabase& database,NativePalGameIVTextureStorage& game,RwCamera* camera){
    NativeGameDatabase shop_database;
    if(!shop_database.Load(root,"shopdb.db"))throw std::runtime_error(std::string(shop_database.LastError()));
    VerifyShopCatalog(shop_database);
    NativeShopRecordCatalog catalog(shop_database);NativeTradeSystemOwner trade;
    if(!trade.Construct())throw std::runtime_error("Shop availability trade owner");trade.State()=0u;
    struct RestorePermission{NativePalGameIVTextureStorage& game;unsigned value;~RestorePermission(){game.SetGameDataEntry(15,value);}} restore_permission{game,game.GameDataEntry(15)};
    const auto check=[](bool ok,const char* why){if(!ok)throw std::runtime_error(why);};
    const auto module=GetModuleHandleA("CEGUIBase.dll");
    constexpr std::array<unsigned,8> methods{0x4864f0u,0x486500u,0x486510u,0x486520u,0x486530u,0x486730u,0x486570u,0x486540u};
    constexpr std::array<unsigned,8> titles{147u,148u,145u,146u,149u,258u,189u,30u};
    const auto text_of=[&](unsigned window){
        const auto text=detail::Export<const void* (__thiscall*)(const void*)>(module,"?getText@Window@CEGUI@@QBEABVString@2@XZ")(reinterpret_cast<void*>(window));
        return std::string(detail::Export<const char* (__thiscall*)(const void*)>(module,"?c_str@String@CEGUI@@QBEPBDXZ")(text));
    };
    const auto parent=detail::Export<void* (__thiscall*)(const void*)>(module,"?getParent@Window@CEGUI@@QBEPAV12@XZ");
    for(unsigned cycle=0;cycle<2u;++cycle){
        std::vector<unsigned> invoked;NativeShopSelectorLayout* live{};bool refuse_retirement{},throw_callback{};
        std::unique_ptr<NativeShopSelectorInput> input;unsigned closed_by_key{};
        std::array<unsigned,0x224u/4u> manager{};
        NativeEventHandlerArray activity([&](auto member,auto slot){check(input && input->OwnsActivity(member),"shop global callback router owns navigation");return input->Callback(member,slot);},
            [&](auto member,auto slot,auto event){check(input && input->OwnsActivity(member),"shop global predicate router owns navigation");return input->Predicate(member,slot,event);});
        NativeShopSelectorLayout layout(ui,root,database,[&](auto method,auto event){
            check(live && live->InCallback() && event,"original shop DLL callback preserves host owner/event");
            if(method==0x4af760u || method==0x4af770u)return input?input->Mouse(method,event):true;
            const auto source=*reinterpret_cast<const unsigned*>(event+8u);
            bool matched{};for(unsigned i=0;i<8u;++i)if(methods[i]==method)matched=source==live->Word(0x40u+i*4u);
            check(matched,"shop callback method and original event source agree");
            if(refuse_retirement)live->Close();
            if(throw_callback)throw std::runtime_error("shop selector callback probe");
            invoked.push_back(method);return true;
        });live=&layout;
        check(layout.Word(0u)==ShopUiFrame::kFrameVtable && layout.Word(0x28u)==ShopUiFrame::kMouseFinalVtable &&
            !layout.Word(0x30u) && !layout.Word(0x34u),"native shop layout preserves original frame/mouse identities with empty navigation");
        layout.SetShopId("Q01-shop");layout.SetTaskScript("ShopTask");
        check(layout.ShopId()=="Q01-shop" && layout.TaskScript()=="ShopTask" &&
            std::string(reinterpret_cast<const char*>(layout.Word(0x64u)))==layout.ShopId() && layout.Word(0x68u)==8u &&
            std::string(reinterpret_cast<const char*>(layout.Word(0x74u)))==layout.TaskScript() && layout.Word(0x78u)==8u,
            "shop/task native std::string storage feeds existing frame text views");
        layout.SetShopId(std::string(300u,'x'));layout.SetTaskScript("");
        check(layout.Word(0x68u)==300u && !layout.Word(0x78u) && *reinterpret_cast<const char*>(layout.Word(0x74u))=='\0',
            "shop text views refresh after host reallocation and empty assignment");
        layout.BindParent(true);check(parent(reinterpret_cast<void*>(layout.RootAddress())),"actual shop root attaches to desktop");
        for(unsigned i=0;i<8u;++i){
            // The original no-argument getter uses relative metrics. Buttons
            // belong to the XML's141x291 selector frame, not PanelButton.
            check(layout.Word(0x40u+i*4u) && std::abs(std::bit_cast<float>(layout.Word(0x80u+i*8u))-7.f/141.f)<0.000001f &&
                std::abs(std::bit_cast<float>(layout.Word(0x84u+i*8u))-(32.f+28.f*i)/291.f)<0.000001f,
                "original XML button slot positions retain DLL relative metrics before availability packing");
            const auto title=database.FindTableTextById("GameString",titles[i]);
            const auto converted=title?ConvertGameTextToUtf8(*title):std::nullopt;
            check(converted && text_of(layout.Word(0x40u+i*4u))==*converted,"all original shop button labels use actual GameString and native encoding conversion");
            (void)NativeCeguiWindowPointerEvents(module).HandleMouseAtCenter(layout.Word(0x40u+i*4u));layout.RethrowEventError();
            check(invoked.size()==i+1u && invoked.back()==methods[i],"all eight original DLL Clicked events reach original method IDs once");
        }
        RwRGBA black{0,0,0,255};check(RwCameraClear(camera,&black,rwCAMERACLEARIMAGE|rwCAMERACLEARZ) && RwCameraBeginUpdate(camera),"shop layout render begin");
        const auto rendered=ui.Render();(void)RwCameraEndUpdate(camera);check(rendered,"actual DLL shop layout render call");
        input=std::make_unique<NativeShopSelectorInput>(layout,activity,reinterpret_cast<unsigned>(manager.data()),[&]{
            ++closed_by_key;layout.BindParent(false);(void)activity.Remove(layout.FrameAddress()+0x28u);
        });
        {
            NativeShopSelectorAvailability availability(layout,catalog,trade,game);
            const auto height=detail::Export<float (__thiscall*)(const void*)>(module,"?getHeight@Window@CEGUI@@QBEMXZ");
            const auto y_position=detail::Export<float (__thiscall*)(const void*)>(module,"?getYPosition@Window@CEGUI@@QBEMXZ");
            const auto visible=detail::Export<bool (__thiscall*)(const void*,bool)>(module,"?isVisible@Window@CEGUI@@QBE_N_N@Z");
            const auto button_height=height(reinterpret_cast<void*>(layout.Word(0x40u)));
            const auto initial_height=height(reinterpret_cast<void*>(layout.Word(0xc0u))),initial_y=y_position(reinterpret_cast<void*>(layout.Word(0xc4u)));
            for(const auto& shop:catalog.Shops()){
                const auto id=catalog.Text(reinterpret_cast<unsigned>(&shop)+4u);layout.SetShopId(id);
                for(unsigned mode=0;mode<4u;++mode){
                    const bool permission=(mode&1u)!=0u,task=(mode&2u)!=0u;
                    game.SetGameDataEntry(15,permission?1u:0u);layout.SetTaskScript(task?"ShopTask":"");availability.Refresh();layout.RethrowEventError();
                    std::vector<unsigned> expected;
                    if(catalog.HasGoods(id,0u)){expected.push_back(layout.Word(0x40u));expected.push_back(layout.Word(0x44u));}
                    if(catalog.HasGoods(id,1u)){expected.push_back(layout.Word(0x48u));expected.push_back(layout.Word(0x4cu));}
                    if(catalog.HasGoods(id,2u))expected.push_back(layout.Word(0x50u));
                    if(task)expected.push_back(layout.Word(0x58u));
                    if(permission && catalog.AllowsSmith(id))expected.push_back(layout.Word(0x54u));
                    expected.push_back(layout.Word(0x5cu));
                    for(unsigned offset=0x40u;offset<0x60u;offset+=4u){const auto w=layout.Word(offset);
                        check(visible(reinterpret_cast<void*>(w),false)==(std::find(expected.begin(),expected.end(),w)!=expected.end()),"actual DLL shop button visibility follows original goods/task/permission/type gates");}
                    check((layout.Word(0x34u)-layout.Word(0x30u))/4u==expected.size(),"shop refresh replaces active navigation range without accumulating old rows");
                    for(unsigned i=0;i<expected.size();++i){
                        check(reinterpret_cast<const unsigned*>(layout.Word(0x30u))[i]==expected[i],"shop active navigation follows original packed button order");
                        std::array<unsigned,2> p{};detail::Export<const void* (__thiscall*)(const void*,void*)>(module,"?getPosition@Window@CEGUI@@QBE?AVVector2@2@XZ")(reinterpret_cast<void*>(expected[i]),p.data());
                        check(p[0]==layout.Word(0x80u+i*8u) && p[1]==layout.Word(0x84u+i*8u),"actual shop controls reuse captured original relative position slots");
                    }
                    const auto expected_height=button_height*static_cast<float>(expected.size());
                    check(std::abs(height(reinterpret_cast<void*>(layout.Word(0xc0u)))-expected_height)<0.00001f &&
                        std::abs(y_position(reinterpret_cast<void*>(layout.Word(0xc4u)))-(initial_y+expected_height-initial_height))<0.00001f,
                        "shop panel and bottom art resize cumulatively without drifting across refreshes");
                }
            }
            layout.SetShopId("__PAL4_missing_shop__");layout.SetTaskScript("");availability.Refresh();
            check(layout.Word(0x34u)-layout.Word(0x30u)==4u && *reinterpret_cast<const unsigned*>(layout.Word(0x30u))==layout.Word(0x5cu),"missing shop safely packs only original exit button");
            std::string navigation_shop;
            for(const auto& shop:catalog.Shops()){const auto id=catalog.Text(reinterpret_cast<unsigned>(&shop)+4u);
                if(catalog.HasGoods(id,0u) || catalog.HasGoods(id,1u)){navigation_shop=id;break;}}
            check(!navigation_shop.empty(),"original shop with multiple navigation entries");
            layout.SetShopId(navigation_shop);layout.SetTaskScript("ShopTask");availability.Refresh();
            const auto member=layout.FrameAddress()+0x28u;
            (void)activity.Insert(member);activity.Update();
            const auto count=(layout.Word(0x34u)-layout.Word(0x30u))/4u;
            check(count>=4u && input->OwnsActivity(member) && !input->OwnsActivity(member+4u),"native global activity contains actual shop navigation owner");
            const auto button=[&](unsigned index){return reinterpret_cast<const unsigned*>(layout.Word(0x30u))[index];};
            const auto key=[&](unsigned code){std::array<unsigned,5> event{};event[4]=code;const auto e=reinterpret_cast<unsigned>(event.data());
                const auto handled=activity.Contains(e)?1u:activity.DispatchKeyDown(activity.GetLast(),e);layout.RethrowEventError();return handled;};
            check(!layout.Word(0x3cu) && key(208u) && layout.Word(0x3cu)==1u && key(200u) && !layout.Word(0x3cu) &&
                key(200u) && layout.Word(0x3cu)==count-1u && key(208u) && !layout.Word(0x3cu),
                "original global input cycles shop vertically and wraps both ends");
            check(!key(203u) && !key(205u) && !key(57u) && !layout.Word(0x3cu),"shop ignores horizontal and unrelated keys without moving selection");
            const auto enable=detail::Export<void (__thiscall*)(void*,bool)>(module,"?setEnabled@Window@CEGUI@@QAEX_N@Z");
            const auto show=detail::Export<void (__thiscall*)(void*,bool)>(module,"?setVisible@Window@CEGUI@@QAEX_N@Z");
            for(unsigned mode=0;mode<3u;++mode){
                layout.Word(0x3cu)=0u;activity.Update();const auto skipped=button(1u);const auto old_lock=*reinterpret_cast<const std::uint8_t*>(skipped+1202u);
                if(mode==0u)enable(reinterpret_cast<void*>(skipped),false);
                if(mode==1u)show(reinterpret_cast<void*>(skipped),false);
                if(mode==2u)*reinterpret_cast<std::uint8_t*>(skipped+1202u)=1u;
                const auto handled=key(208u);enable(reinterpret_cast<void*>(skipped),true);show(reinterpret_cast<void*>(skipped),true);
                *reinterpret_cast<std::uint8_t*>(skipped+1202u)=old_lock;
                check(handled && layout.Word(0x3cu)==2u,"shared shop navigation skips disabled/hidden/locked original DLL buttons");
            }
            const auto before_enter=invoked.size();check(key(28u) && invoked.size()==before_enter+1u && layout.Word(0x3cu)==2u,
                "shop Enter dispatches exactly one real DLL Clicked callback and restores nested mouse/key event scopes");
            (void)NativeCeguiWindowPointerEvents(module).HandleMouseAtCenter(button(count-1u));layout.RethrowEventError();
            check(layout.Word(0x3cu)==count-1u,"actual original MouseButtonUp synchronizes selected shop index");
            check(key(1u) && closed_by_key==1u && !activity.Size() && !parent(reinterpret_cast<void*>(layout.RootAddress())),
                "original shop Esc calls bound close action, detaches actual root and removes global navigation");
            layout.BindParent(true);(void)activity.Insert(member);activity.Update();
        }
        refuse_retirement=true;(void)NativeCeguiWindowPointerEvents(module).HandleMouseAtCenter(layout.Word(0x5cu));
        bool refused{};try{layout.RethrowEventError();}catch(const std::logic_error&){refused=true;}refuse_retirement=false;
        check(refused && layout.RootAddress() && !layout.InCallback(),"shop callback cannot destroy its live layout and deferred error unwinds depth");
        throw_callback=true;(void)NativeCeguiWindowPointerEvents(module).HandleMouseAtCenter(layout.Word(0x5cu));
        bool reported{};try{layout.RethrowEventError();}catch(const std::runtime_error& e){reported=std::string_view(e.what())=="shop selector callback probe";}throw_callback=false;
        check(reported && !layout.InCallback(),"shop exceptions cross DLL callback boundary through deferred delivery");
        layout.RethrowEventError();layout.BindParent(false);
        check(!parent(reinterpret_cast<void*>(layout.RootAddress())),"actual shop hide detaches root from desktop");
        layout.BindParent(true);(void)activity.Remove(layout.FrameAddress()+0x28u);input.reset();activity.ReleaseStorage();
        check(!activity.OutstandingAllocations(),"shop global activity storage retires before borrowed layout");
        layout.Close();layout.Close();check(!layout.RootAddress(),"shop layout retires callbacks/windows and permits repeated close");
        bool closed{};try{layout.BindParent(true);}catch(const std::logic_error&){closed=true;}
        check(closed,"retired shop layout rejects new attachment");
    }
    std::cout<<"PASS: Original TradeSelectWindow layout/availability and native global input,vertical wrap/skip,Enter Clicked,mouse selection,Esc close and retirement verified; transaction/Smith actions still pending\n";
}
}
