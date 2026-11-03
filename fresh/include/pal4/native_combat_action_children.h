#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
namespace pal4 {
class NativeBaseObjectMemory;
class IGomTaskBaseRuntime;
class IGomTaskContainerRuntime;
struct NativeCombatApproachServices {
    std::function<std::array<float,3>(std::uint32_t)> position;
    std::function<void(std::uint32_t,const std::array<float,3>&)> facing;
    std::function<void(std::uint32_t)> run;
    std::function<void(std::array<float,3>&)> normalize;
    std::function<double(const std::array<float,3>&)> magnitude;
};
// Actual protection and approach GOM owners. Borrowed callbacks must
// resolve real actor/model owners. The containing GOM list retires first.
class NativeCombatActionChildren final {
public:
    using Move=std::function<void(std::uint32_t,const std::array<float,3>&)>;
    using Protect=std::function<void(std::uint32_t,std::uint32_t,std::uint32_t)>;
    NativeCombatActionChildren(Move,Protect);
    ~NativeCombatActionChildren();
    NativeCombatActionChildren(const NativeCombatActionChildren&)=delete;
    NativeCombatActionChildren& operator=(const NativeCombatActionChildren&)=delete;
    // Reuse real position/protection services with the containing mixed GOM allocator.
    std::unique_ptr<NativeCombatActionChildren> CreatePositionDomain(NativeBaseObjectMemory&);
    void BindApproach(NativeCombatApproachServices);
    void BindDefend(std::function<void(std::uint32_t child)>);
    void UnbindDefend();
    void ConfigureDefend(std::uint32_t child,std::uint32_t actor);
    void BindEndAttack(std::function<void(std::uint32_t child)>);
    std::uint32_t Create(std::uint32_t factory_vtable);
    // Original recipe configures after linking, before BeginWaiting.
    void ConfigurePosition(std::uint32_t child,std::uint32_t actor,const std::array<float,3>&);
    void ConfigureResponse(std::uint32_t child,std::uint32_t source,std::uint32_t protector,std::uint32_t target);
    void ConfigureEndAttack(std::uint32_t child,std::uint32_t source,std::uint32_t target);
    std::uint32_t ConfigureApproach(std::uint32_t child,std::uint32_t actor,const std::array<float,3>&);
    void Destroy(std::uint32_t unlinked_child);
    void Close();
    IGomTaskBaseRuntime& BaseRuntime() noexcept;
    IGomTaskContainerRuntime& ContainerRuntime() noexcept;
    std::size_t Count() const noexcept;
    std::size_t OwnedAllocations() const noexcept;
private:
    NativeCombatActionChildren(Move,Protect,NativeBaseObjectMemory*);
    class Impl;
    std::unique_ptr<Impl> impl_;
};
}
