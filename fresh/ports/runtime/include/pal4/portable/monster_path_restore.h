#pragma once
#include "pal4/behavior_helper_configuration.h"
#include "pal4/monster_path_endpoint_restore.h"
#include "pal4/portable/actor_playback.h"
#include "pal4/binary_xml_config.h"
namespace pal4::portable {
// Required scene bindings; no empty production fallback. World owns path tokens,
// associations and registration. Any failed AddElement must undo its own work.
class IMonsterPathRestoreWorld:public IBehaviorHelperConfigurationBindings {
public:
    virtual std::uint32_t FindPath(std::string_view name)=0;
    virtual std::uint32_t GridParameter()=0;
    virtual void AddElement(BehaviorPathEndpointState&)=0;
    virtual void RemoveElement(BehaviorPathEndpointState&) noexcept=0;
    virtual void ReleaseAssociation(BehaviorPathEndpointState&) noexcept=0;
};
struct MonsterPathRestoreInput {
    const BinaryXmlConfig& behavior;
    // Caller supplies the actual behavior memory attributes, not a guessed cache root.
    const BinaryXmlNode& memory;
    MonsterTransformRows transform;
    float manager_radius{};
};
// Calls original C02/C03 Accept. Helper must be constructed and unbound.
// Returns original B8 registration state. On failure registration/association
// are rolled back and helper restored; discard/recreate playback before retry
// if an animation operation threw (the engine does not offer atomic playback).
bool RestoreMonsterPath(std::int32_t kind,BehaviorPathEndpointState& helper,
    const MonsterPathRestoreInput&,ActorPlayback&,IMonsterPathRestoreWorld&);
}
