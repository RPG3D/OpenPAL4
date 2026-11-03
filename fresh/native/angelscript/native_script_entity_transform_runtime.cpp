#include "pal4/native_script_entity_transform_runtime.h"
#include "pal4/script_player_position.h"
#include "pal4/script_player_rotation.h"
#include "pal4/game_resource_manager.h"
#include <cstring>
#include <stdexcept>

namespace pal4 {
NativeScriptEntityTransformRuntime::NativeScriptEntityTransformRuntime(
    IScriptPlayerPositionRuntime& positions, IScriptPlayerRotationRuntime& rotations,
    IGameResourceManagerRuntime& transforms)
    :positions_(positions),rotations_(rotations),transforms_(transforms){}
std::uint32_t NativeScriptEntityTransformRuntime::PlayerManager(){return positions_.PlayerManager();}
std::uint32_t NativeScriptEntityTransformRuntime::FindPlayer(std::uint32_t manager,std::int32_t index){
    return selected_=positions_.PlayerByIndex(manager,index);
}
std::uint32_t NativeScriptEntityTransformRuntime::NpcManager(){return positions_.NpcManager();}
std::uint32_t NativeScriptEntityTransformRuntime::FindNpc(std::uint32_t manager,std::string_view name){
    return selected_=positions_.NpcByName(manager,name);
}
float NativeScriptEntityTransformRuntime::ReadFloat(std::uint32_t address){
    if(selected_ && address==selected_+0xa0u)return rotations_.ReadX(selected_);
    if(selected_ && address==selected_+0xa8u)return rotations_.ReadZ(selected_);
    throw std::invalid_argument("PAL4 immediate transform rotation field");
}
void NativeScriptEntityTransformRuntime::RequireSelected(std::uint32_t actor,std::uint32_t flags) const{
    if(!selected_ || actor!=selected_ || flags)throw std::invalid_argument("PAL4 immediate transform owner/mode");
}
void NativeScriptEntityTransformRuntime::SetPosition(std::uint32_t actor,
    const std::array<float,3>& position,std::uint32_t flags){
    RequireSelected(actor,flags);
    // IDA842F10+4 is666550; +60(decimal) is a different player override.
    GameResourceManager resource;
    std::memcpy(&resource,reinterpret_cast<const void*>(actor),sizeof(resource));
    if(!resource.words[5])throw std::logic_error("PAL4 immediate transform Clump unavailable");
    (void)resource.UpdateClumpTransform({position[0],position[1],position[2]},false,transforms_);
    std::memcpy(reinterpret_cast<void*>(actor),&resource,sizeof(resource));
}
std::uint32_t NativeScriptEntityTransformRuntime::SetRotation(std::uint32_t actor,
    const std::array<float,3>& rotation,std::uint32_t flags){
    RequireSelected(actor,flags);
    rotations_.SetRotation(actor,{rotation[0],rotation[1],rotation[2]},flags);
    return 0u; // Both original script overloads return void.
}
}
