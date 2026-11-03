#include "renderware_pipeline_edit_cases.h"
#include "pal4/camera_world_sector_pipeline_native.h"
#include "pal4/matfx_world_sector_pipeline_native.h"
#include <d3d9.h>
#include <array>
#include <cstring>
#include <cstdio>
#include <stdexcept>
#include <vector>

namespace pal4::test {
namespace {
void Require(bool ok, const char* message) { if (!ok) throw std::runtime_error(message); }
std::vector<int> events;
int fail_definition{}, fail_instance{}, fail_config{};
int Id(const RxNodeDefinition* def) { return def->name[0] - '0'; }
RwBool DefinitionInit(RxNodeDefinition* def) {
    const int id = Id(def); events.push_back(10 + id); return id != fail_definition;
}
void DefinitionTerm(RxNodeDefinition* def) { events.push_back(-10 - Id(def)); }
RwBool InstanceInit(RxPipelineNode* node) {
    const int id = Id(node->nodeDef); events.push_back(20 + id); return id != fail_instance;
}
void InstanceTerm(RxPipelineNode* node) { events.push_back(-20 - Id(node->nodeDef)); }
RwBool Configure(RxPipelineNode* node, RxPipeline*) {
    const int id = Id(node->nodeDef); events.push_back(30 + id); return id != fail_config;
}
RwBool Body(RxPipelineNode* node, const RxPipelineNodeParam*) {
    events.push_back(40 + Id(node->nodeDef)); return TRUE;
}
struct Definition {
    std::array<char, 2> name{};
    std::array<RxOutputSpec, 2> outputs{};
    RxNodeDefinition value{};
    Definition(int id, unsigned count) {
        name = {static_cast<char>('0' + id), 0};
        for (auto& out : outputs) out = {const_cast<char*>("next"), nullptr, rxCLVALID_NOCHANGE};
        value.name = name.data();
        value.nodeMethods = {Body, DefinitionInit, DefinitionTerm, InstanceInit, InstanceTerm, Configure, nullptr};
        value.io.numOutputs = count; value.io.outputs = outputs.data();
        value.editable = rxNODEDEFCONST;
    }
};
struct Pipeline {
    RxPipeline* value = RxPipelineCreate();
    Pipeline() { Require(value != nullptr, "editing pipeline allocation"); }
    ~Pipeline() { RxPipelineDestroy(value); }
    RxLockedPipe* Lock() { auto* out = RxPipelineLock(value); Require(out == value, "editing pipeline lock"); return out; }
    void Add(Definition& definition) {
        Require(RxLockedPipeAddFragment(value, nullptr, &definition.value,
                static_cast<RxNodeDefinition*>(nullptr)) == value, "editing add singleton");
    }
    RxPipelineNode* Find(const Definition& definition) {
        return RxPipelineFindNodeByName(value, definition.name.data(), nullptr, nullptr);
    }
};
void Reset() { events.clear(); fail_definition = fail_instance = fail_config = 0; }
void Connect(Pipeline& pipe, RxPipelineNode* from, unsigned slot, RxPipelineNode* to) {
    Require(RxLockedPipeAddPath(pipe.value, RxPipelineNodeFindOutputByIndex(from, slot),
            RxPipelineNodeFindInput(to)) == pipe.value, "editing path connection");
}
void Graph() {
    Reset(); Definition first(1, 1), middle(2, 1), last(3, 0); Pipeline pipe; pipe.Lock();
    // Reverse storage order forces Unlock's topological swaps and index fixups.
    pipe.Add(last); pipe.Add(middle); pipe.Add(first);
    Connect(pipe, pipe.Find(first), 0, pipe.Find(middle));
    Connect(pipe, pipe.Find(middle), 0, pipe.Find(last));
    Require(RxLockedPipeSetEntryPoint(pipe.value, RxPipelineNodeFindInput(pipe.Find(first))) == pipe.value,
            "set nonzero entrypoint");
    Require(RxLockedPipeUnlock(pipe.value) == pipe.value && !pipe.value->locked && pipe.value->entryPoint == 0,
            "unlock topological order");
    for (unsigned i = 0; i < 3; ++i) Require(Id(pipe.value->nodes[i].nodeDef) == static_cast<int>(i + 1), "sorted node identity");
    Require(pipe.value->nodes[0].outputs[0] == 1 && pipe.value->nodes[1].outputs[0] == 2,
            "sorted outputs follow moved nodes");
    Require(events == std::vector<int>({13, 23, 12, 22, 11, 21, 33, 32, 31}),
            "unlock initializes and configures in reverse topological order");
    events.clear(); Require(RxPipelineExecute(pipe.value, nullptr, TRUE) == pipe.value && events == std::vector<int>({41}),
            "Execute invokes entry only; nodes explicitly dispatch outputs");
    events.clear(); pipe.Lock();
    Require(events == std::vector<int>({-21, -11, -22, -12, -23, -13}), "lock terminates in forward order");
    const auto count = events.size(); pipe.Lock(); Require(events.size() == count, "repeated lock is idempotent");
    Require(pipe.value->nodes[0].outputs[0] == 1 && pipe.value->nodes[1].outputs[0] == 2,
            "relock preserves compiled graph connections");
    Require(RxLockedPipeUnlock(pipe.value) == pipe.value, "reunlock multi-node graph");
}
void Validation() {
    Reset(); Require(!RxLockedPipeUnlock(nullptr), "null unlock rejected");
    Definition source(1, 1), sink(2, 0); Pipeline pipe;
    Require(!RxLockedPipeUnlock(pipe.value), "unlocked input rejected");
    pipe.Lock(); Require(RxLockedPipeUnlock(pipe.value) == pipe.value, "empty locked pipeline unlocks");
    // Empty unlocked pipeline must not be executed: Execute dereferences node0.
    pipe.Lock(); pipe.Add(source); pipe.Add(sink);
    Require(!RxLockedPipeUnlock(pipe.value) && pipe.value->locked && events.empty(), "disconnected node rejected before callbacks");
    Connect(pipe, pipe.Find(source), 0, pipe.Find(sink));
    Require(RxLockedPipeUnlock(pipe.value) == pipe.value, "repair disconnected graph and retry");
    pipe.Lock();
    RwInt32 index = 99;
    Require(!RxPipelineFindNodeByName(pipe.value, "missing", nullptr, &index) && index == -1,
            "missing name resets output index");
    Require(!RxPipelineFindNodeByName(nullptr, source.name.data(), nullptr, &index) && index == -1,
            "null pipeline name lookup");
    Pipeline duplicate; duplicate.Lock();
    Definition same1(1, 1), same2(1, 0);
    RwUInt32 first = 99;
    Require(RxLockedPipeAddFragment(duplicate.value, &first, &same1.value, &same2.value,
            static_cast<RxNodeDefinition*>(nullptr)) == duplicate.value && first == 0,
            "variadic fragment links output0 and returns starting index");
    auto* a = RxPipelineFindNodeByName(duplicate.value, "1", nullptr, &index);
    auto* b = RxPipelineFindNodeByName(duplicate.value, "1", a, &index);
    Require(a == &duplicate.value->nodes[0] && b == &duplicate.value->nodes[1] && index == 1,
            "lookup resumes strictly after supplied node");
    Require(!RxPipelineFindNodeByName(duplicate.value, "1", b, &index) && index == -1,
            "lookup after last match fails");
    Pipeline rollback; rollback.Lock(); rollback.Add(source); first = 99;
    Require(!RxLockedPipeAddFragment(rollback.value, &first, &sink.value, &source.value,
            static_cast<RxNodeDefinition*>(nullptr)) && rollback.value->numNodes == 1 && first == 99,
            "fragment without predecessor output rolls back only new nodes");
    Definition too_many(3, 33);
    Require(!RxLockedPipeAddFragment(rollback.value, nullptr, &too_many.value,
            static_cast<RxNodeDefinition*>(nullptr)) && rollback.value->numNodes == 1,
            "more than32 outputs rejected before reading output array");
    Reset(); Definition root(1, 1), cycle_a(2, 1), cycle_b(3, 1); Pipeline cycle; cycle.Lock();
    cycle.Add(root); cycle.Add(cycle_a); cycle.Add(cycle_b);
    Connect(cycle, cycle.Find(root), 0, cycle.Find(cycle_a));
    Connect(cycle, cycle.Find(cycle_a), 0, cycle.Find(cycle_b));
    Connect(cycle, cycle.Find(cycle_b), 0, cycle.Find(cycle_a));
    Require(!RxLockedPipeUnlock(cycle.value) && cycle.value->locked && events.empty(),
            "reachable internal cycle rejected before callbacks");
}
void Dependencies() {
    Reset();
    RxClusterDefinition first{const_cast<char*>("first"), 4, 1, nullptr};
    RxClusterDefinition second{const_cast<char*>("second"), 4, 2, nullptr};
    std::array<RxClusterRef, 2> refs{{{&first, rxCLFORCEPRESENT, 0},
                                      {&second, rxCLFORCEPRESENT, 0}}};
    std::array<RxClusterValidityReq, 2> needs{{rxCLREQ_DONTWANT, rxCLREQ_DONTWANT}};
    Definition node(1, 0);
    node.value.io.numClustersOfInterest = static_cast<RwUInt32>(refs.size());
    node.value.io.clustersOfInterest = refs.data();
    node.value.io.inputRequirements = needs.data();

    {
        Pipeline pipe; pipe.Lock(); pipe.Add(node);
        Require(RxLockedPipeUnlock(pipe.value) == pipe.value &&
                    pipe.value->packetNumClusterSlots == 2,
                "distinct cluster definitions compile to two slots");
        const auto& compiled = pipe.value->nodes[0];
        Require(compiled.inputToClusterSlot && compiled.slotClusterRefs &&
                    compiled.inputToClusterSlot[0] != compiled.inputToClusterSlot[1],
                "node interests map to distinct packet slots");
        Require(compiled.slotClusterRefs[compiled.inputToClusterSlot[0]]->clusterRef == &first &&
                    compiled.slotClusterRefs[compiled.inputToClusterSlot[1]]->clusterRef == &second,
                "compiled slot references retain cluster identity");
    }
    {
        refs[1].clusterDef = &first;
        Pipeline pipe; pipe.Lock(); pipe.Add(node);
        Require(!RxLockedPipeUnlock(pipe.value) && pipe.value->locked,
                "duplicate cluster definitions rejected during dependency chase");
        RwError error{};
        RwErrorGet(&error);
        Require(error.errorCode == 30, "duplicate cluster error comes from dependency chase");
    }
    {
        refs[1].clusterDef = nullptr;
        Pipeline pipe; pipe.Lock(); pipe.Add(node);
        Require(!RxLockedPipeUnlock(pipe.value) && pipe.value->locked,
                "null cluster definition rejected during dependency chase");
        RwError error{};
        RwErrorGet(&error);
        Require(error.errorCode == 31, "null cluster error comes from dependency chase");
    }
}
void Callbacks() {
    for (bool config : {false, true}) {
        Reset(); Definition def(1, 0); Pipeline pipe; pipe.Lock(); pipe.Add(def);
        if (config) fail_config = 1; else fail_instance = 1;
        Require(!RxLockedPipeUnlock(pipe.value) && pipe.value->locked && def.value.InputPipesCnt == 0,
                "instance/config failure leaves locked pipeline and releases definition count");
        Require(events == (config ? std::vector<int>{11, 21, 31, -21, -11} : std::vector<int>{11, 21, -11}),
                "rollback only terminates successfully initialized instances");
        Reset(); Require(RxLockedPipeUnlock(pipe.value) == pipe.value && def.value.InputPipesCnt == 1 &&
                events == std::vector<int>({11, 21, 31}), "retry after callback rollback");
    }
    for (bool config : {false, true}) {
        Reset(); Definition source(1, 1), sink(2, 0); Pipeline pipe; pipe.Lock();
        Require(RxLockedPipeAddFragment(pipe.value, nullptr, &source.value, &sink.value,
                static_cast<RxNodeDefinition*>(nullptr)) == pipe.value, "rollback two-node chain");
        if (config) fail_config = 1; else fail_instance = 1;
        Require(!RxLockedPipeUnlock(pipe.value) && pipe.value->locked &&
                source.value.InputPipesCnt == 0 && sink.value.InputPipesCnt == 0,
                "failure releases initialized suffix of multi-node chain");
        Require(events == (config ? std::vector<int>{12, 22, 11, 21, 32, 31, -21, -11, -22, -12}
                                  : std::vector<int>{12, 22, 11, 21, -11, -22, -12}),
                "multi-node rollback order and initialized range");
    }
}
void DefinitionFailure() {
    Reset(); Definition def(1, 0); Pipeline pipe; pipe.Lock(); pipe.Add(def); fail_definition = 1;
    Require(!RxLockedPipeUnlock(pipe.value) && pipe.value->locked && events == std::vector<int>({11}),
            "definition callback failure stops initialization");
    Require(def.value.InputPipesCnt == 1, "diagnostic: failed definition retains incremented count");
    events.clear(); fail_definition = 0;
    Require(RxLockedPipeUnlock(pipe.value) == pipe.value && events == std::vector<int>({21, 31}) &&
            def.value.InputPipesCnt == 2, "diagnostic: retry skips definition initialization because of stale count");
    std::puts("diagnostic: failed node definition init retains reference and is skipped on retry");
    // The const fixture definition outlives pipe. No global SDK definition is modified.
}
void Allocation(bool& fail_next) {
    Reset(); Definition def(1, 0); Pipeline pipe;
    fail_next = true;
    Require(!RxPipelineLock(pipe.value) && !fail_next && !pipe.value->locked && !pipe.value->nodes,
            "initial lock allocation failure retains empty pipeline");
    pipe.Lock(); pipe.Add(def); fail_next = true;
    Require(!RxLockedPipeUnlock(pipe.value) && !fail_next && pipe.value->locked && events.empty(),
            "unlock resize failure occurs before callbacks and stays locked");
    Require(RxLockedPipeUnlock(pipe.value) == pipe.value && events == std::vector<int>({11, 21, 31}),
            "unlock allocation retry for single-node graph");
}
void Render(RwResEntry*, void*, RwUInt8, RwUInt32) {}
void AtomicNode() {
    auto* def = RxNodeDefinitionGetD3D9AtomicAllInOne();
    Require(def && std::strstr(def->name, "D3D9AtomicAllInOne") &&
                def->pipelineNodePrivateDataSize == 16,
            "SDK atomic all-in-one descriptor and private callback slots");
    const int before = def->InputPipesCnt;
    Pipeline pipe;
    pipe.Lock();
    Require(RxLockedPipeAddFragment(pipe.value, nullptr, def,
                static_cast<RxNodeDefinition*>(nullptr)) == pipe.value,
            "add SDK atomic node");
    Require(RxLockedPipeUnlock(pipe.value) == pipe.value && !pipe.value->locked,
            "unlock SDK atomic pipeline");
    auto* node = RxPipelineFindNodeByName(pipe.value, def->name, nullptr, nullptr);
    Require(node && node->privateData &&
                RxD3D9AllInOneGetInstanceCallBack(node) &&
                RxD3D9AllInOneGetReinstanceCallBack(node) &&
                RxD3D9AllInOneGetLightingCallBack(node) &&
                RxD3D9AllInOneGetRenderCallBack(node),
            "SDK atomic node installs four default callbacks");
    Require(def->InputPipesCnt == before + 1, "atomic node acquired definition");
}
RwBool MatFxInstance(void*, RxD3D9ResEntryHeader*, RwBool) { return TRUE; }
void MatFxWorld(bool& fail_next) {
    bool rejected = false;
    try { NativeMatFxWorldSectorPipelineRuntime invalid(nullptr, Render); }
    catch (const std::invalid_argument&) { rejected = true; }
    Require(rejected, "MatFX factory requires real instance callback binding");
    NativeMatFxWorldSectorPipelineRuntime runtime(MatFxInstance, Render);
    auto* def = RxNodeDefinitionGetD3D9WorldSectorAllInOne();
    const int before = def->InputPipesCnt;
    const auto mipmapping = RwTextureGetMipmapping();
    RwTextureSetMipmapping(TRUE);
    auto* pipeline = reinterpret_cast<RxPipeline*>(MatFxWorldSectorPipeline::Create(runtime));
    Require(pipeline && !pipeline->locked && pipeline->numNodes == 1 &&
            def->InputPipesCnt == before + 1 && !RwTextureGetMipmapping(),
            "53E4C0 creates world-sector node and disables mipmapping");
    auto* node = RxPipelineFindNodeByName(pipeline, def->name, nullptr, nullptr);
    Require(node && RxD3D9AllInOneGetInstanceCallBack(node) == MatFxInstance &&
            RxD3D9AllInOneGetRenderCallBack(node) == Render,
            "53E550 and 53E990 identities bind typed host instance/render callbacks");
    RxD3D9AllInOneRenderCallBack installed{};
    std::memcpy(&installed, static_cast<const char*>(node->privateData) + 12, sizeof installed);
    Require(installed == Render, "original private+12 is render slot");
    rejected = false;
    try { runtime.SetRenderCallback(reinterpret_cast<std::uint32_t>(node), 0u); }
    catch (const std::invalid_argument&) { rejected = true; }
    Require(rejected && RxD3D9AllInOneGetRenderCallBack(node) == Render,
            "unknown callback identity leaves installed render binding intact");
    runtime.DestroyPipeline(reinterpret_cast<std::uint32_t>(pipeline));
    Require(def->InputPipesCnt == before, "MatFX pipeline releases descriptor ownership");
    RwTextureSetMipmapping(TRUE);
    fail_next = true;
    Require(!MatFxWorldSectorPipeline::Create(runtime) && !fail_next &&
            !RwTextureGetMipmapping() && def->InputPipesCnt == before,
            "failed creation preserves original mipmapping side effect without acquiring node");
    RwTextureSetMipmapping(mipmapping);
}
void Game(bool& fail_next) {
    NativeCameraWorldSectorPipelineRuntime runtime(Render);
    const auto* caps = static_cast<const D3DCAPS9*>(RwD3D9GetCaps());
    Require(caps->MaxSimultaneousTextures > 1, "device must exercise game capability success path");
    auto* def = RxNodeDefinitionGetD3D9WorldSectorAllInOne();
    Require(std::strcmp(def->name, "nodeD3D9WorldSectorAllInOne.csl") == 0 &&
            def->pipelineNodePrivateDataSize == 16, "original world-sector descriptor identity and private size");
    const int before = def->InputPipesCnt;
    auto* pipeline = reinterpret_cast<RxPipeline*>(CameraWorldSectorPipeline::Create(runtime));
    Require(pipeline && !pipeline->locked && pipeline->numNodes == 1 && def->InputPipesCnt == before + 1,
            "actual game creates SDK world-sector pipeline");
    auto* node = RxPipelineFindNodeByName(pipeline, def->name, nullptr, nullptr);
    Require(node && node->privateData, "actual world-sector node private data initialized");
    // Original setter783AB0 stores the render callback at privateData+12.
    RxD3D9AllInOneRenderCallBack installed{};
    std::memcpy(&installed, static_cast<const char*>(node->privateData) + 12, sizeof installed);
    Require(installed == Render && RxD3D9AllInOneGetRenderCallBack(node) == Render,
            "game identity token binds host render callback with public getter confirmation");
    runtime.DestroyPipeline(reinterpret_cast<std::uint32_t>(pipeline));
    Require(def->InputPipesCnt == before, "game custom world-sector pipeline lifetime restores reference count");
    fail_next = true;
    Require(!CameraWorldSectorPipeline::Create(runtime) && !fail_next && def->InputPipesCnt == before,
            "actual game pipeline allocation failure returns before node acquisition");
}
}
void RegisterPipelineEditWorldPlugin() {
    Require(RpWorldPluginAttach(), "attach SDK world plugin");
}
void RunPipelineEditCase(std::string_view name, bool& fail_next) {
    if (name == "pipeline_edit_graph") Graph();
    else if (name == "pipeline_edit_validation") Validation();
    else if (name == "pipeline_edit_dependencies") Dependencies();
    else if (name == "pipeline_edit_callbacks") Callbacks();
    else if (name == "pipeline_edit_node_failure") DefinitionFailure();
    else if (name == "pipeline_edit_allocation") Allocation(fail_next);
    else if (name == "pipeline_edit_game") Game(fail_next);
    else if (name == "pipeline_edit_matfx_world") MatFxWorld(fail_next);
    else if (name == "pipeline_edit_atomic_node") AtomicNode();
    else throw std::runtime_error("unknown pipeline edit case");
}
}
