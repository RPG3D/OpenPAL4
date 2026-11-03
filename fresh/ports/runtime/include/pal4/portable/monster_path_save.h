#pragma once
#include "pal4/portable/monster_movement_world.h"
namespace pal4::portable {
// Original C02/C03 Release. Writes the supplied actual behavior-memory node,
// removes/deletes the registered helper, then blends to C01 or Z01 (0.5s).
// A failure can leave partial memory/playback changes. The helper may already
// be deleted if animation selection throws; caller must stop and rebuild.
std::uint32_t SaveMonsterPath(std::int32_t kind,MonsterHelperHandle,
    BinaryXmlNode& memory,ActorPlayback&,MonsterMovementWorld&);
}
