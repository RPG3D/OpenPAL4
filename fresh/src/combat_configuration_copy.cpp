#include "pal4/script_combat_config.h"
namespace pal4 {
std::uint8_t CombatSystemPropertyCopy::CopyConfiguration(const CombatConfigSnapshot& source,ICombatConfigurationCopyServices& target){
    for(std::uint32_t i=0;i<5u;++i)target.CopyWord(i,source.words[i]);
    for(std::uint32_t i=0;i<3u;++i)target.CopyText(i);
    for(std::uint32_t i=0;i<3u;++i)target.CopyFlag(i,source.flags[i]);
    return source.flags[2];
}
}
