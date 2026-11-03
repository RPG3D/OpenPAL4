#pragma once
#include "pal4/portable/monster_path_restore.h"
#include "pal4/portable/monster_path_save.h"
namespace pal4::portable {
// Original class3 Accept/Release; uses the actual memory node and playback.
// Construct with World::CreateRandom. Restore rolls back helper ownership on
// exceptions; animation may have changed and must be rebuilt before retry.
bool RestoreMonsterRandom(BehaviorPathEndpointState&,const MonsterPathRestoreInput&,
    ActorPlayback&,IMonsterPathRestoreWorld&);
// Saves six fields, removes/deletes helper, then blends C01 fallback Z01.
// Failure is not transactional: helper may be deleted before playback fails.
std::uint32_t SaveMonsterRandom(MonsterHelperHandle,BinaryXmlNode& memory,
    ActorPlayback&,MonsterMovementWorld&);
}
