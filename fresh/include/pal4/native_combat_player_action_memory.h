#pragma once
#include <cstddef>
#include <cstdint>
#include <memory>
namespace pal4 {
class NativeCombatPlayerManager;
class NativeCombatTaskQueue;
class INativeCombatTaskDomain;
// Owns actual Player1312/1316 actions through existing53A4A0 assignment.
// Domains and actual actors outlive this owner. Clear before their retirement.
// Queue tasks and these replay templates have distinct ownership.
class NativeCombatPlayerActionMemory final {
public:
    NativeCombatPlayerActionMemory(NativeCombatPlayerManager&,NativeCombatTaskQueue&);
    ~NativeCombatPlayerActionMemory();
    void Assign(std::uint32_t actor,std::uint32_t action,INativeCombatTaskDomain&);
    std::uint32_t Get(std::uint32_t actor) const;
    void Clear();
    std::size_t Count() const noexcept;
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};
}
