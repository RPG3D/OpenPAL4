#pragma once
#include <cstdint>
namespace pal4 {
// Borrow actual ActorManager frame services for53F740. Both phases are required;
// model/effect ownership and camera Begin/End belong to the enclosing battle.
class INativeCombatWorldFrameServices {
public:
    virtual ~INativeCombatWorldFrameServices()=default;
    virtual std::uint32_t Address() const=0;
    virtual void ProcessActorUpdates(std::uint32_t context)=0;
    virtual std::uint32_t UpdateActorComponents(std::uint32_t context)=0;
};
}
