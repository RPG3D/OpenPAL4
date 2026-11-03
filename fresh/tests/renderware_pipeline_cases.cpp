#include "renderware_pipeline_cases.h"
#include <rwcore.h>
#include <array>
#include <cstring>
#include <stdexcept>
#include <vector>

namespace pal4::test {
namespace {
void Require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}
static_assert(sizeof(RxPipeline) == 52 && sizeof(RxPipelineNode) == 40);
static_assert(sizeof(RxCluster) == 28 && offsetof(RxPacket, clusters) == 20);
std::vector<int> events;
RwBool NodeInit(RxNodeDefinition*) { events.push_back(1); return TRUE; }
void NodeTerm(RxNodeDefinition*) { events.push_back(-1); }
RwBool InstanceInit(RxPipelineNode*) { events.push_back(2); return TRUE; }
void InstanceTerm(RxPipelineNode*) { events.push_back(-2); }
RwBool Config(RxPipelineNode*, RxPipeline*) { events.push_back(3); return TRUE; }
struct Execution {
    bool result = true;
    bool dirty{};
    bool packet{};
    int calls{};
    std::array<unsigned, 4> borrowed{10, 20, 30, 40};
};
RwBool Body(RxPipelineNode* node, const RxPipelineNodeParam* params) {
    auto& state = *static_cast<Execution*>(params->dataParam);
    Require(params->heap == RxHeapGetGlobalHeap(), "execution heap routing");
    state.dirty = params->heap->dirty != FALSE;
    ++state.calls;
    if (state.packet) {
        auto* packet = RxPacketCreate(node);
        Require(packet && packet->pipeline->embeddedPacket == packet &&
                packet->inputToClusterSlot == node->inputToClusterSlot &&
                packet->slotsContinue == node->slotsContinue &&
                packet->slotClusterRefs == node->slotClusterRefs, "embedded packet node tables");
        Require(packet->pipeline->embeddedPacketState == rxPKST_INUSE, "packet create state");
        auto* cluster = RxClusterLockWrite(packet, 0, node);
        Require(cluster && cluster->attributes == 0x1357 &&
                (cluster->flags & rxCLFLAGS_MODIFIED), "first write resolves reference and attributes");
        Require(RxClusterSetExternalData(cluster, state.borrowed.data(), 4, 4) == cluster,
                "external cluster setup");
        Require(cluster->data == state.borrowed.data() && cluster->numUsed == 4 &&
                cluster->numAlloced == 4, "external cluster is borrowed");
        cluster->currentData = state.borrowed.data() + 2;
        Require(RxClusterLockWrite(packet, 0, node) == cluster &&
                cluster->data != state.borrowed.data() &&
                !(cluster->flags & rxCLFLAGS_EXTERNAL) &&
                cluster->currentData == cluster->data &&
                std::memcmp(cluster->data, state.borrowed.data(), 16) == 0,
                "read-only external cluster becomes owned copy on write");
        static_cast<unsigned*>(cluster->data)[0] = 99;
        Require(state.borrowed[0] == 10, "copy on write preserves caller buffer");
        Require(RxClusterResizeData(cluster, 2) == cluster && cluster->numUsed == 2 &&
                cluster->numAlloced == 2 && cluster->currentData == cluster->data &&
                static_cast<unsigned*>(cluster->data)[0] == 99, "resize clamps used count and resets cursor");
        // SetExternalData frees the previous owned allocation.
        RxClusterSetExternalData(cluster, state.borrowed.data(), 4, 4);
        cluster->flags |= rxCLFLAGS_EXTERNALMODIFIABLE;
        Require(RxClusterLockWrite(packet, 0, node) == cluster && cluster->data == state.borrowed.data(),
                "modifiable external data stays borrowed");
        // Leave the packet in use: Execute must destroy it even when Body fails.
    }
    return state.result;
}
struct Definition {
    RxClusterDefinition cluster{const_cast<char*>("pal4-packet-words"), 4, 0x1357, nullptr};
    RxClusterRef reference{&cluster, rxCLFORCEPRESENT, 0};
    RxClusterValidityReq requirement = rxCLREQ_DONTWANT;
    RxNodeDefinition value{};
    explicit Definition(bool packet = false) {
        value.name = const_cast<char*>("pal4-pipeline-contract");
        value.nodeMethods = {Body, NodeInit, NodeTerm, InstanceInit, InstanceTerm, Config, nullptr};
        if (packet) value.io = {1, &reference, &requirement, 0, nullptr};
        value.editable = rxNODEDEFCONST;
    }
};
struct Pipeline {
    RxPipeline* value{};
    explicit Pipeline(RxNodeDefinition& definition) {
        value = RxPipelineCreate(); Require(value != nullptr, "pipeline allocation");
        auto* locked = RxPipelineLock(value);
        Require(locked && RxLockedPipeAddFragment(locked, nullptr, &definition,
                static_cast<RxNodeDefinition*>(nullptr)), "pipeline fragment");
        Require(RxLockedPipeUnlock(locked) == value, "pipeline unlock");
    }
    void Destroy() { RxPipelineDestroy(value); value = nullptr; }
    ~Pipeline() { RxPipelineDestroy(value); }
};
void Lifetime(bool& fail_next) {
    fail_next = true;
    Require(!RxPipelineCreate() && !fail_next, "pipeline create allocation failure");
    auto* empty = RxPipelineCreate(); const RxPipeline zero{};
    Require(empty && std::memcmp(empty, &zero, sizeof zero) == 0, "create initializes all52 bytes");
    RxPipelineDestroy(empty); RxPipelineDestroy(nullptr);
    Definition def; events.clear();
    Pipeline first(def.value), second(def.value);
    Require(events == std::vector<int>({1, 2, 3, 2, 3}) && def.value.InputPipesCnt == 2,
            "shared definition init once, instance init/config twice");
    first.Destroy();
    Require(events.back() == -2 && def.value.InputPipesCnt == 1, "first destroy retains definition");
    Require(RxPipelineLock(second.value) != nullptr, "relock second pipeline");
    Require(events == std::vector<int>({1, 2, 3, 2, 3, -2, -2, -1}) &&
            def.value.InputPipesCnt == 0, "last unlocked instance terminates definition on lock");
    const auto count = events.size(); second.Destroy();
    Require(events.size() == count, "locked destruction does not repeat termination callbacks");
}
void Clusters() {
    RxCluster cluster{};
    Require(RxClusterInitializeData(&cluster, 8, 4) == &cluster && cluster.data &&
            cluster.currentData == cluster.data && cluster.numAlloced == 8 && cluster.numUsed == 0 &&
            cluster.stride == 4 && (cluster.flags & rxCLFLAGS_CLUSTERVALID), "owned cluster initialize");
    auto* data = static_cast<unsigned*>(cluster.data);
    for (unsigned i = 0; i < 8; ++i) data[i] = 100 + i;
    cluster.numUsed = 8;
    Require(RxClusterResizeData(&cluster, 4) == &cluster && cluster.numUsed == 4 &&
            cluster.numAlloced == 4 && static_cast<unsigned*>(cluster.data)[3] == 103,
            "owned resize data and used count");
    std::array<unsigned, 3> borrowed{5, 6, 7};
    Require(RxClusterSetExternalData(&cluster, borrowed.data(), 0x10004, 3) == &cluster &&
            cluster.stride == 4 && cluster.numUsed == 3 && cluster.numAlloced == 3 &&
            (cluster.flags & 3) == 3, "external stride truncation and ownership flags");
    // This SDK cleanup helper returns null despite its pointer declaration.
    // It has no accepted original-EXE match; use it only for fixture cleanup.
    RxClusterDestroyData(&cluster);
    Require(!cluster.data && !cluster.flags && !cluster.numUsed && !cluster.numAlloced && borrowed[0] == 5,
            "external cleanup clears ownership and preserves caller storage");
}
void Execute(bool packet) {
    Definition def(packet); Pipeline pipe(def.value); Execution state; state.packet = packet;
    auto* heap = RxHeapGetGlobalHeap(); Require(heap != nullptr, "global pipeline heap");
    // An allocation marks the heap dirty. No packet pointer uses this allocation.
    Require(RxHeapAlloc(heap, 32) != nullptr && heap->dirty, "heap dirty fixture");
    Require(RxPipelineExecute(pipe.value, &state, FALSE) == pipe.value && state.dirty,
            "heapReset false preserves dirty heap before callback");
    if (packet) Require(pipe.value->embeddedPacketState == rxPKST_UNUSED, "success packet cleanup");
    state.result = false;
    Require(!RxPipelineExecute(pipe.value, &state, TRUE) && !state.dirty && state.calls == 2,
            "heap reset before callback and failure propagation");
    if (packet) Require(pipe.value->embeddedPacketState == rxPKST_UNUSED && state.borrowed[0] == 10,
                        "failure packet cleanup preserves borrowed input");
    state.result = true;
    Require(RxPipelineExecute(pipe.value, &state, TRUE) == pipe.value && state.calls == 3,
            "execution state reusable after failure");
}
}
void RunPipelineCase(std::string_view name, bool& fail_next) {
    if (name == "pipeline_lifetime") Lifetime(fail_next);
    else if (name == "pipeline_clusters") Clusters();
    else if (name == "pipeline_execute") Execute(false);
    else if (name == "pipeline_packet") Execute(true);
    else throw std::runtime_error("unknown pipeline case");
}
}
