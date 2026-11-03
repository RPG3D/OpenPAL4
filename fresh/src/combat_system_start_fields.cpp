#include "pal4/combat_system_start.h"
namespace pal4 {
void CombatSystemStart::InitializeRequestFields(std::uint32_t object,
    std::uint32_t mode,ICombatSystemStartFieldsRuntime& runtime) {
    const auto previous_flag=runtime.ReadByte(object+501u);
    const auto preserve_flag=runtime.ReadByte(object+502u);
    runtime.WriteByte(object+516u,static_cast<std::uint8_t>(mode));
    runtime.WriteWord(object+512u,0u);
    runtime.WriteWord(object+508u,7u);
    runtime.WriteWord(object+504u,1u);
    runtime.WriteWord(object+388u,0u);
    runtime.WriteWord(object+392u,0u);
    runtime.WriteByte(object+518u,0u);
    runtime.WriteByte(object+519u,previous_flag);
    if(preserve_flag==0u)runtime.WriteByte(object+501u,0u);
}
}
