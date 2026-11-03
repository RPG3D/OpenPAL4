#include "pal4/native_listbox_spinner_item.h"
#include "pal4/native_cegui_ui_session.h"
#include <rwcore.h>
#include "support/native_system_property_probe.h"
#include "pal4/native_trade_window_input.h"
#include "pal4/native_trade_window_repeat.h"
#include "pal4/native_game_timer_queue.h"
#include "pal4/native_trade_window_layout.h"
#include "pal4/native_event_handler_array.h"
#include "pal4/native_cegui_default_list_item.h"
#include "../native/ui/cegui_exports_native.h"
#include <array>
#include <map>
#include <memory>
#include <vector>
#include <iostream>
namespace pal4::test {
namespace {
void Check(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
unsigned Address(const void* p){return reinterpret_cast<unsigned>(p);}
}
void VerifyNativeTradeWindowInput(const std::filesystem::path& root,NativeCeguiUiSession& ui,RwCamera* camera){
    const auto module=GetModuleHandleA("CEGUIBase.dll"),look=GetModuleHandleA("OIRAMLOOK.dll");
    const auto set_visible=detail::Export<void (__thiscall*)(void*,bool)>(module,"?setVisible@Window@CEGUI@@QAEX_N@Z");
    const auto set_hover=detail::Export<void (__thiscall*)(void*,int)>(module,"?setHoverRowIndex@MultiColumnList@CEGUI@@QAEXH@Z");
    const auto reset=detail::Export<void (__thiscall*)(void*)>(module,"?resetList@MultiColumnList@CEGUI@@QAEXXZ");
    std::array<unsigned,0x224u/4u> manager{};std::unique_ptr<NativeTradeWindowInput> input;
    std::unique_ptr<NativeTradeWindowRepeat> repeat;
    NativeGameTimerQueue timers([](auto,auto){throw std::logic_error("unexpected parameterless Trade callback");},
        [&](auto owner,auto method,auto row,auto flag){Check(bool(repeat),"Trade repeat callback owner exists");repeat->Callback(owner,method,row,flag);});
    manager[0x1a0u/4u]=timers.Construct();
    std::vector<std::unique_ptr<NativeCeguiDefaultListItem>> cells;std::vector<unsigned> methods;
    unsigned close_calls{},prompt_calls{},quantity_reads{},quantity_writes{},changes{};bool fail_quantity{},nested{},retire_from_change{};
    std::unique_ptr<NativeNumericSpinnerPool> pool;std::vector<std::unique_ptr<NativeListboxSpinnerItem>> spinners;std::vector<std::array<unsigned,3>> spinner_events;
    NativeEventHandlerArray activity([&](auto member,auto slot){Check(input && input->OwnsActivity(member),"Trade input activity owner");return input->Callback(member,slot);},
        [&](auto member,auto slot,auto event){Check(input && input->OwnsActivity(member),"Trade input predicate owner");return input->Predicate(member,slot,event);});
    NativeTradeWindowLayout layout(ui,root,Address(manager.data()),[&](unsigned method,unsigned event){
        Check(event!=0u,"Trade real DLL input event exists");
        if(method==0x48cc20u || method==0x48cc80u || method==0x48cce0u){Check(bool(repeat),"Trade repeat frame owner exists");return repeat->Event(method,event);}
        if(method==0x48d1d0u || method==0x48d1a0u)return true; // Recorded detail/hover endpoints.
        Check(method==0x48d290u || method==0x48d310u,"only actual arrow callbacks are bound in input fixture");methods.push_back(method);
        if(nested){nested=false;std::array<unsigned,5> inner{};inner[4]=28u;Check(activity.DispatchKeyDown(activity.GetLast(),Address(inner.data())),"nested arrow callback may reenter active input");}
        return true;
    },[&]{if(repeat)repeat->Close();reset(reinterpret_cast<void*>(layout.Word(0x28u)));spinners.clear();cells.clear();if(pool)pool->Close();});
    NativeSpinnerServices spinner_services;
    spinner_services.changed=[&](unsigned item){Check(pool && pool->Value(item)>=0,"native spinner change callback sees actual value");++changes;if(retire_from_change)spinners.at(1)->Close();};
    spinner_services.event=[&](unsigned key,unsigned row,unsigned column){Check((key==0x8b185cu || key==0x8b1860u || key==0x8b1864u) && row<3u && !column,"native spinner repeat event keys/row");spinner_events.push_back({key,row,column});Check(layout.Event(key,{row,column}),"spinner press/release reaches actual Trade frame descriptor");};
    pool=std::make_unique<NativeNumericSpinnerPool>(ui,root,std::move(spinner_services));
    repeat=std::make_unique<NativeTradeWindowRepeat>(layout,*pool,timers,Address(manager.data()));
    NativeTradeWindowInputServices services;
    services.close=[&]{++close_calls;};services.prompt=[&]{++prompt_calls;};
    // Real shared ListboxSpinnerItem/NumericSpinner/pool back the quantity
    // endpoint. Final close/prompt remain recorded; real repeat timing is composed below.
    services.spinner_value=[&](unsigned cell){++quantity_reads;return pool->Value(cell);};
    services.set_spinner_value=[&](unsigned cell,int value){if(fail_quantity)throw std::runtime_error("quantity failure");++quantity_writes;pool->SetValue(cell,value);};
    bool missing{};auto incomplete=services;incomplete.set_spinner_value={};
    try{NativeTradeWindowInput bad(layout,std::move(incomplete));}catch(const std::invalid_argument&){missing=true;}Check(missing,"Trade input refuses absent quantity endpoint");
    input=std::make_unique<NativeTradeWindowInput>(layout,std::move(services));
    layout.BindParent(true);const auto list=layout.Word(0x28u),plain=layout.FrameAddress()+0x19cu,quantity=layout.FrameAddress()+0x1a8u;
    set_visible(reinterpret_cast<void*>(layout.Word(0x44u)),true);set_visible(reinterpret_cast<void*>(layout.Word(0x48u)),true);
    for(unsigned column=0;column<2u;++column){detail::NativeString text(module,column?"quantity":"name");
        detail::Export<void (__thiscall*)(void*,const void*,unsigned,float)>(module,"?addColumn@MultiColumnList@CEGUI@@QAEXABVString@2@IM@Z")(reinterpret_cast<void*>(list),text.Data(),column,0.45f);}
    for(unsigned row=0;row<3u;++row){
        (void)detail::Export<unsigned (__thiscall*)(void*,unsigned)>(module,"?addRow@MultiColumnList@CEGUI@@QAEII@Z")(reinterpret_cast<void*>(list),row);
        auto cell=std::make_unique<NativeCeguiDefaultListItem>(module,look,list,"item");const auto address=cell->Address();
        detail::Export<void (__thiscall*)(void*,void*,unsigned,unsigned)>(module,"?setItem@MultiColumnList@CEGUI@@QAEXPAVListboxItem@2@II@Z")(reinterpret_cast<void*>(list),reinterpret_cast<void*>(address),0u,row);cells.push_back(std::move(cell));
        auto spinner=pool->Create();spinner->Bind(list,row);spinner->SetRange(0,3);
        detail::Export<void (__thiscall*)(void*,void*,unsigned,unsigned)>(module,"?setItem@MultiColumnList@CEGUI@@QAEXPAVListboxItem@2@II@Z")(reinterpret_cast<void*>(list),reinterpret_cast<void*>(spinner->Address()),1u,row);spinners.push_back(std::move(spinner));
    }
    const auto row=[&]{return *reinterpret_cast<const int*>(list+0x4c0u);};
    const auto key=[&](unsigned code){std::array<unsigned,5> e{};e[4]=code;return activity.DispatchKeyDown(activity.GetLast(),Address(e.data()));};
    (void)activity.Insert(plain);Check(!layout.Word(0x1a4u),"real Insert invokes original status reset");set_hover(reinterpret_cast<void*>(list),0);layout.RethrowEventError();
    Check(key(200u) && (layout.Word(0x1a4u)&0xffu)==1u && *reinterpret_cast<const unsigned char*>(layout.Word(0x48u)+0x4b1u)==1u,"Up at first DLL row enters original arrow glow");
    Check(key(200u) && row()==0,"Up in arrow focus stays outside rows");
    Check(key(203u) && methods.back()==0x48d290u,"Left in arrow focus clicks actual original previous button");
    nested=true;Check(key(205u) && methods.back()==0x48d310u && prompt_calls==1u,"Right clicks actual next button and scoped input supports nested callback");
    Check(key(208u) && !layout.Word(0x1a4u) && row()==0,"Down leaves arrow focus and selects first DLL row");
    Check(key(208u) && row()==1 && key(208u) && row()==2 && key(208u) && row()==2,"native DLL hover navigation stops at last row");
    Check(key(200u) && row()==1,"Up moves to preceding actual row");
    Check(!key(57u) && key(28u) && prompt_calls==2u && key(1u) && close_calls==1u,"unknown key unhandled while Enter/Esc reach required endpoints");
    std::array<unsigned,5> probe{};probe[4]=28u;Check(!activity.Contains(Address(probe.data())),"both original fallback predicate slots return existing shared zero callback");
    Check(input->Callback(plain,16u)==1u && input->Callback(plain,20u)==1u,"verified shared true status/update slots remain constant callbacks");
    (void)activity.Remove(plain);(void)activity.Insert(quantity);set_hover(reinterpret_cast<void*>(list),1);layout.RethrowEventError();
    const auto quantity_cell=spinners[1]->Address();
    Check(key(205u) && pool->Value(quantity_cell)==1 && quantity_reads==1u && quantity_writes==1u &&
        key(203u) && pool->Value(quantity_cell)==0 && quantity_reads==2u && quantity_writes==2u,"quantity variant looks up hovered DLL row column1 and forwards exact signed increment/decrement");
    fail_quantity=true;bool failed{};try{(void)key(205u);}catch(const std::runtime_error& e){failed=std::string_view(e.what())=="quantity failure";}fail_quantity=false;
    Check(failed && key(205u) && pool->Value(quantity_cell)==1,"quantity endpoint error restores input scope and permits retry");
    set_hover(reinterpret_cast<void*>(list),0);layout.RethrowEventError();Check(key(200u) && (layout.Word(0x1b0u)&0xffu),"quantity variant shares category focus logic");
    const auto before_quantities=quantity_writes;Check(key(203u) && methods.back()==0x48d290u && quantity_writes==before_quantities,"quantity variant arrows select categories when focused");
    (void)input->Callback(quantity,12u);Check(!layout.Word(0x1b0u),"quantity input activation clears focus and real arrow glow");
    set_visible(reinterpret_cast<void*>(layout.Word(0x44u)),false);set_visible(reinterpret_cast<void*>(layout.Word(0x48u)),false);
    Check(key(200u) && !layout.Word(0x1b0u),"hidden category arrows prevent entering arrow focus");
    // Verify actual range/disabled-button policy, DLL text and public image
    // owner lifetime, not a test-side quantity map.
    spinners[1]->SetValue(100);Check(spinners[1]->Value()==3,"native numeric update clamps to actual maximum");
    spinners[1]->SetValue(101);Check(spinners[1]->Value()==3,"disabled increment direction preserves maximum");
    spinners[1]->SetValue(-5);Check(spinners[1]->Value()==0,"native numeric update clamps to minimum");
    retire_from_change=true;bool retirement_rejected{};try{spinners[1]->SetValue(1);}catch(const std::logic_error&){retirement_rejected=true;}retire_from_change=false;
    Check(retirement_rejected && spinners[1]->Address() && spinners[1]->Value()==1,"native change callback cannot retire its active row");
    // Real DLL list rendering invokes the native item vtable/layout and attaches
    // child widgets. The test window is hidden and remains nonactivating.
    RwRGBA black{0,0,0,255};Check(RwCameraClear(camera,&black,rwCAMERACLEARIMAGE|rwCAMERACLEARZ) && RwCameraBeginUpdate(camera),"begin actual spinner list render");
    const auto rendered=ui.Render();(void)RwCameraEndUpdate(camera);pool->RethrowEventError();layout.RethrowEventError();Check(rendered && changes>0u,"actual spinner rows render through original DLL and native vtable");
    const auto word=[](unsigned p){return *reinterpret_cast<const unsigned*>(p);};
    const auto numeric=word(quantity_cell+424u),left=word(numeric+424u),right=word(numeric+428u),text=word(numeric+432u);
    Check(word(numeric+316u)==list && word(numeric+308u)==1u && word(word(numeric+436u)+68u)==list,"DLL row draw transfers row/list and attaches original spinner widgets");
    const auto fire=detail::Export<void (__thiscall*)(void*,const void*,void*,const void*)>(module,"?fireEvent@EventSet@CEGUI@@UAEXABVString@2@AAVEventArgs@2@0@Z");
    detail::NativeString empty(module,"");
    const auto click=[&](unsigned target,const char* symbol,unsigned button=0u){
        std::array<unsigned,10> e{};detail::Export<void (__thiscall*)(void*,void*)>(module,"??0WindowEventArgs@CEGUI@@QAE@PAVWindow@1@@Z")(e.data(),reinterpret_cast<void*>(target));e[7]=button;
        fire(reinterpret_cast<void*>(target),detail::Export<const void*>(module,symbol),e.data(),empty.Data());
        detail::Export<void (__thiscall*)(void*)>(module,"??1WindowEventArgs@CEGUI@@UAE@XZ")(e.data());pool->RethrowEventError();};
    const auto clicked="?EventClicked@PushButton@CEGUI@@2VString@2@B",down="?EventMouseButtonDown@Window@CEGUI@@2VString@2@B",up="?EventMouseButtonUp@Window@CEGUI@@2VString@2@B";
    click(right,clicked);Check(spinners[1]->Value()==2,"actual original right-button subscription increments quantity");
    click(left,clicked);Check(spinners[1]->Value()==1,"actual original left-button subscription decrements quantity");
    const auto get_text=detail::Export<const void* (__thiscall*)(const void*)>(module,"?getText@Window@CEGUI@@QBEABVString@2@XZ");
    Check(detail::Export<const char* (__thiscall*)(const void*)>(module,"?c_str@String@CEGUI@@QBEPBDXZ")(get_text(reinterpret_cast<void*>(text)))[0]=='1',"actual DLL spinner label tracks value");
    click(left,down);click(right,down);click(right,up);click(left,up);click(right,down,1u);
    Check(spinner_events.size()==4u && spinner_events[0]==std::array<unsigned,3>{0x8b185cu,1u,0u} && spinner_events[1]==std::array<unsigned,3>{0x8b1860u,1u,0u} && spinner_events[2][0]==0x8b1864u && spinner_events[3][0]==0x8b1864u,"all six original button subscriptions preserve repeat keys/row and filter nonleft presses");
    timers.Update(1.f);Check(spinners[1]->Value()==1 && !timers.Count() && !timers.EventCount(),"button-up cancels pending delay and timer before first repeat");
    click(right,down);timers.Update(0.19f);timers.Update(0.005f);
    Check(spinners[1]->Value()==1 && !timers.Count(),"original repeat waits 0.2 seconds before creating periodic callback");
    timers.Update(0.006f);Check(spinners[1]->Value()==1 && timers.Count()==1u,"expired delay schedules original 0.1-second repeat without immediate increment");
    timers.Update(0.08f);Check(spinners[1]->Value()==1,"periodic callback preserves remaining time");
    timers.Update(0.02f);Check(spinners[1]->Value()==2 && row()==1,"actual timer modifies quantity and DLL hover row");
    timers.Update(0.11f);timers.Update(0.11f);Check(spinners[1]->Value()==3,"held original button respects numeric maximum");
    click(left,down);timers.Update(0.21f);Check(spinners[1]->Value()==2,"switching direction replaces previous timer and preserves decrement flag");
    click(left,up);timers.Update(1.f);Check(spinners[1]->Value()==2 && !timers.Count() && !timers.EventCount(),"release stops periodic and delayed callbacks");
    click(right,down);repeat->Cancel();timers.Update(1.f);Check(spinners[1]->Value()==2,"page cancellation stops callbacks before row replacement");
    bool foreign_timer{};try{repeat->Callback(layout.FrameAddress()+4u,0x48cd20u,1,0);}catch(const std::invalid_argument&){foreign_timer=true;}
    bool invalid_row{};try{repeat->Callback(layout.FrameAddress(),0x48cd20u,3,0);}catch(const std::out_of_range&){invalid_row=true;}
    Check(foreign_timer && invalid_row,"native repeat rejects foreign owner and retired/out-of-range row");
    bool busy{};try{pool->Close();}catch(const std::logic_error&){busy=true;}Check(busy,"pool rejects retirement while list rows are live");
    // Release borrowed cells only after the original list has detached them.
    const auto last_numeric=word(spinners.back()->Address()+424u);reset(reinterpret_cast<void*>(list));spinners.clear();cells.clear();
    Check(pool->IdleCount()==3u,"detached native rows return their three numeric widgets to the pool");
    auto recycled=pool->Create(99);Check(word(recycled->Address()+424u)==last_numeric && pool->IdleCount()==2u && recycled->Value()==0,"native pool reuses last numeric object and original row constructor resets initial value");
    recycled->Close();Check(pool->IdleCount()==3u,"recycled row returns numeric exactly once");
    bool foreign{};try{(void)input->Callback(plain+4u,12u);}catch(const std::invalid_argument&){foreign=true;}Check(foreign,"foreign input component rejected");
    (void)activity.Remove(quantity);layout.Close();timers.Destroy();Check(!timers.OutstandingAllocations() && !pool->OutstandingAllocations(),"native spinner/timer owners release rows/controls/callbacks/buffers");Check(!input->OwnsActivity(plain) && !input->OwnsActivity(quantity) && !activity.Size(),"input/activity retire before original layout windows");activity.ReleaseStorage();
    std::cout<<"PASS: native Trade activity/status and actual DLL row/arrow key input,nested dispatch,error retry,foreign/retired gates; actual native pooled quantities/range/rendering and original delay/repeat/release timer chain; close/prompt endpoints recorded\n";
}
}
