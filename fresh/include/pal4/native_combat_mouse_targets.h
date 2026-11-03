#pragma once
#include <array>
#include <cstdint>
#include <memory>
namespace pal4 {
class NativeCombatTargetSelection;class NativeCombatRosterTargets;class NativeCombatWorldResources;
class NativeCameraManagerOwner;class NativeScriptEventRouter;
// Borrows actual mixed rosters, world, CombatCamera and router. Close before
// target/model/world/camera retirement. This is pointer selection, not world draw.
class NativeCombatMouseTargets final {
public:
    NativeCombatMouseTargets(NativeCombatTargetSelection&,NativeCombatRosterTargets&,NativeCombatWorldResources&,
        NativeCameraManagerOwner&,NativeScriptEventRouter&);
    ~NativeCombatMouseTargets();
    // Borrow existing53F3B0/4C2AB0 publication and real SDK camera membership.
    // Does not publish pointers or attach cameras on behalf of the world owner.
    void BindWorldCamera();
    //574DF0/576190/53BC20. First pixel corresponds to config12/13; second is
    //the callback's float coordinate pair. Neither is implicitly UI scaled.
    std::uint32_t Update(const std::array<float,2>& tips_pixel,const std::array<float,2>& selection_pixel);
    //53C2C0 only: finds an actor without changing selection visuals.
    std::uint32_t FindNearest(const std::array<float,2>& pixel);
    //53C500 periodic preferred-Monster / fallback-Player tips scan.
    std::uint32_t UpdateHoverTips(const std::array<float,2>& pixel);
    std::uint32_t LastAtomicIndex()const;
    void Close();
private:
    class Impl;std::unique_ptr<Impl> impl_;
};
}
