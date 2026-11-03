#include "pal4/game_manager_components.h"
#include "pal4/binary_xml_config.h"
#include "pal4/native_base_object_member_runtime.h"
#include "pal4/native_base_object_memory.h"
#include "pal4/native_event_document_runtime.h"
#include "pal4/native_game_manager_component_runtime.h"
#include "pal4/native_game_object_animation_lifecycle.h"
#include "pal4/native_monster_behavior_tree_runtime.h"
#include "pal4/native_npc_object_owner.h"
#include "pal4/native_npc_behavior_config.h"
#include "pal4/native_behavior_tasks.h"
#include "pal4/native_script_object_runtime.h"
#include "support/rw_dict_schema_fake.h"

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <new>
#include <array>
#include <stdexcept>
#include <vector>
#include <unordered_set>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::fprintf(stderr, "FAIL: %s\n", message);
        std::exit(EXIT_FAILURE);
    }
}

class FailingMemory final : public pal4::IStringBufferStorageRuntime {
public:
    explicit FailingMemory(pal4::NativeBaseObjectMemory& memory,
                           std::uint32_t fail_at = 6u)
        : memory_(memory), fail_at_(fail_at) {}
    std::uint32_t Allocate(std::uint32_t bytes) override {
        if (++allocations == fail_at_) throw std::bad_alloc{};
        return memory_.Allocate(bytes);
    }
    std::uint32_t Free(std::uint32_t address) override {
        return memory_.Free(address);
    }
    std::uint32_t ReadDword(std::uint32_t address) override {
        return memory_.ReadDword(address);
    }
    void WriteDword(std::uint32_t address,
                    std::uint32_t value) override {
        memory_.WriteDword(address, value);
    }
    void WriteByte(std::uint32_t address,
                   std::uint8_t value) override {
        memory_.WriteByte(address, value);
    }
    std::uint32_t allocations = 0u;
private:
    pal4::NativeBaseObjectMemory& memory_;
    std::uint32_t fail_at_;
};

class EmptyAnimationRuntime final :
    public pal4::NativeGameObjectAnimationLifecycle {
public:
    using NativeGameObjectAnimationLifecycle::NativeGameObjectAnimationLifecycle;

    std::uint32_t FindHierarchy(std::uint32_t) override {
        throw std::logic_error("unloaded NPC has no animation hierarchy");
    }
    void ProcessAnimationData(pal4::GameObjectEmbeddedAnimationState&,
                              std::uint32_t, std::int32_t) override {
        throw std::logic_error("unloaded NPC has no animation data");
    }
};

class EmptySchemaRuntime final : public pal4::test::RwDictSchemaFake {
public:
    std::uint32_t SetCurrentDict(std::uint32_t dict) override {
        Check(dict == 0u, "empty NPC base clears its resource dictionaries");
        ++resets;
        return 0u;
    }
    std::uint32_t resets = 0u;
};

// Observes the real scheduler/task images at the required helper boundary.
// Engine grid/animation helpers are deliberately not simulated by this fixture.
class TaskHelpers final : public pal4::INativeBehaviorHelpers {
public:
    TaskHelpers(pal4::NativeBaseObjectMemory& memory,std::uint32_t owner):memory(memory),owner(owner){}
    bool Supports(std::int32_t kind) const override{return kind>=0 && kind<=4;}
    void Construct(std::int32_t kind,std::uint32_t helper) override {
        std::memset(reinterpret_cast<void*>(helper),0,kind==2?0xbcu:kind==4?0xc0u:0xe0u);
        ++constructed[kind];
    }
    void Discard(std::int32_t,std::uint32_t helper) override {Check(!live.contains(helper),"failed helper must remain unaccepted");}
    void Accept(std::int32_t kind,std::uint32_t target,std::uint32_t helper) override {
        Check(target==owner+0x6b0u+8u*kind,"NPC five-class binder selects its actual target subobject");
        if(fail_accept){fail_accept=false;throw std::runtime_error("behavior helper acceptance rejected");}
        Check(live.insert(helper).second,"helper accepted once");
        memory.WriteByte(helper+0xb8u,1u);++accepted[kind];
    }
    void Update(std::int32_t kind,std::uint32_t,std::uint32_t helper) override {
        Check(live.contains(helper),"scheduler updates only live helpers");++updated[kind];
    }
    void Release(std::int32_t kind,std::uint32_t,std::uint32_t helper) override {
        Check(live.erase(helper)==1u,"helper released exactly once");(void)memory.Free(helper);++released[kind];
    }
    bool IsComplete(std::int32_t kind,std::uint32_t,std::uint32_t helper) override {
        Check(kind==2 && live.contains(helper),"only class2 asks target completion");return complete;
    }
    pal4::NativeBaseObjectMemory& memory;
    std::uint32_t owner;
    std::array<unsigned,5> constructed{},accepted{},updated{},released{};
    std::unordered_set<std::uint32_t> live;
    bool complete{};
    bool fail_accept{};
};

void Walk(const std::uint32_t node, const std::uint32_t nil,
          pal4::NativeMonsterBehaviorTreeRuntime& runtime,
          std::vector<std::uint32_t>& ids) {
    if (node == nil) return;
    Walk(runtime.ReadDword(node), nil, runtime, ids);
    ids.push_back(runtime.ReadDword(node + 12u));
    Walk(runtime.ReadDword(node + 8u), nil, runtime, ids);
}

}  // namespace

int main() {
    pal4::NativeBaseObjectMemory memory;
    EmptySchemaRuntime schema;
    EmptyAnimationRuntime animation(memory);
    pal4::NativeBaseObjectMemberRuntime base(
        memory, schema, animation, memory, memory.Path());
    pal4::NativeEventDocumentRuntime document(memory);
    pal4::NativeScriptObjectRuntime script(memory);
    pal4::NativeMonsterBehaviorTreeRuntime tree(memory);
    pal4::NativeGameManagerComponentRuntime runtime(
        base, document, script, tree);
    const auto address = memory.Allocate(
        sizeof(pal4::GameManagerComponentState));
    std::memset(reinterpret_cast<void*>(static_cast<std::uintptr_t>(address)),
                0, sizeof(pal4::GameManagerComponentState));
    auto& state = *reinterpret_cast<pal4::GameManagerComponentState*>(
        static_cast<std::uintptr_t>(address));
    const auto constructed = pal4::GameManagerComponents::Initialize(
        state, address, 0u, runtime);
    Check(constructed == address &&
              state.words[0u] == 0x842dc0u &&
              state.words[0x6dcu / 4u] == 0x849398u &&
              state.words[0x6d4u / 4u] == address &&
              state.words[0x6a8u / 4u] == 5u &&
              state.words[0x218u / 4u] == 0x84b8c8u &&
              state.words[0x170u / 4u] == 0x842db0u &&
              state.words[0x544u / 4u] != 0u &&
              state.words[0x554u / 4u] != 0u &&
              state.words[0x564u / 4u] ==
                  pal4::CollisionActor::kVtableAddress &&
              document.DocumentCount() == 1u &&
              document.SharedNil() != 0u &&
              memory.OutstandingAllocations() == 15u,
          "native NPC component constructor owns base, document, task and tree");
    const auto event_document = state.words[
        (pal4::GameManagerComponents::kEventProcessorOffset + 16u) / 4u];
    Check(event_document != 0u &&
              std::strcmp(reinterpret_cast<const char*>(
                  static_cast<std::uintptr_t>(
                      memory.ReadDword(event_document + 4u))),
                  "behAIAtomicMgr-memory") == 0 &&
              memory.ReadDword(event_document + 24u) != 0u,
          "NPC event processor owns the initialized original XML document");
    std::vector<std::uint32_t> ids;
    const auto header = state.words[0x6a0u / 4u];
    Walk(tree.ReadDword(header + 4u), tree.SharedNil(), tree, ids);
    Check(ids == std::vector<std::uint32_t>{0u, 1u, 2u, 3u, 4u},
          "native NPC constructor registers all five behavior classes");
    const auto initial_allocations=memory.OutstandingAllocations();
    pal4::BinaryXmlNode disabled{"NPCINFO_Behavior"};
    pal4::LoadNativeNpcBehaviorConfig(address,&disabled,document);
    Check(memory.OutstandingAllocations()==initial_allocations,
        "default NPC group3 creates no native events");
    pal4::BinaryXmlNode fuzzy{"NPCINFO_Behavior"};
    fuzzy.attributes.push_back({"NPCINFO_Behavior_GroupMode",1u,std::int32_t(2)});
    pal4::BinaryXmlNode atomic{"NPCINFO_Behavior_Object"};
    atomic.attributes={{"NPCINFO_Behavior_ObjectType",1u,std::int32_t(4)},
        {"NPCINFO_Behavior_ObjectProportion",1u,std::int32_t(37)},
        {"NPCINFO_Behavior_ObjectWaitMode",1u,std::int32_t(1)},
        {"NPCINFO_Behavior_ObjectWaitTime",2u,0.75f},
        {"NPCINFO_Behavior_ObjectExpectTime",2u,2.5f}};
    fuzzy.children.push_back(atomic);
    pal4::LoadNativeNpcBehaviorConfig(address,&fuzzy,document);
    auto& processor=*reinterpret_cast<pal4::ScriptEventProcessorState*>(address+0x658u);
    const auto event=*reinterpret_cast<const std::uint32_t*>(processor.words[1]);
    Check(event && memory.ReadDword(event+16u)==2u &&
        memory.ReadDword(event+28u)-memory.ReadDword(event+24u)==60u,
        "NPC config creates one owned native atomic event with original group mode");
    const auto node=memory.ReadDword(event+24u);
    Check(document.AttributeToInt(document.FindNumericAttribute(node,"behAIAtomic-classID"),-1)==4 &&
        document.AttributeToInt(document.FindNumericAttribute(node,"behAIAtomic-proportion"),-1)==37 &&
        document.AttributeToInt(document.FindNumericAttribute(node,"beh-vtm-waitMode"),-1)==1 &&
        document.AttributeToFloat(document.FindNumericAttribute(node,"beh-vtm-waitTime"),-1.f)==0.75f &&
        document.AttributeToFloat(document.FindNumericAttribute(node,"beh-vtm-expectTime"),-1.f)==2.5f,
        "NPC native atomic documents preserve class, proportion and wait parameters");
    (void)pal4::ScriptEventProcessor::ClearEvents(processor,document);
    Check(processor.words[1]==processor.words[2] &&
        memory.OutstandingAllocations()==initial_allocations+1u,
        "NPC fuzzy clear releases event/documents and retains the processor pointer capacity");
    auto multiple = fuzzy;
    multiple.children = {atomic, atomic, atomic};
    pal4::LoadNativeNpcBehaviorConfig(address, &multiple, document);
    const auto multiple_event = memory.ReadDword(processor.words[1]);
    const auto multiple_begin = memory.ReadDword(multiple_event + 24u);
    Check(memory.ReadDword(multiple_event + 28u) - multiple_begin == 3u * 60u &&
              memory.ReadDword(multiple_event + 32u) - multiple_begin == 4u * 60u,
        "NPC behavior appends initialized records through original geometric array growth (IDA42D230/42DA10)");
    for (std::uint32_t index = 0; index < 3u; ++index) {
        const auto record = multiple_begin + index * 60u;
        Check(document.NodeName(record) == "behAIAtomic-attribute" &&
                  document.AttributeToInt(document.FindNumericAttribute(record,
                      "behAIAtomic-classID"), -1) == 4 &&
                  document.AttributeToInt(document.FindNumericAttribute(record,
                      "behAIAtomic-proportion"), -1) == 37 &&
                  document.AttributeToFloat(document.FindNumericAttribute(record,
                      "beh-vtm-expectTime"), -1.f) == 2.5f,
            "behavior node copies retain their names, attributes and wait parameters");
    }
    (void)pal4::ScriptEventProcessor::ClearEvents(processor, document);
    Check(memory.OutstandingAllocations() == initial_allocations + 1u,
        "incremental behavior construction releases copied nodes and transient documents");
    auto partial = multiple;
    partial.children[2].attributes[2].value = std::int32_t(0);
    pal4::LoadNativeNpcBehaviorConfig(address, &partial, document);
    const auto partial_event = memory.ReadDword(processor.words[1]);
    Check(memory.ReadDword(partial_event + 16u) == 3u &&
              memory.ReadDword(partial_event + 28u) - memory.ReadDword(partial_event + 24u) == 120u,
        "invalid later NPC rule retains its already appended prefix and unpublished group mode");
    (void)pal4::ScriptEventProcessor::ClearEvents(processor, document);
    Check(memory.OutstandingAllocations() == initial_allocations + 1u,
        "partially loaded NPC event remains safely owned by the processor");
    pal4::GameManagerComponents::Cleanup(state, address, runtime);
    Check(schema.resets == 3u &&
              document.DocumentCount() == 0u &&
              document.SharedNil() == 0u &&
              tree.SharedNil() == 0u &&
              tree.SharedReferenceCount() == 0u &&
              memory.OutstandingAllocations() == 1u,
          "native NPC component cleanup releases owned subobjects");
    (void)memory.Free(address);
    Check(memory.OutstandingAllocations() == 0u,
          "NPC object storage releases after component cleanup");

    FailingMemory failing_memory(memory);
    pal4::NativeEventDocumentRuntime failing_document(memory);
    pal4::NativeScriptObjectRuntime failing_script(failing_memory);
    pal4::NativeMonsterBehaviorTreeRuntime failing_tree(failing_memory);
    pal4::NativeGameManagerComponentRuntime failing_runtime(
        base, failing_document, failing_script, failing_tree);
    const auto failed_address = memory.Allocate(sizeof(state));
    std::memset(reinterpret_cast<void*>(
                    static_cast<std::uintptr_t>(failed_address)),
                0, sizeof(state));
    auto& failed_state = *reinterpret_cast<pal4::GameManagerComponentState*>(
        static_cast<std::uintptr_t>(failed_address));
    bool allocation_failed = false;
    try {
        (void)pal4::GameManagerComponents::Initialize(
            failed_state, failed_address, 0u, failing_runtime);
    } catch (const std::bad_alloc&) {
        allocation_failed = true;
    }
    Check(allocation_failed && failing_memory.allocations == 6u &&
              schema.resets == 6u &&
              failing_document.DocumentCount() == 0u &&
              failing_document.SharedNil() == 0u &&
              failing_tree.SharedNil() == 0u &&
              failing_tree.SharedReferenceCount() == 0u &&
              memory.OutstandingAllocations() == 1u,
          "partial NPC factory registration unwinds all subobjects");
    (void)memory.Free(failed_address);
    Check(memory.OutstandingAllocations() == 0u,
          "failed NPC constructor leaves no native allocations");

    for (std::uint32_t fail_at = 1u; fail_at <= 4u; ++fail_at) {
        FailingMemory failed_xml_memory(memory, fail_at);
        pal4::NativeEventDocumentRuntime failed_xml(failed_xml_memory);
        pal4::ScriptEventProcessorState processor{};
        bool xml_failed = false;
        try {
            (void)pal4::ScriptEventProcessor::Initialize(
                processor, 0x1000u, 0u, failed_xml);
        } catch (const std::bad_alloc&) {
            xml_failed = true;
        }
        Check(xml_failed && failed_xml_memory.allocations == fail_at &&
                  failed_xml.DocumentCount() == 0u &&
                  failed_xml.SharedNil() == 0u &&
                  memory.OutstandingAllocations() == 0u,
              "each event XML allocation failure releases its owned memory");
    }

    pal4::NativeEventDocumentRuntime shared_xml(memory);
    pal4::ScriptEventProcessorState first{};
    pal4::ScriptEventProcessorState second{};
    (void)pal4::ScriptEventProcessor::Initialize(
        first, 0x1000u, 0u, shared_xml);
    const auto shared_nil = shared_xml.SharedNil();
    (void)pal4::ScriptEventProcessor::Initialize(
        second, 0x2000u, 0u, shared_xml);
    Check(shared_xml.DocumentCount() == 2u &&
              shared_xml.SharedNil() == shared_nil,
          "two event documents reuse the XML tree nil");
    (void)pal4::ScriptEventProcessor::Cleanup(first, shared_xml);
    Check(shared_xml.DocumentCount() == 1u &&
              shared_xml.SharedNil() == shared_nil,
          "first event document leaves the shared nil alive");
    (void)pal4::ScriptEventProcessor::Cleanup(second, shared_xml);
    Check(shared_xml.DocumentCount() == 0u &&
              shared_xml.SharedNil() == 0u &&
              memory.OutstandingAllocations() == 0u,
          "last event document releases the shared nil");

    pal4::NativeNpcObjectOwner owner(memory, schema, animation);
    const auto owned_address = owner.Construct();
    Check(owned_address != 0u && owner.Address() == owned_address &&
              memory.OutstandingAllocations() == 15u,
          "NPC component owner retains the complete native object image");
    owner.BindBorrowedActor(0x12345000u, 0x12346000u);
    pal4::GameResourceManagerRuntimeStub transform_runtime;
    owner.SetTransform({1.0f, 2.0f, 3.0f},
                       {0.0f, 90.0f, 0.0f}, transform_runtime);
    owner.SetStateFlag(7u);
    Check(owner.ActorBound() &&
              memory.ReadDword(owned_address + 0x14u) == 0x12345000u &&
              memory.ReadDword(owned_address + 0x30u) == 0x12346000u &&
              memory.ReadDword(owned_address + 0x24u) == 1u &&
              memory.ReadDword(owned_address + 0x174u) == 0x12345000u &&
              memory.ReadDword(owned_address + 0xacu) == 0x3f800000u &&
              memory.ReadDword(owned_address + 0xb0u) == 0x40000000u &&
              memory.ReadDword(owned_address + 0xb4u) == 0x40400000u &&
              memory.ReadDword(owned_address + 0xa4u) == 0x42b40000u &&
              memory.ReadDword(owned_address + 0x1f0u) == 7u,
          "NPC object borrows clumps and applies its transform and state");
    bool duplicate_actor_rejected = false;
    try {
        owner.BindBorrowedActor(0x12347000u, 0u);
    } catch (const std::logic_error&) {
        duplicate_actor_rejected = true;
    }
    Check(duplicate_actor_rejected &&
              memory.ReadDword(owned_address + 0x14u) == 0x12345000u,
          "duplicate NPC actor binding leaves the original clump intact");
    constexpr std::array<pal4::NativeNpcAnimationClip, 2> clips{{
        {"C01", 0x12341000u}, {"J01", 0x12342000u}}};
    owner.BindBorrowedAnimations("gamedata\\PALActor\\201J\\", clips);
    const auto records = memory.ReadDword(owned_address + 0x178u);
    Check(records != 0u && memory.ReadDword(records - 4u) == 2u &&
              memory.ReadDword(records + 32u) == clips[0].animation &&
              memory.ReadDword(records + 0x44u + 32u) ==
                  clips[1].animation &&
              std::strcmp(reinterpret_cast<const char*>(
                  static_cast<std::uintptr_t>(
                      memory.ReadDword(records + 4u))),
                  "gamedata\\PALActor\\201J\\") == 0 &&
              std::strcmp(reinterpret_cast<const char*>(
                  static_cast<std::uintptr_t>(
                      memory.ReadDword(records + 0x44u + 20u))),
                  "J01") == 0 &&
              memory.ReadDword(owned_address + 0x17cu) == records &&
              owner.SelectBorrowedAction("J01") &&
              memory.ReadDword(owned_address + 0x17cu) == records + 0x44u &&
              !owner.SelectBorrowedAction("missing"),
          "NPC embedded animation records borrow clips and select the default action");
    bool duplicate_animation_rejected = false;
    try {
        owner.BindBorrowedAnimations("duplicate", clips);
    } catch (const std::logic_error&) {
        duplicate_animation_rejected = true;
    }
    Check(duplicate_animation_rejected,
          "NPC cannot replace a live borrowed animation array");
    owner.BindManagerGrid(0x12348000u);
    Check(memory.ReadDword(owned_address + 0x204u) == 0x12348000u,
          "NPC object stores its manager grid member reference");
    const auto npc_record = memory.Allocate(84u);
    memory.WriteDword(npc_record + 76u, 0x12349000u);
    pal4::BinaryXmlConfig empty_behavior;
    owner.SetupBehavior(npc_record, empty_behavior, transform_runtime);
    Check(memory.ReadDword(owned_address + 0x654u) == npc_record &&
              memory.ReadDword(owned_address + 0x674u) == 0x594e00u &&
              memory.ReadByte(owned_address + 0x670u) == 1u &&
              memory.ReadDword(owned_address + 0x6d8u) == 0u,
          "empty NPC behavior sets source and enables the event processor");
    pal4::BinaryXmlNode behavior_root{"NPCINFO_Behavior"};behavior_root.children.push_back(fuzzy);
    const auto behavior_bytes=pal4::BinaryXmlConfig::Serialize(behavior_root);
    pal4::BinaryXmlConfig active_behavior;
    Check(behavior_bytes && active_behavior.Parse(*behavior_bytes),"configured NPC behavior fixture parses");
    owner.SetupBehavior(npc_record,active_behavior,transform_runtime,true);
    const auto behavior_allocations=memory.OutstandingAllocations();
    Check(memory.ReadDword(owned_address+0x660u)-memory.ReadDword(owned_address+0x65cu)==4u,
        "NPC resume builds one real event from the retained original document");
    owner.PauseBehavior(transform_runtime);
    Check(memory.ReadDword(owned_address+0x654u)==npc_record &&
        memory.ReadDword(owned_address+0x6d8u)==0u &&
        memory.ReadDword(owned_address+0x660u)==memory.ReadDword(owned_address+0x65cu),
        "NPC pause clears events/child while retaining original source record and actor");
    owner.SetupBehavior(npc_record,active_behavior,transform_runtime,true);
    owner.SetupBehavior(npc_record,active_behavior,transform_runtime,true);
    Check(memory.OutstandingAllocations()==behavior_allocations &&
        memory.ReadDword(owned_address+0x6d8u)!=0u &&
        memory.ReadDword(owned_address+0x660u)-memory.ReadDword(owned_address+0x65cu)==4u,
        "repeated NPC resume clears previous event and rebuilds without duplicates or allocation growth");
    owner.PauseBehavior(transform_runtime);
    auto helpers=std::make_shared<TaskHelpers>(memory,owned_address);
    owner.BindBehaviorHelpers(helpers);
    for(std::int32_t kind=0;kind<5;++kind){
        auto selected=fuzzy;
        for(auto& attribute:selected.children.front().attributes){
            if(attribute.name=="NPCINFO_Behavior_ObjectType")attribute.value=kind;
            else if(attribute.name=="NPCINFO_Behavior_ObjectWaitMode")attribute.value=std::int32_t(2);
            else if(attribute.name=="NPCINFO_Behavior_ObjectWaitTime")attribute.value=.25f;
            else if(attribute.name=="NPCINFO_Behavior_ObjectExpectTime")attribute.value=.5f;
        }
        pal4::BinaryXmlNode root{"NPCINFO_Behavior"};root.children.push_back(selected);
        const auto bytes=pal4::BinaryXmlConfig::Serialize(root);
        Check(bytes && active_behavior.Parse(*bytes),"five-class behavior config parses");
        owner.SetupBehavior(npc_record,active_behavior,transform_runtime,true);
        helpers->complete=false;
        unsigned motion_calls{};
        const auto tick=[&](float dt){return owner.ProcessWorldFrame(dt,0u,[&](float,std::uint32_t){++motion_calls;return 7u;});};
        Check(tick(.1f)==7u && helpers->constructed[kind]==0u,
            "actual NPC scheduler creates a waiting task before constructing its helper");
        for(unsigned frame=0;frame<12u && !helpers->accepted[kind];++frame)(void)tick(.1f);
        Check(helpers->accepted[kind]==1u && motion_calls>1u,"all five shared task factories begin after real Gom wait");
        (void)tick(1.f);
        if(kind==2){
            Check(helpers->released[kind]==0u,"class2 completion follows its target rather than elapsed duration");
            helpers->complete=true;(void)tick(.1f);
        }
        Check(helpers->updated[kind]>0u && helpers->released[kind]==1u && helpers->live.empty(),
            "native class run/finish releases helper through required services");
        owner.PauseBehavior(transform_runtime);
        owner.SetupBehavior(npc_record,active_behavior,transform_runtime,true);
        if(kind==4){
            helpers->fail_accept=true;(void)tick(.1f);
            const auto waiting_allocations=memory.OutstandingAllocations();
            bool failed=false;
            try{for(unsigned frame=0;frame<12u;++frame)(void)tick(.1f);}
            catch(const std::runtime_error& error){failed=std::string_view(error.what())=="behavior helper acceptance rejected";}
            const auto sentinel=memory.ReadDword(owned_address+0x68cu);
            const auto task=memory.ReadDword(memory.ReadDword(sentinel)+8u);
            Check(failed && helpers->live.empty() && memory.OutstandingAllocations()==waiting_allocations &&
                memory.ReadDword(task+0x40u)==0u,
                "failed generic helper acceptance frees unaccepted storage and leaves a retryable task without dangling pointer");
        }
        for(unsigned frame=0;frame<12u && helpers->accepted[kind]<2u;++frame)(void)tick(.1f);
        Check(helpers->accepted[kind]==2u,"restarted behavior accepts another helper");
        owner.PauseBehavior(transform_runtime);
        Check(helpers->released[kind]==2u && helpers->live.empty(),
            "pause cancels each active task and releases its helper exactly once");
    }
    owner.UnbindBehavior();
    Check(memory.ReadDword(owned_address + 0x654u) == 0u &&
              memory.ReadDword(owned_address + 0x674u) == 0u &&
              memory.ReadByte(owned_address + 0x670u) == 0u,
          "NPC behavior drops its source before manager teardown");
    (void)memory.Free(npc_record);
    owner.UnbindManagerGrid();
    Check(memory.ReadDword(owned_address + 0x204u) == 0u,
          "NPC object detaches from its manager before manager teardown");
    owner.Destroy();
    Check(owner.Address() == 0u && !owner.ActorBound() &&
              schema.resets == 9u &&
              memory.OutstandingAllocations() == 0u,
          "NPC component owner releases its object and nested resources");
    bool unconstructed_actor_rejected = false;
    try {
        owner.BindBorrowedActor(0x12345000u, 0u);
    } catch (const std::logic_error&) {
        unconstructed_actor_rejected = true;
    }
    Check(unconstructed_actor_rejected,
          "NPC actor binding requires a live component image");
    return EXIT_SUCCESS;
}
