#include "pal4/monster_behavior_manager.h"
#include "pal4/monster_behavior_construction.h"
#include "pal4/monster_behavior_component.h"

#include <algorithm>
#include <bit>

namespace pal4 {
namespace {

void WriteWord(MonsterBehaviorManagerState& state, std::uint32_t offset,
               std::uint32_t value) {
    state.words[offset / 4u] = value;
}

std::uint32_t ReadWord(const MonsterBehaviorManagerState& state,
                       std::uint32_t offset) {
    return state.words[offset / 4u];
}


void WriteByte(MonsterBehaviorManagerState& state, std::uint32_t offset,
               std::uint8_t value) {
    const auto shift = (offset & 3u) * 8u;
    const auto mask = 0xffu << shift;
    auto& word = state.words[offset / 4u];
    word = (word & ~mask) | (static_cast<std::uint32_t>(value) << shift);
}

ScriptEventProcessorState ProcessorState(
    const MonsterBehaviorManagerState& state) {
    ScriptEventProcessorState processor{};
    std::copy_n(state.words.begin() +
                    MonsterBehaviorManager::kProcessorOffset / 4u,
                processor.words.size(), processor.words.begin());
    return processor;
}

void StoreProcessor(MonsterBehaviorManagerState& state,
                    const ScriptEventProcessorState& processor) {
    std::copy(processor.words.begin(), processor.words.end(),
              state.words.begin() +
                  MonsterBehaviorManager::kProcessorOffset / 4u);
}

std::uint32_t ClearTransientArrays(MonsterBehaviorManagerState& state,
                                   IMonsterPursuitEntryRuntime& runtime) {
    WriteWord(state, 0x718u, ReadWord(state, 0x714u));
    const auto begin = ReadWord(state, 0x724u);
    const auto end = ReadWord(state, 0x728u);
    for (auto record = begin; record != end; record += 16u) {
        MonsterBehaviorManager::ClearHistoryRecord(record, runtime);
    }
    WriteWord(state, 0x728u, begin);
    return end;
}

std::uint32_t SetComponent(MonsterBehaviorManagerState& state,std::uint32_t address,
    std::uint32_t source,bool dispatch,bool replace,IMonsterBehaviorManagerRuntime& runtime) {
    struct Adapter final:IMonsterBehaviorComponentRuntime {
        IMonsterBehaviorManagerRuntime& runtime;
        explicit Adapter(IMonsterBehaviorManagerRuntime& value):runtime(value){}
        std::uint32_t SourceDocument(std::uint32_t source) override {return runtime.ReadDword(source+0x48u);}
        void ClearEventsAndTasks(MonsterBehaviorManagerState& state,std::uint32_t address) override {
            auto processor=ProcessorState(state);(void)ScriptEventProcessor::ClearEvents(processor,runtime.ProcessorRuntime());
            runtime.CleanupScriptObjectCollection(address+MonsterBehaviorManager::kScriptOffset);StoreProcessor(state,processor);
        }
        std::uint32_t XmlChildByIndex(std::uint32_t document,std::uint32_t index) override {return runtime.XmlChildByIndex(document,index);}
        void LoadFuzzyBehaviorConfig(std::uint32_t address,std::uint32_t child) override {runtime.LoadFuzzyBehaviorConfig(address,child);}
        void DispatchComponentField(std::uint32_t address,std::uint32_t slot,std::uint32_t field) override {runtime.DispatchComponentField(address,slot,field);}
        void ClearHistoryRecord(std::uint32_t record) override {MonsterBehaviorManager::ClearHistoryRecord(record,runtime);}
    } adapter(runtime);
    return MonsterBehaviorManager::SetComponentMembers(state,address,source,dispatch,replace,adapter);
}

}  // namespace

void MonsterBehaviorManager::ClearHistoryRecord(
    std::uint32_t record, IMonsterPursuitEntryRuntime& runtime) {
    if (runtime.ReleaseNativeHistoryRecord(record)) return;
    const auto data = runtime.ReadDword(record + 4u);
    if (data != 0u) {
        const auto reference = runtime.ReadByte(data - 1u);
        if (reference == 0u || reference == 0xffu) {
            runtime.FreeStringStorage(record, data - 1u,
                                       runtime.ReadDword(record + 12u) + 2u);
        } else {
            runtime.WriteByte(data - 1u,
                              static_cast<std::uint8_t>(reference - 1u));
        }
    }
    for (auto offset : {4u, 8u, 12u})
        runtime.WriteDword(record + offset, 0u);
}

IScriptEventProcessorRuntime& MonsterBehaviorManagerRuntimeStub::ProcessorRuntime() {
    return processor_;
}
IScriptObjectRuntime& MonsterBehaviorManagerRuntimeStub::ScriptRuntime() {
    return script_;
}
std::uint8_t MonsterBehaviorManagerRuntimeStub::ProcessorIncomingByte() {
    return 0u;
}
MonsterBehaviorTreeBytes MonsterBehaviorManagerRuntimeStub::TreeInputBytes() {
    return {};
}
void MonsterBehaviorManagerRuntimeStub::ConstructBase(
    MonsterBehaviorManagerState&, std::uint32_t) {}
void MonsterBehaviorManagerRuntimeStub::DestructBase(
    MonsterBehaviorManagerState&, std::uint32_t) {}
void MonsterBehaviorManagerRuntimeStub::InitializeTree(
    MonsterBehaviorManagerState&, std::uint32_t) {}
void MonsterBehaviorManagerRuntimeStub::CleanupTree(
    MonsterBehaviorManagerState&, std::uint32_t) {}
void MonsterBehaviorManagerRuntimeStub::DestroyAuxiliary(
    MonsterBehaviorManagerState&, std::uint32_t) {}
void MonsterBehaviorManagerRuntimeStub::CleanupWordArray(
    MonsterBehaviorManagerState&, std::uint32_t) {}
void MonsterBehaviorManagerRuntimeStub::CleanupHistory(
    MonsterBehaviorManagerState&, std::uint32_t) {}
void MonsterBehaviorManagerRuntimeStub::InsertBehavior(
    std::uint32_t, std::uint32_t, std::uint32_t) {}
void MonsterBehaviorManagerRuntimeStub::SetRecordErrorFlag(
    std::uint32_t, std::uint8_t) {}
void MonsterBehaviorManagerRuntimeStub::FreeMemory(std::uint32_t) {}
std::uint32_t MonsterBehaviorManagerRuntimeStub::ReadDword(
    std::uint32_t) { return 0u; }
void MonsterBehaviorManagerRuntimeStub::WriteDword(
    std::uint32_t, std::uint32_t) {}
std::uint32_t MonsterBehaviorManagerRuntimeStub::TreeFirst(
    std::uint32_t) { return 0u; }
std::uint32_t MonsterBehaviorManagerRuntimeStub::TreeNext(
    std::uint32_t) { return 0u; }
std::uint32_t MonsterBehaviorManagerRuntimeStub::TreeNilAddress() { return 0u; }
void MonsterBehaviorManagerRuntimeStub::TreeEraseSubtree(
    std::uint32_t, std::uint32_t) {}
void MonsterBehaviorManagerRuntimeStub::TreeRemoveNode(
    std::uint32_t, std::uint32_t) {}
void MonsterBehaviorManagerRuntimeStub::ReleaseSharedTreeNil() {}
std::uint8_t MonsterBehaviorManagerRuntimeStub::ReadByte(
    std::uint32_t) { return 0u; }
void MonsterBehaviorManagerRuntimeStub::WriteByte(
    std::uint32_t, std::uint8_t) {}
void MonsterBehaviorManagerRuntimeStub::FreeStringStorage(
    std::uint32_t, std::uint32_t, std::uint32_t) {}
std::uint32_t MonsterBehaviorManagerRuntimeStub::ResetBaseObject(
    MonsterBehaviorManagerState&, std::uint32_t) { return 0u; }
std::uint32_t MonsterBehaviorManagerRuntimeStub::ProcessManagerState(
    MonsterBehaviorManagerState& state, std::uint32_t address,
    float elapsed) {
    MonsterPursuitRuntimeStub pursuit;
    return MonsterBehaviorManager::UpdatePursuitState(
        state, address, elapsed, pursuit) ? 1u : 0u;
}
void MonsterBehaviorManagerRuntimeStub::CheckStateAndProcessCollision(
    MonsterBehaviorManagerState&, std::uint32_t, float) {}
IScriptEventSchedulerRuntime& MonsterBehaviorManagerRuntimeStub::SchedulerRuntime() {
    return scheduler_;
}
std::uint32_t MonsterBehaviorManagerRuntimeStub::UpdatePosition(
    std::uint32_t, float, std::uint32_t) { return 0u; }
void MonsterBehaviorManagerRuntimeStub::CleanupScriptObjectCollection(
    std::uint32_t) {}
std::uint32_t MonsterBehaviorManagerRuntimeStub::XmlChildByIndex(
    std::uint32_t, std::uint32_t) { return 0u; }
void MonsterBehaviorManagerRuntimeStub::LoadFuzzyBehaviorConfig(
    std::uint32_t manager_address, std::uint32_t child_address) {
    MonsterFuzzyBehaviorLoader::Load(manager_address, child_address, fuzzy_);
}
void MonsterBehaviorManagerRuntimeStub::DispatchComponentField(
    std::uint32_t, std::uint32_t, std::uint32_t) {}
std::uint32_t MonsterBehaviorManagerRuntimeStub::GetConfigAttribute(
    std::uint32_t, const char*) { return 0u; }
std::int32_t MonsterBehaviorManagerRuntimeStub::ParseConfigInt(
    std::uint32_t, std::int32_t fallback) { return fallback; }
std::uint32_t MonsterBehaviorManagerRuntimeStub::CallActionSlot4(
    std::uint32_t) { return 0u; }
std::uint32_t MonsterBehaviorManagerRuntimeStub::SelectAnimation(
    std::uint32_t, const char*) { return 0u; }
std::uint32_t MonsterBehaviorManagerRuntimeStub::ProcessCollisionAndConfig(
    std::uint32_t, float, float) { return 0u; }

std::uint32_t MonsterBehaviorManager::Construct(
    MonsterBehaviorManagerState& state,std::uint32_t address,
    IMonsterBehaviorManagerRuntime& runtime) {
    struct LegacyConstruction final:IMonsterBehaviorConstructionRuntime {
        IMonsterBehaviorManagerRuntime& runtime;
        explicit LegacyConstruction(IMonsterBehaviorManagerRuntime& value):runtime(value){}
        void ConstructBase(MonsterBehaviorManagerState& s,std::uint32_t a) override {runtime.ConstructBase(s,a);}
        void DestructBase(MonsterBehaviorManagerState& s,std::uint32_t a) override {runtime.DestructBase(s,a);}
        void ConstructProcessor(MonsterBehaviorManagerState& s,std::uint32_t a) override {
            auto processor=ProcessorState(s);(void)ScriptEventProcessor::Initialize(processor,a,runtime.ProcessorIncomingByte(),runtime.ProcessorRuntime());StoreProcessor(s,processor);
        }
        void DestructProcessor(MonsterBehaviorManagerState& s,std::uint32_t) override {
            auto processor=ProcessorState(s);(void)ScriptEventProcessor::Cleanup(processor,runtime.ProcessorRuntime());StoreProcessor(s,processor);
        }
        void ConstructScript(MonsterBehaviorManagerState&,std::uint32_t a) override {(void)ScriptObject::Initialize(a,runtime.ScriptRuntime());}
        void DestructScript(MonsterBehaviorManagerState&,std::uint32_t a) override {(void)ScriptObject::Destroy(a,runtime.ScriptRuntime());}
        MonsterBehaviorTreeBytes TreeInputBytes() override {return runtime.TreeInputBytes();}
        void InitializeTree(MonsterBehaviorManagerState& s,std::uint32_t a) override {runtime.InitializeTree(s,a);}
        void CleanupTree(MonsterBehaviorManagerState& s,std::uint32_t a) override {runtime.CleanupTree(s,a);}
        void DestroyAuxiliary(MonsterBehaviorManagerState& s,std::uint32_t a) override {runtime.DestroyAuxiliary(s,a);}
        void CleanupWordArray(MonsterBehaviorManagerState& s,std::uint32_t a) override {runtime.CleanupWordArray(s,a);}
        void CleanupHistory(MonsterBehaviorManagerState& s,std::uint32_t a) override {runtime.CleanupHistory(s,a);}
        void InsertBehavior(std::uint32_t a,std::uint32_t id,std::uint32_t factory) override {runtime.InsertBehavior(a,id,factory);}
    } adapter(runtime);
    return ConstructMembers(state,address,adapter);
}

void MonsterBehaviorManager::Destroy(
    MonsterBehaviorManagerState& state, std::uint32_t address,
    IMonsterBehaviorManagerRuntime& runtime) {
    WriteWord(state, 0x000u, 0x842c98u);
    WriteWord(state, 0x170u, 0x842c8cu);
    WriteWord(state, 0x1ecu, 0x842c80u);

    const auto records_begin = ReadWord(state, 0x724u);
    const auto records_end = ReadWord(state, 0x728u);
    for (auto record = records_begin; record != records_end; record += 16u)
        runtime.SetRecordErrorFlag(record, 1u);
    runtime.FreeMemory(records_begin);
    for (auto offset : {0x724u, 0x728u, 0x72cu})
        WriteWord(state, offset, 0u);

    runtime.FreeMemory(ReadWord(state, 0x714u));
    for (auto offset : {0x714u, 0x718u, 0x71cu})
        WriteWord(state, offset, 0u);
    runtime.DestroyAuxiliary(state, address + kAuxiliaryOffset);

    WriteWord(state, 0x698u, 0x842d64u);
    const auto tree = address + kTreeOffset;
    const auto header = ReadWord(state, 0x6a0u);
    auto current = runtime.ReadDword(header);
    if (ReadWord(state, 0x6a8u) != 0u &&
        current == runtime.TreeFirst(tree) &&
        header == ReadWord(state, 0x6a0u)) {
        runtime.TreeEraseSubtree(tree, runtime.ReadDword(header + 4u));
        runtime.WriteDword(header + 4u, runtime.TreeNilAddress());
        WriteWord(state, 0x6a8u, 0u);
        runtime.WriteDword(header, header);
        runtime.WriteDword(header + 8u, header);
        (void)runtime.TreeFirst(tree);
    } else {
        while (current != header) {
            const auto next = runtime.TreeNext(current);
            runtime.TreeRemoveNode(tree, current);
            current = next;
        }
    }
    runtime.FreeMemory(header);
    WriteWord(state, 0x6a0u, 0u);
    WriteWord(state, 0x6a8u, 0u);
    runtime.ReleaseSharedTreeNil();

    (void)ScriptObject::Destroy(address + kScriptOffset,
                                runtime.ScriptRuntime());
    auto processor = ProcessorState(state);
    (void)ScriptEventProcessor::Cleanup(processor, runtime.ProcessorRuntime());
    StoreProcessor(state, processor);
    runtime.DestructBase(state, address);
}

std::uint32_t MonsterBehaviorManager::Reset(
    MonsterBehaviorManagerState& state, std::uint32_t address,
    IMonsterBehaviorManagerRuntime& runtime) {
    (void)CleanupBuffers(state, address, true, runtime);
    (void)ClearTransientArrays(state, runtime);
    return runtime.ResetBaseObject(state, address);
}

std::uint32_t MonsterBehaviorManager::CleanupBuffers(
    MonsterBehaviorManagerState& state, std::uint32_t address,
    bool notify, IMonsterBehaviorManagerRuntime& runtime) {
    const auto source = ReadWord(state, 0x654u);
    if (source != 0u) {
        auto processor = ProcessorState(state);
        (void)ScriptEventProcessor::ClearEvents(
            processor, runtime.ProcessorRuntime());
        StoreProcessor(state, processor);
        runtime.CleanupScriptObjectCollection(address + kScriptOffset);
        (void)ScriptEventProcessor::SetFlag24(processor, 0u);
        StoreProcessor(state, processor);
        WriteWord(state, 0x704u, 0u);
        if (notify)
            runtime.DispatchComponentField(address, 4u, source + 0x18u);
        WriteWord(state, 0x6d0u, 0u);
    }
    for (auto offset : {0x6e0u, 0x730u, 0x734u, 0x738u})
        WriteWord(state, offset, 0u);
    return ClearTransientArrays(state, runtime);
}

std::uint32_t MonsterBehaviorManager::InitializeComponent(
    MonsterBehaviorManagerState& state, std::uint32_t address,
    std::uint32_t source_address, bool dispatch_fields,
    IMonsterBehaviorManagerRuntime& runtime) {
    return SetComponent(state, address, source_address, dispatch_fields,
                        false, runtime);
}

std::uint32_t MonsterBehaviorManager::ReplaceComponent(
    MonsterBehaviorManagerState& state, std::uint32_t address,
    std::uint32_t source_address, bool dispatch_fields,
    IMonsterBehaviorManagerRuntime& runtime) {
    return SetComponent(state, address, source_address, dispatch_fields,
                        true, runtime);
}

std::uint32_t MonsterBehaviorManager::ProcessStorageAction(
    MonsterBehaviorManagerState& state, std::uint32_t action_address,
    IMonsterBehaviorManagerRuntime& runtime) {
    const auto source = ReadWord(state, 0x654u);
    if (source == 0u)
        return 0u;
    const auto config = runtime.ReadDword(source + 0x4cu);
    const auto attribute = runtime.GetConfigAttribute(
        config, "PAL4_MST_NEED_STORAGE");
    const auto required = attribute != 0u
        ? runtime.ParseConfigInt(attribute, 0) : 0;
    if (required <= 0)
        return static_cast<std::uint32_t>(required);
    const auto action_result = runtime.CallActionSlot4(action_address);
    if (action_result != 11u)
        return action_result;
    const auto value = runtime.ReadDword(action_address + 0x14u);
    WriteWord(state, 0x1f0u, value);
    return value;
}

std::uint32_t MonsterBehaviorManager::SetStateAndCleanup(
    MonsterBehaviorManagerState& state, std::uint32_t address,
    std::uint32_t argument, IMonsterPursuitEntryRuntime& runtime) {
    const auto current = ReadWord(state, 0x6e0u);
    if (current == 1u || current == 2u) return current;

    WriteWord(state, 0x6e0u, 1u);
    WriteWord(state, 0x6e8u, ReadWord(state, 0xacu));
    WriteWord(state, 0x6ecu, ReadWord(state, 0xb0u));
    WriteWord(state, 0x6f0u, ReadWord(state, 0xb4u));
    WriteWord(state, 0x6fcu, ReadWord(state, 0x6f8u));
    WriteWord(state, 0x6e4u, argument);
    (void)ClearTransientArrays(state, runtime);

    auto processor = ProcessorState(state);
    (void)ScriptEventProcessor::ClearEvents(
        processor, runtime.ProcessorRuntime());
    StoreProcessor(state, processor);
    runtime.CleanupScriptObjectCollection(address + kScriptOffset);
    (void)ScriptEventProcessor::SetFlag24(processor, 0u);
    StoreProcessor(state, processor);

    const auto animation = address + 0x170u;
    const auto result = runtime.SelectAnimation(animation, "Z02");
    return result != 0u ? result
        : runtime.SelectAnimation(animation, "C03");
}


}  // namespace pal4
