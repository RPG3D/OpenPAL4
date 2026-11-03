#include "pal4/combat_ending_window_level_up.h"

#include <string>

namespace pal4 {

void ICombatEndingWindowLevelUpRuntime::VisitLearnedNames(std::uint32_t list,std::uint32_t actor,std::int32_t level,const LearnedNameVisitor& visit){
    for(auto cursor=ReadWord(list+4u);cursor!=ReadWord(list+8u);cursor+=4u){
        const auto record=ReadWord(cursor),slot=ReadWord(actor+0x4a8u);
        if(static_cast<std::int32_t>(ReadWord(record+0x38u+4u*slot))==level)visit(ReadText(ReadWord(record+0xcu)));
    }
}

void CombatEndingWindowLevelUp::Apply(
    const std::uint32_t score_row, const std::int32_t new_level,
    const std::uint32_t actor, const std::uint32_t hint_window,
    const std::uint32_t actor_list,
    ICombatEndingWindowLevelUpRuntime& runtime) {
    if (new_level <= static_cast<std::int32_t>(
                         runtime.ReadWord(score_row + 0x48u))) return;

    runtime.WriteWord(score_row + 0x48u,
                      static_cast<std::uint32_t>(new_level));
    runtime.SetWindowText(runtime.ReadWord(score_row + 4u),
                          std::to_string(new_level));

    const auto actor_name = runtime.ReadText(runtime.ReadWord(actor + 0x45cu));
    auto notice = runtime.FormatLevelNotice(
        runtime.GameString(0xcau), actor_name, new_level);
    bool has_matching_actor=false;
    runtime.VisitLearnedNames(actor_list,actor,new_level,[&](std::string_view name){has_matching_actor=true;notice+="<br/>";notice+=runtime.FormatActorNotice(runtime.GameString(0xb8u),name);});

    runtime.SetHintFlag(hint_window, 0u);
    runtime.ShowHint(hint_window, notice, score_row+64u,3.0f, 0u, 0u);
    runtime.RegisterActorEvent(runtime.CombatLevelUp(actor));
    runtime.PlaySound("WA18.wav", 1u);
    if (has_matching_actor) runtime.PlaySound("WA17.wav", 1u);
    const auto effect = runtime.SpawnEffect(actor, "EFF17", 0x8fa274u);
    runtime.ResetEffectHandle(effect);
}

}  // namespace pal4
