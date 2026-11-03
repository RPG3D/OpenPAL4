#include "pal4/monster_behavior_component.h"
#include <algorithm>
namespace pal4 {
namespace {
void WriteWord(MonsterBehaviorManagerState& state,std::uint32_t offset,std::uint32_t value){state.words[offset/4]=value;}
std::uint32_t ReadWord(const MonsterBehaviorManagerState& state,std::uint32_t offset){return state.words[offset/4];}
}
std::uint32_t MonsterBehaviorManager::SetComponentMembers(MonsterBehaviorManagerState& state,
                           std::uint32_t address,
                           std::uint32_t source_address,
                           bool dispatch_fields, bool replace,
                           IMonsterBehaviorComponentRuntime& runtime) {
    WriteWord(state, 0x654u, source_address);
    if (source_address != 0u) {
        const auto xml = runtime.SourceDocument(source_address);
        if (xml != 0u) {
            if (replace) runtime.ClearEventsAndTasks(state,address);
            ScriptEventProcessorState processor{};
            std::copy_n(state.words.begin()+kProcessorOffset/4,processor.words.size(),processor.words.begin());
            (void)ScriptEventProcessor::SetFirstValue28(processor, 0x594e00u);
            (void)ScriptEventProcessor::SetFlag24(processor, 1u);
            std::copy(processor.words.begin(),processor.words.end(),state.words.begin()+kProcessorOffset/4);
            WriteWord(state, 0x704u, 0u);
            const auto first_child = runtime.XmlChildByIndex(xml, 0u);
            WriteWord(state, 0x6d0u, first_child);
            runtime.LoadFuzzyBehaviorConfig(address, first_child);
            if (dispatch_fields) {
                runtime.DispatchComponentField(address, 4u,
                                               source_address + 0x18u);
                runtime.DispatchComponentField(address, 8u,
                                               source_address + 0x24u);
            }
        }
    }
    for (auto offset : {0x6e0u, 0x730u, 0x734u, 0x738u})
        WriteWord(state, offset, 0u);
    WriteWord(state,0x718u,ReadWord(state,0x714u));
    const auto begin=ReadWord(state,0x724u),end=ReadWord(state,0x728u);
    for(auto record=begin;record!=end;record+=16u)runtime.ClearHistoryRecord(record);
    WriteWord(state,0x728u,begin);return end;
}

}
