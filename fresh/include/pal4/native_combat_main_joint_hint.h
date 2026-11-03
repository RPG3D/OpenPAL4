#pragma once
#include <cstdint>
namespace pal4 {
class NativeCombatMainWindow;
class NativeCombatPlayerManager;
class NativeCombatMonsterManager;
struct NativeCombatSystemImage;
//54CC60 borrows live owners. Does not subscribe or resolve/queue an attack.
class NativeCombatMainJointHint final {
public:
    NativeCombatMainJointHint(NativeCombatMainWindow&,NativeCombatPlayerManager&,
        NativeCombatMonsterManager&,NativeCombatSystemImage&);
    void Handle(std::uint32_t payload);
private:
    NativeCombatMainWindow& window_;
    NativeCombatPlayerManager& players_;
    NativeCombatMonsterManager& monsters_;
    NativeCombatSystemImage& system_;
};
}
