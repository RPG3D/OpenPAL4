#pragma once
#include <cstdint>
namespace pal4 {
class ICombatSystemStartRenderRuntime {
public:
    virtual ~ICombatSystemStartRenderRuntime()=default;
    virtual void WriteWord(std::uint32_t,std::uint32_t)=0;
    virtual std::uint32_t GetRenderCallback()=0;
    virtual void SetRenderCallback(std::uint32_t)=0;
};
class CombatSystemStartRender final {
public:
    static constexpr std::uint32_t kRenderCallback=0x574df0u;
    //5756D0 after the actual nonempty Player roster gate. Does not publish
    //state1/combat_prepare or reset audio; those remain the enclosing Start tail.
    static void Install(std::uint32_t object,ICombatSystemStartRenderRuntime&);
};
}
