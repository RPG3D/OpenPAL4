#include "pal4/portable/controller.h"
#include "pal4/pal_actor_control_reset.h"
#include <stdexcept>

namespace pal4::portable {
namespace {
[[noreturn]] void Missing(){throw std::logic_error("active controller effect/pet reset is not bound");}
struct Runtime {
    ControllerState& state;
    IControllerResetServices& services;
    pal4::PalActorControlState& image;
    void SetCursor(std::string_view name){
        if(name!="default.cur")throw std::logic_error("invalid reset cursor");
        // The original writes mode/vectors before its first external call.
        state.enabled=image.words[0]!=0;state.transient={};state.input_vector={};
        services.SetDefaultCursor();
    }
    void ProcessInput(pal4::PalActorControlState&) {
        // 423D90 -> input ResetActor, database ResetDialog, motion Reset.
        state.input.result=0;state.input.pending[3]=0;
        state.database.flags[0]=0;state.database.flags[2]=0;state.database.flags[3]=1;
        state.database.actor=state.actor;
        state.motion.active=false;state.motion.pending=false;state.motion.mode=0;state.motion.reached=true;
        if(state.actor)state.animation_scale=1;
    }
    void ProcessInputAndClearVector(pal4::PalActorControlState&) {
        // 423E50 -> input reset without animation, database ResetWithUi.
        state.input.result=0;state.input.pending[3]=0;
        services.HideInteractionUi();
        state.database.flags[0]=0;state.database.flags[1]=0;state.database.flags[2]=0;state.database.flags[3]=1;
        state.database.actor=state.actor;state.input_vector={};
        if(state.actor)state.animation_scale=1;
    }
    void InitializeInputState(){services.ClearPlatformInput();}
    void ProcessMonsters(){services.SetMonsterPursuitStopped(true);}
    void ClearMonsterFlags(){services.SetMonsterPursuitStopped(false);}
    void SetActorAnimationFlag(std::uint32_t,std::uint32_t){state.animation_scale=1;}
    bool DialogNeedsUpdate(std::uint32_t* dialog){if(dialog[1])Missing();return false;}
    void UpdateDialog(std::uint32_t*){Missing();}
    void UpdateGameStateConditional(pal4::PalActorControlState&,std::uint32_t){Missing();}
    void UpdateGameStateAndUi(pal4::PalActorControlState&,std::uint32_t){Missing();}
    void UpdateDialogStatus(pal4::PalActorControlState&,std::uint32_t){Missing();}
    // This runtime does not load a pet. Future pet loading must supply its
    // owned state and lifecycle endpoints before allowing that branch.
    std::uint32_t GetPetManager(){return 1;}
    bool PetActive(std::uint32_t){return false;}
    std::uint32_t GetArenaManager(){Missing();}
    void ScriptPetOnRemove(std::uint32_t){Missing();}
    void PetVirtualRemove(std::uint32_t){Missing();}
    std::uint32_t ResetPet(std::uint32_t){Missing();}
    void PetVirtualLoad(std::uint32_t,std::string_view,std::string_view,std::uint32_t){Missing();}
    void InitializePetData(std::uint32_t,std::uint32_t){Missing();}
    void SetPetStateFlag(std::uint32_t,std::uint32_t){Missing();}
    std::uint32_t ScriptPetOnAdd(std::uint32_t){Missing();}
};
}
void ActorController::ResetAndSync(bool enabled,IControllerResetServices& services) {
    if(!Initialized())throw std::logic_error("controller reset before initialization");
    pal4::PalActorControlState image;
    image.words[0]=state_.enabled;image.words[1]=state_.actor?*state_.actor+1:0;image.words[33]=state_.input.mode;
    constexpr std::array<unsigned,8> indices{20,21,22,14,15,16,17,18};
    for(unsigned i=0;i<indices.size();++i)image.words[indices[i]]=state_.transient[i];
    Runtime runtime{state_,services,image};
    (void)pal4::ResetActorControlState(image,enabled,runtime);
    state_.enabled=image.words[0]!=0;
    for(unsigned i=0;i<indices.size();++i)state_.transient[i]=image.words[indices[i]];
    ++state_.reset_count;
}
void ActorController::ProcessInput(bool clear_vector,IControllerResetServices& services) {
    if(!Initialized())throw std::logic_error("controller input reset before initialization");
    pal4::PalActorControlState image;Runtime runtime{state_,services,image};
    if(clear_vector)runtime.ProcessInputAndClearVector(image);else runtime.ProcessInput(image);
}
} // namespace pal4::portable
