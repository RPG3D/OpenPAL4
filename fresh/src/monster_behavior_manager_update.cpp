#include "pal4/monster_behavior_manager.h"
#include <algorithm>
#include <bit>
namespace pal4 {
namespace {
std::uint32_t ReadWord(const MonsterBehaviorManagerState& s,std::uint32_t offset){return s.words[offset/4];}
void WriteWord(MonsterBehaviorManagerState& s,std::uint32_t offset,std::uint32_t value){s.words[offset/4]=value;}
ScriptEventProcessorState ProcessorState(const MonsterBehaviorManagerState& s){
    ScriptEventProcessorState p{};std::copy_n(s.words.begin()+MonsterBehaviorManager::kProcessorOffset/4,p.words.size(),p.words.begin());return p;
}
void StoreProcessor(MonsterBehaviorManagerState& s,const ScriptEventProcessorState& p){
    std::copy(p.words.begin(),p.words.end(),s.words.begin()+MonsterBehaviorManager::kProcessorOffset/4);
}
}
std::uint32_t MonsterBehaviorManager::Update(
    MonsterBehaviorManagerState& state, std::uint32_t address,
    float elapsed, std::uint32_t argument,
    IMonsterBehaviorFrameRuntime& runtime) {
    const auto active = ReadWord(state, 0x1f0u);
    if (active == 0u)
        return active;

    if (ReadWord(state, 0x730u) == 1u) {
        const auto accumulated = static_cast<double>(elapsed) +
            std::bit_cast<float>(ReadWord(state, 0x738u));
        WriteWord(state, 0x738u,
                  std::bit_cast<std::uint32_t>(
                      static_cast<float>(accumulated)));
        const auto limit = std::bit_cast<float>(ReadWord(state, 0x734u));
        if (accumulated > static_cast<double>(limit)) {
            WriteWord(state, 0x738u, ReadWord(state, 0x734u));
            WriteWord(state, 0x730u, 0u);
            runtime.CheckStateAndProcessCollision(state, address, elapsed);
        }
    } else if (runtime.ProcessManagerState(state, address, elapsed) == 0u) {
        auto processor = ProcessorState(state);
        try {
            (void)ScriptEventProcessor::HandleEventProcessing(
                processor, elapsed, address + kScriptOffset,
                address + 0x698u, address + 0x6acu, 0u,
                runtime.SchedulerRuntime());
        } catch (...) {
            // The native ABI mutates this embedded image in place. Retain
            // owned candidate buffers when an unbound service throws.
            StoreProcessor(state, processor);
            throw;
        }
        StoreProcessor(state, processor);
    }
    return runtime.UpdatePosition(address, elapsed, argument);
}

}
