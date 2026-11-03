#pragma once
#include "pal4/native_base_object_memory.h"
#include "pal4/native_combat_message_children.h"
#include "pal4/native_combat_action_children.h"
#include "pal4/gom_task_base.h"
#include "pal4/gom_task_container.h"
#include <set>
#include <cstring>
#include <stdexcept>
namespace pal4 {
// Shared typed GOM owner for message and approach/position children. Extracted
// from the verified Magic/Stunt owner; Property uses the same allocator/dispatch.
class NativeCombatSequenceChildren final:public IGomTaskBaseRuntime,public IGomTaskContainerRuntime {
public:
        NativeBaseObjectMemory memory;NativeCombatMessageChildren messages;std::unique_ptr<NativeCombatActionChildren> positions;
        std::set<std::uint32_t> positioned;
        NativeCombatSequenceChildren(NativeCombatActorSkillRuntime& skills,ICombatActionTaskRuntime& playback,NativeCombatMessageChildren::Model model,
            NativeEffectSceneSession& effects,NativeScriptEventRouter& events,float& scale,NativeCombatActionChildren& prototype)
            :messages(skills,playback,std::move(model),effects,events,scale,memory),positions(prototype.CreatePositionDomain(memory)){}
        IGomTaskContainerRuntime& Owner(std::uint32_t a){if(positioned.contains(a))return positions->ContainerRuntime();if(messages.Owns(a))return messages.ContainerRuntime();throw std::invalid_argument("foreign combat sequence child");}
        std::uint32_t AllocateMemory(std::uint32_t n)override{return memory.Allocate(n);}
        std::uint32_t FreeMemory(std::uint32_t a)override{return memory.Free(a);}
        std::uint32_t ReadWord(std::uint32_t a)override{return memory.ReadDword(a);}
        void WriteWord(std::uint32_t a,std::uint32_t v)override{memory.WriteDword(a,v);}
        void CopyCString(std::uint32_t a,const char* s,std::uint32_t n)override{std::memcpy(reinterpret_cast<void*>(a),s,n);}
        GomTaskBaseState& TaskAt(std::uint32_t a)override{return Owner(a).TaskAt(a);}
        void DeleteTask(std::uint32_t a,std::uint32_t flags)override{Owner(a).DeleteTask(a,flags);positioned.erase(a);}
        bool IsWaitComplete(std::uint32_t a)override{return Owner(a).IsWaitComplete(a);}
        void BeginRun(std::uint32_t a)override{Owner(a).BeginRun(a);}
        void Tick(std::uint32_t a,float dt)override{Owner(a).Tick(a,dt);}
        bool IsDurationComplete(std::uint32_t a)override{return Owner(a).IsDurationComplete(a);}
        void Finish(std::uint32_t a)override{Owner(a).Finish(a);}
        void Destroy(std::uint32_t a){if(positioned.contains(a)){positions->Destroy(a);positioned.erase(a);}else messages.Destroy(a);}
};

}
