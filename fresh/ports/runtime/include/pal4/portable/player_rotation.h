#pragma once
#include "pal4/portable/player.h"
#include "pal4/portable/player_notifications.h"
namespace pal4::portable {
class PlayerRotation {
public:
    PlayerRotation(PlayerRoster& players,PlayerNotifications& notifications);
    ~PlayerRotation();
    PlayerRotation(const PlayerRotation&)=delete;
    PlayerRotation& operator=(const PlayerRotation&)=delete;
    void SetCurrentAngle(float angle);
    void WriteReport(std::ostream& output) const;
private:struct Impl;std::unique_ptr<Impl> impl_;
};
}
