#pragma once
#include "pal4/gob_obstacle.h"
#include "pal4/gob_obstacle_combat_callback.h"
#include "pal4/gob_machine.h"
#include "pal4/native_gob_object_image.h"
#include "pal4/string_buffer.h"
#include "pal4/dname_node_event_record.h"
#include "pal4/effect_handle.h"
#include <functional>
#include <stdexcept>

namespace pal4 {
// Construction/destruction borrows the Machine creator. Launch strings are
// StringBuffer records, not the unrelated legacy text records used by snapshots.
class NativeGobObstacleLifecycle final : public IGobObstacleRuntime,
    private IGobObstacleCombatCallbackRuntime, private IDNameNodeRuntime {
public:
    NativeGobObstacleLifecycle(NativeBaseObjectMemory& memory,
        IGobMachineConstructRuntime& machine, INativeGobEffectService* effects=nullptr,
        std::function<std::uint32_t(std::uint32_t)> cleanup={})
        : memory_(memory),machine_(machine),effects_(effects),cleanup_(std::move(cleanup)){}
    std::uint32_t Allocate(std::uint32_t n) override{return memory_.Allocate(n);}
    void Free(std::uint32_t p) override{(void)memory_.Free(p);}
    void ConstructMachine(std::uint32_t p) override{(void)GobMachine::Construct(p,machine_);}
    void DestructMachine(std::uint32_t p) override{GobMachine::Destruct(p,machine_);}
    std::uint32_t CleanupMachineResources(std::uint32_t p) override{
        if(!cleanup_)throw std::logic_error("Obstacle Machine cleanup service missing");
        return cleanup_(p);
    }
    void InitializeObstacleHelper(std::uint32_t p) override{(void)GobObstacleCombatCallback::Construct(p,*this);}
    void DestructObstacleHelper(std::uint32_t p) override{(void)GobObstacleCombatCallback::ConstructBase(p,*this);}
    void InitializeStringRecord(std::uint32_t p) override{(void)StringBuffer::InitializeEmptyAt(p,memory_);}
    void ReleaseStringRecord(std::uint32_t p) override{(void)StringBuffer::CleanupAt(p,memory_);}
    void InitializeEffectHandle(std::uint32_t p) override{(void)DNameNode::Construct(p,*this);}
    void ResetEffectHandle(std::uint32_t p) override{
        EffectHandle::Reset(*reinterpret_cast<EffectSystemHandleWords*>(p));
    }
    bool DialogNeedsUpdate(std::uint32_t p) override{
        if(!effects_){if(ReadWord(p+4u))throw std::logic_error("Obstacle effect service missing");return false;}
        return effects_->IsActive(ReadWord(p),ReadWord(p+4u));
    }
    void UpdateDialogContent(std::uint32_t p) override{
        if(!effects_)throw std::logic_error("Obstacle effect service missing");
        effects_->Release(ReadWord(p),ReadWord(p+4u));
    }
    std::uint32_t ReadWord(std::uint32_t p) override{return memory_.ReadDword(p);}
    void WriteWord(std::uint32_t p,std::uint32_t v) override{memory_.WriteDword(p,v);}
private:
    // Only helper lifecycle uses this endpoint. Runtime callbacks have their
    // separate scene-owned adapter; never execute them through a creator.
    std::uint32_t MovementAdapter() override{throw std::logic_error("Obstacle callback through lifecycle");}
    void InvokeObjectSlot80(std::uint32_t,const std::array<std::uint32_t,3>&) override{throw std::logic_error("Obstacle callback through lifecycle");}
    void ApplyEffectTypeToActor(std::uint32_t,std::uint32_t) override{throw std::logic_error("Obstacle callback through lifecycle");}
    NativeBaseObjectMemory& memory_;
    IGobMachineConstructRuntime& machine_;
    INativeGobEffectService* effects_;
    std::function<std::uint32_t(std::uint32_t)> cleanup_;
};
}
