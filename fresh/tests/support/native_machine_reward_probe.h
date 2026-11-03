#pragma once
#include "pal4/native_gob_machine_reward_tasks.h"
#include "pal4/native_gob_machine_runtime.h"
#include "pal4/native_script_object_runtime.h"
#include "pal4/native_inventory_session.h"
#include "pal4/native_script_inventory_change.h"
#include "pal4/native_cegui_ui_session.h"
#include "pal4/native_miles_sound_2d_runtime.h"
#include "pal4/gob_machine_task_runtime.h"
#include <functional>
#include <iostream>
#include <set>
#include <stdexcept>
namespace pal4::test {
inline void VerifyNativeMachineRewards(NativeBaseObjectMemory& memory,
    const NativeGameDatabase& database,NativeCeguiUiSession& ui,
    NativeMilesSound2dRuntime& sound,const std::function<void(float)>& advance){
    const auto check=[](bool ok,const char* text){if(!ok)throw std::runtime_error(text);};
    const auto before=memory.OutstandingAllocations();
    // Like actual NewGame: construction alone does not initialize money+134.
    NativeInventorySession stock;(void)stock.Start();stock.Reset();stock.BindDatabase(database);
    std::set<std::uint32_t> items,equipment;
    stock.BindItemObserver([&](std::uint32_t id){items.insert(id);});
    stock.BindEquipmentObserver([&](std::uint32_t id){equipment.insert(id);});
    std::vector<std::uint32_t> handles;
    NativeScriptInventoryChange hints(stock,database,
        [&]{if(!ui.LoadOriginalHintLayout())throw std::runtime_error(ui.LastError());return ui.HintFrameAddress();},
        [&](std::uint32_t window,std::string_view text,float x,float y,float duration,std::uint32_t parent,std::uint8_t flag){
            check(window==ui.HintFrameAddress(),"reward probe uses actual HintWindow");
            if(!ui.ShowOriginalHint(text,x,y,duration,parent,flag))throw std::runtime_error(ui.LastError());
        },
        [&](std::string_view path,std::uint32_t mode){check(mode==1u,"reward sound plays once");handles.push_back(sound.Play(path,false,true,{},0.f,0.f));});
    NativeGobMachineRewardServices services{&stock,&database,&hints,[&]{return ui.HintVisible();}};
    NativeScriptObjectRuntime tasks(memory);
    NativeGobMachineRewardTasks rewards(memory,tasks,services);
    for(const auto type:{7,8,9}){
        GobMachineTaskDefinition definition;definition.type=type;
        if(type==9)definition.parameters={{"PAL4_GOMTask_getitem_money",std::int32_t(200)}};
        else definition.parameters={{"PAL4_GOMTask_getitem_id",std::int32_t(type==7?3103:3504)},
            {"PAL4_GOMTask_getitem_num",std::int32_t(type==7?3:99)}};
        const auto task=rewards.Construct();
        rewards.Initialize(task,stock.Address(),0u,definition);rewards.Begin(task);
        check(ui.HintVisible(),"real reward opens actual HintWindow");
        rewards.Tick(task);rewards.Tick(task);
        check(!memory.ReadByte(task+0x3cu),"reward task waits while actual hint remains visible");
        if(type==7)check(stock.ItemCount(3103)==3u && items.contains(3103),"property reward uses original count and discovery observer");
        else if(type==8){
            unsigned count{};for(unsigned category=0;category<7u;++category)count+=stock.EquipmentCount(category);
            check(count==1u && equipment.contains(3504),"equipment reward adds exactly one instance despite amount99");
        }else check(stock.ReadDword(stock.Address()+0x134u)==200u,"money reward uses actual capped inventory counter");
        advance(1.1f);
        check(!ui.HintVisible(),"actual one-shot hint timer closes reward UI");
        rewards.Tick(task);check(memory.ReadByte(task+0x3cu)!=0u,"reward completes after actual hint closes");
        rewards.Destruct(task);(void)memory.Free(task);
        advance(.001f);
    }
    for(const auto handle:handles)(void)sound.StopSample(handle);
    stock.Close();check(stock.OutstandingAllocations()==0u && memory.OutstandingAllocations()==before,
        "isolated native reward inventory/task/text ownership cleans up");
    std::cout<<"Native Machine rewards: actual Property3103/count3, one Equip3504 ignoring99, Money200, discovery, original formats/audio, real hint timer wait and cleanup verified\n";
}
}
