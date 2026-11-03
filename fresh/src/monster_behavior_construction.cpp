#include "pal4/monster_behavior_construction.h"
#include "pal4/monster_combat_parameter_callback.h"
namespace pal4 {
namespace {
void WriteWord(MonsterBehaviorManagerState& state,std::uint32_t offset,std::uint32_t value){state.words[offset/4]=value;}
void WriteByte(MonsterBehaviorManagerState& state,std::uint32_t offset,std::uint8_t value){const auto shift=(offset&3u)*8u;auto& word=state.words[offset/4];word=(word&~(0xffu<<shift))|(std::uint32_t(value)<<shift);}
struct CallbackStateWriter final : IMonsterCombatParameterCallbackRuntime {
    MonsterBehaviorManagerState& state;
    std::uint32_t base;
    CallbackStateWriter(MonsterBehaviorManagerState& state,
                        std::uint32_t base)
        : state(state), base(base) {}
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        state.words[(address - base) / 4u] = value;
    }
};

}
std::uint32_t MonsterBehaviorManager::ConstructMembers(
    MonsterBehaviorManagerState& state, std::uint32_t address,
    IMonsterBehaviorConstructionRuntime& runtime) {
    int stage = -1;
    try {
        runtime.ConstructBase(state, address);
        stage = 0;

        runtime.ConstructProcessor(state, address + kProcessorOffset);
        stage = 1;

        runtime.ConstructScript(state, address + kScriptOffset);
        stage = 2;

        const auto tree_bytes = runtime.TreeInputBytes();
        WriteByte(state, kTreeOffset, tree_bytes.first);
        WriteByte(state, kTreeOffset + 1u, tree_bytes.second);
        WriteByte(state, kTreeOffset + 8u, 0u);
        runtime.InitializeTree(state, address + kTreeOffset);
        WriteWord(state, 0x698u, 0x842d64u);
        WriteWord(state, 0x6b0u, 0x842d58u);
        WriteWord(state, 0x6b8u, 0x842d4cu);
        WriteWord(state, 0x6c0u, 0x842d3cu);
        WriteWord(state, 0x6c8u, 0x842d2cu);
        WriteWord(state, 0x6acu, 0x842d28u);
        WriteWord(state, 0x6b0u, 0x842d1cu);
        WriteWord(state, 0x6b8u, 0x842d10u);
        WriteWord(state, 0x6c0u, 0x842d00u);
        WriteWord(state, 0x6c8u, 0x842cf0u);
        stage = 3;

        CallbackStateWriter callback_writer(state, address);
        (void)MonsterCombatParameterCallback::Construct(
            address + kAuxiliaryOffset, callback_writer);
        stage = 4;
        WriteByte(state, 0x710u, tree_bytes.second);
        WriteWord(state, 0x714u, 0u);
        WriteWord(state, 0x718u, 0u);
        WriteWord(state, 0x71cu, 0u);
        WriteByte(state, 0x720u, tree_bytes.second);
        WriteWord(state, 0x724u, 0u);
        WriteWord(state, 0x728u, 0u);
        WriteWord(state, 0x72cu, 0u);
        stage = 6;

        WriteWord(state, 0x000u, 0x842c98u);
        WriteWord(state, 0x170u, 0x842c8cu);
        WriteWord(state, 0x1ecu, 0x842c80u);
        WriteWord(state, 0x204u, 0u);
        WriteWord(state, 0x654u, 0u);
        WriteWord(state, 0x6ccu, address);
        WriteWord(state, 0x6dcu, address);
        RegisterMonsterBehaviorFactories(address + kTreeOffset, runtime);

        WriteWord(state, 0x6e0u, 0u);
        WriteWord(state, 0x6e4u, 0u);
        WriteWord(state, 0x6f8u, 0x3f800000u);
        WriteWord(state, 0x6fcu, 0x3f800000u);
        for (auto offset : {0x700u, 0x704u, 0x708u, 0x70cu,
                            0x730u, 0x734u, 0x738u})
            WriteWord(state, offset, 0u);
        return address;
    } catch (...) {
        if (stage >= 6) {
            runtime.CleanupHistory(state, address + 0x720u);
            runtime.CleanupWordArray(state, address + 0x710u);
        }
        if (stage >= 4)
            runtime.DestroyAuxiliary(state, address + kAuxiliaryOffset);
        if (stage >= 3)
            runtime.CleanupTree(state, address + 0x698u);
        if (stage >= 2)
            runtime.DestructScript(state, address + kScriptOffset);
        if (stage >= 1)
            runtime.DestructProcessor(state, address + kProcessorOffset);
        if (stage >= 0)
            runtime.DestructBase(state, address);
        throw;
    }
}

}
