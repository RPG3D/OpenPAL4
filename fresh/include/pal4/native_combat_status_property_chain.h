#pragma once
#include "pal4/script_object_factory.h"
#include "pal4/script_property_chain.h"
#include "pal4/script_property_forwarders.h"
#include "pal4/actor_manager_property_state.h"
#include <cstdint>

namespace pal4 {
class NativeBaseObjectMemory;

// Borrows the actor allocator. Ownership remains in the original actor796/800
// and node4/8 handles; no second host list adopts or destroys those nodes.
class NativeCombatStatusPropertyChain final : public IScriptObjectFactoryRuntime,
    public IScriptPropertyChainRuntime, public IScriptPropertyForwardRuntime {
public:
    static constexpr std::uint32_t kEastBOneVtable=0x847730u;
    static constexpr std::uint32_t kEastBTwoVtable=0x8475d0u;
    static constexpr std::uint32_t kEastBThreeVtable=0x847470u;
    static constexpr std::uint32_t kEastBFourVtable=0x847310u;
    NativeCombatStatusPropertyChain(NativeBaseObjectMemory&,const std::uint32_t& base_vtable);
    void RequireRoot(std::uint32_t root) const;
    bool Contains(std::uint32_t root,std::uint32_t node) const;
    ActorPropertyHandle Create(std::uint32_t group,std::uint32_t variant);
    void AttachPrevious(std::uint32_t incoming,ActorPropertyHandle previous);
    void Destroy(std::uint32_t node);
    ScriptForwardResult ForwardNotification(std::uint32_t node,std::uint32_t slot,
        std::span<const std::uint32_t> arguments);
    std::uint32_t ReadWord(std::uint32_t) override;
    std::uint8_t ReadByte(std::uint32_t) override;
    void WriteWord(std::uint32_t,std::uint32_t) override;
    void WriteByte(std::uint32_t,std::uint8_t) override;
    std::uint32_t Allocate(std::uint32_t) override;
    std::uint32_t CallLink(std::uint32_t,std::uint32_t,std::int32_t,std::int32_t) override;
    std::int32_t CallValue(std::uint32_t,std::uint32_t,std::int32_t) override;
    ScriptForwardResult PreservedRegisters() override;
    ScriptForwardResult CallVirtual(std::uint32_t,std::uint32_t,
        std::span<const std::uint32_t>) override;
private:
    NativeBaseObjectMemory& memory_;
    const std::uint32_t& base_vtable_;
    void RequireNode(std::uint32_t) const;
};
}
