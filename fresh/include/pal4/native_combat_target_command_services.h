#pragma once
#include "pal4/native_combat_target_selection.h"
#include "pal4/actor_manager_target_selection_controller.h"
namespace pal4 {
class NativeGameDatabase;
class NativePropertyRecordCatalog;
class InputManager;
class NativeSkillSystemOwner;
// Remaining required production roster/actor services. Returned names are actual
// borrowed C strings owned by the actor, never original EXE static addresses.
class INativeCombatTargetSelectionActors {
public:
    virtual ~INativeCombatTargetSelectionActors()=default;
    virtual std::uint32_t CombatSystemAddress()=0;
    virtual std::uint32_t RefreshSelection(std::uint32_t controller)=0;
    virtual std::uint32_t SetTargetSelected(std::uint32_t actor,std::uint32_t value)=0;
    virtual std::uint32_t ActorNameAddress(std::uint32_t actor)=0;
};
// Real original DB target-mode algorithm and existing InputManager reset.
// Borrows all dependencies; immutable numeric Magic/Stunt projections are owned.
class NativeCombatTargetCommandServices final : public INativeCombatTargetSelectionServices {
public:
    NativeCombatTargetCommandServices(const NativeGameDatabase&,const NativePropertyRecordCatalog&,
        InputManager&,NativeSkillSystemOwner&,INativeCombatTargetSelectionActors&);
    ~NativeCombatTargetCommandServices() override;
    // Share existing immutable record projections with command selection.
    std::uint32_t FindCommandRecord(SelectionRecordKind,std::uint32_t id)const;
    std::uint32_t CommandTargetType(SelectionRecordKind,std::uint32_t record)const;
    void ReportInvalidTargetType(std::uint32_t);
    std::uint32_t CombatSystemAddress() override;
    std::uint32_t InputManagerAddress() override;
    std::uint32_t DetermineTargetMode(std::uint32_t controller) override;
    std::uint32_t RefreshSelection(std::uint32_t controller) override;
    std::uint32_t InitializeInput(std::uint32_t input) override;
    std::uint32_t SetTargetSelected(std::uint32_t actor,std::uint32_t value) override;
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};
}
