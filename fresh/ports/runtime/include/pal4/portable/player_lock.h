#pragma once
#include "pal4/portable/player.h"
#include "pal4/portable/world.h"
#include "pal4/portable/controller.h"
#include "pal4/portable/scene_ui.h"
namespace pal4::portable {
void LockPlayer(PlayerRoster& players,ActorController& controller,SceneUi& ui,
    IControllerResetServices& services,const std::function<void(std::uint32_t,std::string_view)>& select_animation);
void UnlockPlayer(PlayerRoster& players,ActorController& controller,SceneUi& ui,
    IControllerResetServices& services,const std::function<void(Point3,Point3)>& update_grid);
}
