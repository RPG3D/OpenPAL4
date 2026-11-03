#include "pal4/native_shop_task_script.h"
#include "pal4/native_timed_script_stack.h"
#include "pal4/native_angelscript_engine.h"
#include "pal4/native_pk_file_manager_runtime.h"
#include <cstring>
#include "pal4/shop_ui_actions.h"
#include "support/native_system_property_probe.h"
#include "pal4/native_shop_selector_window.h"
#include "pal4/native_shop_selector_layout.h"
#include "pal4/native_shop_record_catalog.h"
#include "pal4/native_trade_system_owner.h"
#include "pal4/native_pal_game_iv_texture_storage.h"
#include "pal4/native_arena_load_runtime.h"
#include "pal4/native_event_handler_array.h"
#include "pal4/native_game_database.h"
#include "pal4/native_cegui_window_pointer_events.h"
#include "pal4/trade_system_ui.h"
#include <array>
#include <iostream>
#include <vector>
namespace pal4::test {
namespace {
void Check(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
unsigned Addr(const void* p){return reinterpret_cast<unsigned>(p);}
// Own real native temporary configs. State entry is the explicit recorded
// boundary; actual registered state dispatch has its separate native fixture.
struct SelectorXml final : IGameArenaLoadRuntime {
    NativeArenaLoadRuntime& native;IGameArenaLoadRuntime& xml;NativeTradeSystemOwner& trade;
    unsigned smith_entries{};bool fail{};
    SelectorXml(NativeArenaLoadRuntime& a,NativeTradeSystemOwner& t):native(a),xml(a),trade(t){}
    unsigned ArenaManager()override{throw std::logic_error("unexpected shop XML arena");}
    unsigned ReadWord(unsigned)override{throw std::logic_error("unexpected shop XML raw word");}
    void WriteWord(unsigned,unsigned)override{throw std::logic_error("unexpected shop XML raw write");}
    unsigned InitializeXmlDocument()override{return xml.InitializeXmlDocument();}
    std::optional<unsigned> FindAttribute(unsigned d,std::string_view key)override{return xml.FindAttribute(d,key);}
    void SetInt(unsigned a,int v)override{xml.SetInt(a,v);}
    void SetString(unsigned a,std::string_view v)override{xml.SetString(a,v);}
    unsigned InitializeIntAttribute(std::string_view key,int v)override{return xml.InitializeIntAttribute(key,v);}
    unsigned InitializeStringAttribute(std::string_view key,std::string_view v)override{return xml.InitializeStringAttribute(key,v);}
    void AddAttribute(unsigned d,unsigned a)override{xml.AddAttribute(d,a);}
    void DestroyAttribute(unsigned a)override{xml.DestroyAttribute(a);}
    unsigned GameInstance()override{return xml.GameInstance();}
    void InitializeStateAndUi(unsigned game,unsigned state,unsigned document)override{
        Check(game==GameInstance() && state==5u && native.LiveDocumentCount()==1u && native.ReadConfigInteger(document,"Smith_OpenWithSystem")==0u && trade.State()==2u,
            "shop Smith enters5 only after original close/Scene/state2 and carries actual owned zero config");
        if(fail)throw std::runtime_error("shop fixture Smith entry failure");++smith_entries;
    }
    unsigned ActorControl()override{throw std::logic_error("unexpected shop actor control");}
    void ProcessActorInput(unsigned)override{throw std::logic_error("unexpected shop actor input");}
    void ProcessActorInputAndClearVector(unsigned)override{throw std::logic_error("unexpected shop actor vector");}
    unsigned InputManager()override{throw std::logic_error("unexpected shop input owner");}
    void InitializeInputState(unsigned)override{throw std::logic_error("unexpected shop input reset");}
    unsigned Milliseconds()override{throw std::logic_error("unexpected shop clock");}
    unsigned GameState()override{throw std::logic_error("unexpected shop render state");}
    std::int32_t DestroyXmlDocument(unsigned d)override{return xml.DestroyXmlDocument(d);}
};
struct SelectorTrade final : ITradeSystemUiRuntime {
    NativePalGameIVTextureStorage& game;NativeTradeSystemOwner& trade;const NativeShopRecordCatalog& catalog;
    NativeEventHandlerArray& activity;unsigned manager;NativeShopSelectorWindow* window{};
    bool music_text{};unsigned category{},buy{},opened{};std::vector<unsigned> states,events;std::string shop;
    SelectorTrade(NativePalGameIVTextureStorage& g,NativeTradeSystemOwner& t,const NativeShopRecordCatalog& c,NativeEventHandlerArray& a,unsigned m)
        :game(g),trade(t),catalog(c),activity(a),manager(m){}
    void CreateMusicText(std::string_view file)override{
        Check(file=="p37.mp3" && !music_text && window && !window->Visible() && !activity.Size(),"shop category hides/removes selector before original music path");music_text=true;
    }
    unsigned BackgroundMusicPlayer()override{return 0x1000u;}
    void FadeInSpecialMusic(unsigned p,float seconds,unsigned flags)override{Check(p==0x1000u && seconds==3.f && !flags && music_text,"original shop fade arguments");}
    void DestroyMusicText()override{Check(music_text,"shop temporary music text");music_text=false;}
    unsigned GameDatabaseInstance()override{return 0x4000u;}
    unsigned FindShopGoods(unsigned table,unsigned text)override{
        Check(table==0x406cu && window->Layout() && text==window->Layout()->FrameAddress()+0x60u && trade.State()==1u,"original shop table lookup after state1");
        shop=window->Layout()->ShopId();return catalog.FindShop(shop);
    }
    unsigned UIFrameManager()override{return manager;}
    std::int32_t ProcessEvent(unsigned tree,unsigned key,unsigned first,unsigned second)override{
        Check(tree==manager+260u,"shop event tree identity");events.push_back(key);
        if(key==TradeSystemUi::kEventShowSelector || key==TradeSystemUi::kEventShopId || key==TradeSystemUi::kEventTradeTask){
            Check(window && window->Event(key,first,second),"original TradeSystemUi event reaches actual selector owner");return 0x134;
        }
        Check(window && !window->Visible() && !activity.Size() && trade.State()==1u,"actual selector is hidden before Trade window events");
        if(key==TradeSystemUi::kEventTradeActive)Check(first==1u && !second,"shop active event");
        else if(key==TradeSystemUi::kEventTradeSelector){buy=first;Check(!second,"shop buy/sell selector payload");}
        else if(key==TradeSystemUi::kEventTradeGoods){category=first;Check(second==catalog.FindShop(shop),"shop goods event uses actual ShopProperty record");}
        else throw std::logic_error("unexpected selector trade event");
        return 1;
    }
    void ToggleWindows(unsigned m,bool visible)override{Check(m==manager && visible,"original shop edge visibility");++opened;}
    unsigned GameArenaManager()override{throw std::logic_error("unexpected selector close-shop arena");}
    unsigned ReadWord(unsigned)override{throw std::logic_error("unexpected selector trade word");}
    std::int32_t SetBackgroundMusic(unsigned,unsigned,unsigned)override{throw std::logic_error("unexpected selector music restore");}
    unsigned GameStateInstance()override{return game.Address();}
    std::int32_t InitializeGameState(unsigned g,unsigned state,unsigned argument)override{
        Check(g==game.Address() && (state==1u || state==4u) && !argument,"original selector state request");
        if(state==1u)Check(window && !window->Visible() && !activity.Size(),"selector hides before Scene transition request");
        states.push_back(state);return 0x1234;
    }
};
}
void VerifyNativeShopSelectorWindow(const std::filesystem::path& root,NativeCeguiUiSession& ui,
    const NativeGameDatabase& database,NativePalGameIVTextureStorage& game,NativeArenaLoadRuntime& native,NativePkFileManagerRuntime& packages){
    NativeGameDatabase shop_database;Check(shop_database.Load(root,"shopdb.db"),"selector owner shop database");NativeShopRecordCatalog catalog(shop_database);
    NativeTradeSystemOwner trade;Check(trade.Construct()!=0u,"selector owner trade system");
    std::array<unsigned,0x224u/4u> manager{};NativeShopSelectorWindow* window{};
    NativeEventHandlerArray activity([&](auto a,auto slot){Check(window && window->OwnsActivity(a),"selector owner callback route");return window->Callback(a,slot);},
        [&](auto a,auto slot,auto e){Check(window && window->OwnsActivity(a),"selector owner predicate route");return window->Predicate(a,slot,e);});
    SelectorXml xml(native,trade);SelectorTrade runtime(game,trade,catalog,activity,Addr(manager.data()));
    struct Directory{std::filesystem::path old=std::filesystem::current_path();~Directory(){std::filesystem::current_path(old);}} directory;
    std::filesystem::current_path(root);
    NativeAngelScriptEngine task_engine;unsigned task_loads{};
    const auto package_count=packages.LoadedPackageCount();
    NativeTimedScriptStack task_stack(task_engine,root,[&](auto source,auto module){
        Check(source=="Q01.csb" && module=="shop-script[ShopTask]" && window && !window->Visible() && trade.State()==2u,
            "real task DLL callback closes selector/trade before loading original task module key");
        ++task_loads;const std::string name(module);const char* text="void ShopTask(){global_mvar+=4;giWait(0.25f);global_mvar+=5;}";
        Check(task_engine.Raw()->AddScriptSection(name.c_str(),"selector task",text,static_cast<int>(std::strlen(text)))>=0 && task_engine.Raw()->Build(name.c_str())>=0,
            "real selector task builds actual SDK script context");
    });
    unsigned tutorials{},tasks{};bool retire_from_task{};NativeShopSelectorServices services;
    services.tutorial=[&](unsigned key){Check(window && key==8u && window->Visible() && activity.GetLast()==window->Layout()->FrameAddress()+0x28u,"original selector tutorial follows actual parent/activity insertion");++tutorials;};
    services.task=[&](unsigned frame,unsigned event,IShopUiActionsRuntime& actions,std::string_view task_name){Check(window && frame==window->Layout()->FrameAddress() && event,"selector task forwards original frame/event");
        Check(task_name=="ShopTask" && actions.TradeSystem()==trade.Address(),"selector borrows exact task/action owners");
        if(retire_from_task)window->Close();++tasks;bool handled{};
        task_stack.DispatchUiScript({},[]{},[&](auto& selected){handled=NativeShopTaskScript::Execute(event,
            {packages,selected,actions,static_cast<IGameArenaLoadRuntime&>(native).ArenaManager(),frame,"Q01.csb",task_name,
             [](auto,auto,auto){throw std::logic_error("unexpected missing selector task");}});});return handled;};
    NativeShopSelectorWindow owner(ui,root,database,catalog,trade,game,activity,Addr(manager.data()),runtime,xml,std::move(services));window=&owner;runtime.window=&owner;
    Check(!owner.Layout() && !owner.Event(0x8b0980u,0u,0u) && !owner.Layout() && !owner.Event(0xdeadbeefu,1u,0u),"hidden absent/unknown selector events preserve lazy construction");
    TradeSystemUi operations(trade.State());const std::string task="ShopTask";std::array<std::string,3> categories;std::string smith;
    for(const auto& row:catalog.Shops()){const auto id=catalog.Text(Addr(&row)+4u);
        for(unsigned category=0;category<3u;++category)if(categories[category].empty() && catalog.HasGoods(id,category))categories[category]=id;
        if(smith.empty() && catalog.AllowsSmith(id))smith=id;}
    Check(!smith.empty() && !categories[0].empty() && !categories[1].empty() && !categories[2].empty(),"original shops cover all five category actions and Smith");
    const auto open=[&](const std::string& id,const std::string& script){Check(operations.SetTradeItemInfo(Addr(id.c_str()),Addr(script.c_str()),runtime)==0x34u && owner.Visible() && activity.Size()==1u && !trade.State(),"existing SetTradeItemInfo routes3 events to lazy actual selector and preserves return/state0");};
    const auto click=[&](unsigned offset){(void)NativeCeguiWindowPointerEvents(GetModuleHandleA("CEGUIBase.dll")).HandleMouseAtCenter(owner.Layout()->Word(offset));owner.RethrowError();};
    unsigned retained{};
    for(const auto action:std::array<std::array<unsigned,3>,5>{{{0x40u,0u,1u},{0x44u,0u,0u},{0x48u,1u,1u},{0x4cu,1u,0u},{0x50u,2u,1u}}}){
        open(categories[action[1]],task);const auto frame=owner.Layout()->FrameAddress();Check(!retained || retained==frame,"selector retains original layout across category reopen");retained=frame;
        const auto before=runtime.opened;const auto events=runtime.events.size();click(action[0]);
        Check(!owner.Visible() && !activity.Size() && trade.State()==1u && runtime.category==action[1] && runtime.buy==action[2] && runtime.shop==categories[action[1]] && runtime.opened==before+1u && !runtime.music_text &&
            std::vector<unsigned>(runtime.events.begin()+events,runtime.events.end())==std::vector<unsigned>{TradeSystemUi::kEventTradeActive,TradeSystemUi::kEventTradeSelector,TradeSystemUi::kEventTradeGoods},
            "all five actual DLL category callbacks reuse original hide/music/state1/lookup/trade-event sequence");
    }
    open(categories[0],task);click(0x5cu);Check(!owner.Visible() && trade.State()==2u && runtime.states.back()==1u && !activity.Size(),"selector Exit reuses actual ShopUiActions and SetTradeState ordering");
    Check(operations.ClearTradeItemInfo(runtime)==0x34u && owner.Visible(),"ClearTradeItemInfo returns to same selector and retained shop text");
    std::array<unsigned,5> escape{};escape[4]=1u;Check(activity.DispatchKeyDown(activity.GetLast(),Addr(escape.data())) && !owner.Visible() && trade.State()==2u && runtime.states.back()==1u,"selector Esc shares exact close action with Exit");
    struct Restore{NativePalGameIVTextureStorage& game;unsigned value;~Restore(){game.SetGameDataEntry(15,value);}} restore{game,game.GameDataEntry(15)};game.SetGameDataEntry(15,1u);
    open(smith,"");const auto before_smith=runtime.states.size();click(0x54u);
    Check(!owner.Visible() && !activity.Size() && trade.State()==2u && runtime.states.size()==before_smith+1u && runtime.states.back()==1u && xml.smith_entries==1u && !native.LiveDocumentCount(),
        "actual Smith DLL callback reuses original close-to-Scene then config0/Foundry5 sequence with balanced native XML");
    xml.fail=true;open(smith,"");bool failed{};try{click(0x54u);}catch(const std::runtime_error& e){failed=std::string_view(e.what())=="shop fixture Smith entry failure";}xml.fail=false;
    Check(failed && !native.LiveDocumentCount() && !owner.Visible() && trade.State()==2u,"failed Smith entry releases real temporary XML and preserves original closed/Scene-request state");
    open(smith,task);retire_from_task=true;bool guarded{};try{click(0x58u);}catch(const std::logic_error&){guarded=true;}retire_from_task=false;
    Check(guarded && owner.Visible() && activity.Size()==1u,"selector owner refuses retirement from original DLL task callback");click(0x58u);
    Check(tasks==1u && task_loads==1u && !owner.Visible() && !activity.Size() && trade.State()==2u && task_stack.Depth()==1u && !task_engine.GlobalStoryVariable(),
        "actual DLL task button executes native close then queues exactly one real SDK context after deferred error recovery");
    Check(task_stack.ProcessSelected(0.f) && task_engine.GlobalStoryVariable()==4 && task_stack.Active() &&
        task_stack.ProcessSelected(0.25f) && task_engine.GlobalStoryVariable()==9 && !task_stack.Active() && packages.LoadedPackageCount()==package_count,
        "actual selector task SDK wait/resume/retirement balances original package scopes");
    Check(tutorials>=10u,"selector show consistently invokes original tutorial8 boundary");
    owner.Close();owner.Close();Check(!owner.Layout() && !owner.Visible() && !activity.Size() && !native.LiveDocumentCount(),"selector owner retires UI/input before borrowed state/catalog owners");activity.ReleaseStorage();
    bool closed{};try{(void)owner.Event(0x8b0980u,1u,0u);}catch(const std::logic_error&){closed=true;}Check(closed,"closed selector rejects future events");
    std::cout<<"PASS: Native selector lazy3-event routing,real DLL5 trade actions/Exit/Esc/Smith,existing TradeSystemUi and native XML cleanup verified; actual task SDK queue/wait/resume; state/music/TradeWindow endpoints recorded\n";
}
}
