#pragma once
#include <cstdint>
#include <memory>
#include <span>
namespace pal4 {
class NativeCombatPlayerManager;
class NativeCombatActorSkillRuntime;
// Borrow the original Player first roster and its actual property/collection
// owners. These owners must outlive this service and every bound UI callback.
class NativeCombatActionAvailability final {
public:
    NativeCombatActionAvailability(NativeCombatPlayerManager&,
        std::span<NativeCombatActorSkillRuntime* const>);
    ~NativeCombatActionAvailability();
    std::uint8_t Property(std::uint32_t player,std::uint32_t command);
    bool HasAvailableMagic();
    void Close();
private:
    class Impl;std::unique_ptr<Impl> impl_;
};
}
