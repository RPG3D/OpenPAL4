#pragma once
#include "pal4/native_actor_behavior_resources.h"
#include "pal4/native_actor_animation_state_binding.h"
#include "pal4/native_monster_scene_object_builder.h"
#include "pal4/native_monster_info_reader.h"
#include "pal4/native_monster_manager_owner.h"
#include "pal4/native_world_path_planner_owner.h"
#include "pal4/game_object_embedded_animation.h"
#include <algorithm>
#include <bit>
#include <iostream>
#include <stdexcept>

namespace pal4::test {
template<class Schema,class Animation>
void VerifyNativeBehaviorAnimationSelection(const std::filesystem::path& root,
    NativeBaseObjectMemory& memory,Schema& schema,Animation& animation) {
    const auto check=[](bool value,const char* message) {
        if(!value)throw std::runtime_error(message);
    };
    NativeMonsterInfoReader reader;check(reader.Load(root,"M13","1"),"M13 behavior selection source");
    const auto found=std::find_if(reader.Records().begin(),reader.Records().end(),
        [](const auto& row){return row.name=="m13-09-01";});
    check(found!=reader.Records().end() && found->resource=="519","actual m13-09-01 model519");
    NativeWorldPathPlannerOwner paths;
    NativePlayerTransformRuntime transforms;
    NativeMonsterManagerOwner manager;check(manager.Construct()!=0u,"behavior selection manager");
    NativeMonsterSceneObjectBuilder builder(root,manager,memory,schema,animation,transforms);
    check(manager.InitializeScene(std::span(&*found,1u),[&](const auto& row){builder.Build(row);}),
          "behavior selection actor creation");
    const auto actor=manager.FindObject("m13-09-01");
    auto& binding=manager.AnimationBinding(actor);
    NativeActorBehaviorResources resources(paths,
        [&](auto value){check(value==actor,"behavior selection actor identity");},
        [&](auto value)->NativeActorAnimationStateBinding& {
            check(value==actor,"behavior selection binding identity");return binding;
        });
    auto& state=*reinterpret_cast<GameObjectEmbeddedAnimationState*>(actor+0x170u);
    check(binding.Select("C02") && binding.SelectBlended("Z01",0.5f),"original C02 to Z01 transition");
    binding.SetRestart(0u);
    check(binding.AdvanceFrame(0.017f) && state.words[15]==1u,"actual Z01 blend in progress");
    const auto before=state.words;
    check(resources.Select(actor,"z01",false,0.f) && state.words==before,
          "same-name behavior restore preserves the actual blend instead of restarting Z01");
    check(resources.Select(actor,"Z01",true,0.25f) && state.words==before,
          "same-name behavior release preserves blend duration and completion state");
    for(unsigned frame=1;frame<100u;++frame)
        check(binding.AdvanceFrame(0.017f),"advance retained behavior blend");
    check(binding.Completed()==0u,"Z01 includes the retained half-second blend before completion");
    const auto progressing=state.words;
    const auto clock=std::bit_cast<std::uint32_t>(binding.Time());
    check(binding.Time()>0.f && resources.Select(actor,"z01",false,0.f) &&
              state.words==progressing && std::bit_cast<std::uint32_t>(binding.Time())==clock,
          "same-name behavior request preserves a nonzero SDK interpolator clock");
    for(unsigned frame=0;frame<40u && !binding.Completed();++frame)
        check(binding.AdvanceFrame(0.017f),"advance Z01 to actual completion");
    check(binding.Completed()!=0u,"Z01 eventually completes");
    const auto completed=state.words;
    check(resources.Select(actor,"Z01",false,0.f) && state.words==completed,
          "same-name behavior request retains completion latch");
    check(resources.Select(actor,"c02",true,0.25f) && state.words[15]==1u &&
              state.words[13]==std::bit_cast<std::uint32_t>(0.25f),
          "different-name behavior request retains the supplied blend duration");
    const auto changed=state.words;
    check(!resources.Select(actor,"missing_behavior_action",false,0.f) && state.words==changed,
          "missing behavior action retains the selected animation");
    std::cout<<"Native behavior animation selection: model519 C02/Z01, same-name blend/time/latch retention and changed/missing action\n";
}
}
