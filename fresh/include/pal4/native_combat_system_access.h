#pragma once
#include <cstdint>
#include <functional>
#include <memory>
namespace pal4 {
// Native registration adopts an eagerly composed CombatSystem whose typed
// owners live outside this access adapter. Validate checks every constructed
// dependency before acquisition; Close precedes retiring any borrowed owner.
// This does not run state Entry, cold517 initialization or original destruction.
class NativeCombatSystemAccess final {
public:
    NativeCombatSystemAccess(std::uint32_t constructed,std::function<void()> validate);
    ~NativeCombatSystemAccess();
    NativeCombatSystemAccess(const NativeCombatSystemAccess&)=delete;
    NativeCombatSystemAccess& operator=(const NativeCombatSystemAccess&)=delete;
    std::uint32_t Acquire();
    std::uint32_t RegistrationCount() const;
    void Close();
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};
}
