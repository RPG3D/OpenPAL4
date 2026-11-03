#include "support/native_test_display.h"
#include <windows.h>
#include <rwcore.h>
#include <d3d9.h>
#include "support/renderware_memory_fixture.h"
#include "support/d3d9_device_forwarder.h"
#include <array>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <string_view>
#include <vector>

extern "C" {
extern IDirect3DDevice9 *_RwD3DDevice;
RwBool _rwIm3DCreatePlatformTransformPipeline(RxPipeline **);
void _rwIm3DDestroyPlatformTransformPipeline(RxPipeline **);
RwBool _rwIm3DCreatePlatformRenderPipelines(rwIm3DRenderPipelines *);
void _rwIm3DDestroyPlatformRenderPipelines(rwIm3DRenderPipelines *);
RwBool _rwD3D9Im3DRenderOpen();
void _rwD3D9Im3DRenderClose();
RxNodeDefinition *RxNodeDefinitionGetD3D9ImmInstance();
}
namespace {
using namespace pal4::test::memory_fixture;
void Check(bool ok, const char *why) {
    if (!ok)
        throw std::runtime_error(why);
}
static_assert(sizeof(RxCluster) == 28 && offsetof(RxPacket, clusters) == 20);
static_assert(sizeof(rwIm3DRenderPipelines) == 24 && sizeof(RxRenderStateVector) == 44);
static_assert(sizeof(RxPipeline) == 52 && sizeof(RxPipelineNode) == 40);
static_assert(sizeof(RxPipelineNodeTopSortData) == 12 && sizeof(RxPipelineRequiresCluster) == 12);
static_assert(offsetof(RwGlobals, memoryAlloc) == 280 && offsetof(RwGlobals, engineStatus) == 292);
struct Session {
    HWND window{};
    Session() {
        const RwMemoryFunctions memory{Allocate, Free, Reallocate, Callocate};
        Check(RwEngineInit(&memory, 0, 4 * 1024 * 1024), "engine init");
        window = pal4::test::CreateTestWindow("STATIC", "PAL4 packets", WS_OVERLAPPEDWINDOW, 64, 64, nullptr,
                               nullptr, GetModuleHandleA(nullptr), nullptr);
        Check(window, "window");
        RwEngineOpenParams params{window};
        Check(RwEngineOpen(&params) && RwEngineStart(), "engine start");
    }
    ~Session() {
        failNth = 0;
        RwEngineStop();
        RwEngineClose();
        RwEngineTerm();
        DestroyWindow(window);
    }
};

void Platform() {
    auto *original = RwIm3DGetTransformPipeline();
    RxPipeline *created{};
    Check(_rwIm3DCreatePlatformTransformPipeline(&created) && created && created != original &&
              created->pluginId == 1,
          "transform pipeline create/publish");
    Check(RwIm3DGetTransformPipeline() == created && created->numNodes == 1,
          "one-node active transform");
    _rwIm3DDestroyPlatformTransformPipeline(&created);
    Check(!created && RwIm3DGetTransformPipeline() == original,
          "destroy restores platform transform default");
    _rwIm3DDestroyPlatformTransformPipeline(&created);
    const auto old = RwIm3DGetRenderPipeline(rwPRIMTYPETRILIST);
    _rwD3D9Im3DRenderClose();
    rwIm3DRenderPipelines render{};
    render.pointList = reinterpret_cast<RxPipeline *>(0x1234);
    Check(_rwIm3DCreatePlatformRenderPipelines(&render), "render pipeline create");
    Check(render.triList && render.triList == render.triFan && render.triList == render.triStrip &&
              render.triList == render.lineList && render.triList == render.polyLine &&
              render.pointList == reinterpret_cast<RxPipeline *>(0x1234),
          "five shared slots and untouched point slot");
    Check(RwIm3DGetRenderPipeline(rwPRIMTYPELINELIST) == render.triList, "render aliases installed");
    _rwIm3DDestroyPlatformRenderPipelines(&render);
    Check(!render.triList && !render.triFan && !render.triStrip && !render.lineList &&
              !render.polyLine && render.pointList == reinterpret_cast<RxPipeline *>(0x1234),
          "destroy clears only five slots");
    Check(RwIm3DGetRenderPipeline(rwPRIMTYPETRILIST) == old, "render default restored");
    Check(_rwD3D9Im3DRenderOpen(), "restore render buffers");
}
RxPipeline *failedPipeline{};
RwBool RejectConfig(RxPipelineNode *, RxPipeline *pipeline) {
    failedPipeline = pipeline;
    return FALSE;
}
class RejectIndexBuffer final : public pal4::test::D3D9DeviceForwarder {
  public:
    RejectIndexBuffer() : D3D9DeviceForwarder(_RwD3DDevice) { _RwD3DDevice = this; }
    ~RejectIndexBuffer() { _RwD3DDevice = original_; }
    HRESULT STDMETHODCALLTYPE CreateIndexBuffer(UINT, DWORD, D3DFORMAT, D3DPOOL,
                                                IDirect3DIndexBuffer9 **out, HANDLE *) override {
        *out = nullptr;
        return D3DERR_OUTOFVIDEOMEMORY;
    }
};
void *RetainShrink(void *p, size_t n, RwUInt32 hint) {
    if (p && n <= live.at(p))
        return p; // Legal allocator behavior: retain physical capacity on shrink.
    return Reallocate(p, n, hint);
}
void PlatformFailure() {
    auto *original = RwIm3DGetTransformPipeline();
    auto *sentinel = reinterpret_cast<RxPipeline *>(0x1234);
    auto *out = sentinel;
    failNth = 1;
    Check(!_rwIm3DCreatePlatformTransformPipeline(&out) && !failNth && out == sentinel &&
              RwIm3DGetTransformPipeline() == original,
          "create failure preserves output/active pipeline");
    const auto before = live;
    auto *definition = RxNodeDefinitionGetD3D9ImmInstance();
    auto saved = definition->nodeMethods.pipelineNodeConfig;
    auto originalRealloc = RWSRCGLOBAL(memoryFuncs).rwrealloc;
    RWSRCGLOBAL(memoryFuncs).rwrealloc = RetainShrink;
    definition->nodeMethods.pipelineNodeConfig = RejectConfig;
    const auto result = _rwIm3DCreatePlatformTransformPipeline(&out);
    definition->nodeMethods.pipelineNodeConfig = saved;
    RWSRCGLOBAL(memoryFuncs).rwrealloc = originalRealloc;
    Check(!result && out == sentinel && live.size() > before.size(),
          "unlock failure loses original pipeline handle");
    auto *leaked = failedPipeline; // Exact callback argument; pipeline objects use the SDK freelist.
    Check(leaked && leaked->locked, "failed unlock leaves diagnostic-recoverable locked pipeline");
    RxPipelineDestroy(leaked);
    Check(live == before, "diagnostic cleanup of lost pipeline, no production fix");
    const auto *old = RwIm3DGetRenderPipeline(rwPRIMTYPETRILIST);
    _rwD3D9Im3DRenderClose();
    rwIm3DRenderPipelines render{};
    {
        RejectIndexBuffer reject;
        Check(!_rwIm3DCreatePlatformRenderPipelines(&render), "device-open failure propagated");
    }
    Check(render.triList && RwIm3DGetRenderPipeline(rwPRIMTYPETRILIST) == render.triList,
          "device failure occurs after output slots and active pipeline publication");
    _rwIm3DDestroyPlatformRenderPipelines(&render);
    Check(RwIm3DGetRenderPipeline(rwPRIMTYPETRILIST) == old && _rwD3D9Im3DRenderOpen(),
          "explicit cleanup restores render defaults and buffers");
}
void *guardedRegion{};
std::uintptr_t guardBegin{}, guardEnd{}, faultTarget{}, faultInstruction{};
ULONG_PTR faultOperation{};
void *GuardShrink(void *p, size_t n, RwUInt32 hint) {
    if (p && n < live.at(p)) {
        SYSTEM_INFO info{};
        GetSystemInfo(&info);
        const auto page = info.dwPageSize;
        const auto usable = (n + page - 1) & ~(size_t(page) - 1);
        auto *region = static_cast<unsigned char *>(
            VirtualAlloc(nullptr, usable + page, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE));
        if (!region)
            return nullptr;
        DWORD old{};
        if (!VirtualProtect(region + usable, page, PAGE_NOACCESS, &old)) {
            VirtualFree(region, 0, MEM_RELEASE);
            return nullptr;
        }
        auto *result = region + usable - n;
        std::memcpy(result, p, n);
        Free(p);
        guardedRegion = region;
        guardBegin = reinterpret_cast<std::uintptr_t>(region + usable);
        guardEnd = guardBegin + page;
        return result;
    }
    return Reallocate(p, n, hint);
}
int CapturePlatformFault(EXCEPTION_POINTERS *exception) {
    const auto *record = exception->ExceptionRecord;
    faultInstruction = reinterpret_cast<std::uintptr_t>(record->ExceptionAddress);
    if (record->ExceptionCode == EXCEPTION_ACCESS_VIOLATION && record->NumberParameters >= 2) {
        faultOperation = record->ExceptionInformation[0];
        faultTarget = record->ExceptionInformation[1];
    }
    return EXCEPTION_EXECUTE_HANDLER;
}
DWORD GuardPlatformCreate() {
    RxPipeline *out{};
    __try {
        _rwIm3DCreatePlatformTransformPipeline(&out);
        return 0;
    } __except (CapturePlatformFault(GetExceptionInformation())) {
        return GetExceptionCode();
    }
}
[[noreturn]] void PlatformMoveFailure() {
    auto *definition = RxNodeDefinitionGetD3D9ImmInstance();
    auto saved = definition->nodeMethods.pipelineNodeConfig;
    auto originalRealloc = RWSRCGLOBAL(memoryFuncs).rwrealloc;
    failedPipeline = nullptr;
    definition->nodeMethods.pipelineNodeConfig = RejectConfig;
    RWSRCGLOBAL(memoryFuncs).rwrealloc = GuardShrink;
    const auto code = GuardPlatformCreate();
    definition->nodeMethods.pipelineNodeConfig = saved;
    RWSRCGLOBAL(memoryFuncs).rwrealloc = originalRealloc;
    const bool observed = code == EXCEPTION_ACCESS_VIOLATION && failedPipeline && guardedRegion &&
                          faultOperation == 1 && faultTarget >= guardBegin && faultTarget < guardEnd;
    std::fprintf(stdout, "rollback write: exe_rva=%zx guard_offset=%zx operation=%zu\n",
                 faultInstruction - reinterpret_cast<std::uintptr_t>(GetModuleHandleA(nullptr)),
                 faultTarget - guardBegin, static_cast<size_t>(faultOperation));
    std::fprintf(stdout, "guarded moving shrink: code=%08lX config_called=%d moved=%d\n", code,
                 failedPipeline != nullptr, guardedRegion != nullptr);
    std::fflush(stdout);
    // An expected SDK out-of-bounds rollback leaves partially rebuilt metadata.
    // This isolated CTest process exits without attempting unsafe engine teardown.
    ExitProcess(observed ? 0 : 1);
}
void StateVector() {
    auto *defaults = const_cast<RxRenderStateVector *>(RxRenderStateVectorGetDefaultRenderStateVector());
    Check(defaults, "default vector");
    const auto before = *defaults;
    RxRenderStateVector output{};
    output.Flags = 0xdeadbeef;
    Check(RxRenderStateVectorSetDefaultRenderStateVector(&output) == &output &&
              !std::memcmp(&output, &before, sizeof output) &&
              !std::memcmp(defaults, &before, sizeof before),
          "misleading setter name actually loads defaults into argument");
    Check(!RxRenderStateVectorSetDefaultRenderStateVector(nullptr), "null vector fails");
    RwError error{};
    RwErrorGet(&error);
    Check(static_cast<unsigned>(error.errorCode) == 0x80000016u, "null vector error");
    Check(RwEngineStop(), "stop for nonstarted policy");
    output.Flags = 0xdeadbeef;
    Check(!RxRenderStateVectorSetDefaultRenderStateVector(&output) && output.Flags == 0xdeadbeef,
          "nonstarted arbitrary destination fails unchanged");
    RwErrorGet(&error);
    Check(static_cast<unsigned>(error.errorCode) == 0x80000018u, "nonstarted target error");
    Check(RxRenderStateVectorSetDefaultRenderStateVector(defaults) == defaults && defaults->Flags == 7 &&
              defaults->ShadeMode == rwSHADEMODEGOURAUD && defaults->SrcBlend == rwBLENDSRCALPHA &&
              defaults->DestBlend == rwBLENDINVSRCALPHA && !defaults->TextureRaster &&
              defaults->AddressModeU == rwTEXTUREADDRESSWRAP &&
              defaults->AddressModeV == rwTEXTUREADDRESSWRAP && defaults->FilterMode == rwFILTERLINEAR &&
              defaults->FogType == rwFOGTYPENAFOGTYPE,
          "only global default target may initialize before start");
    Check(defaults->BorderColor.red == 255 && defaults->BorderColor.alpha == 255 &&
              defaults->FogColor.blue == 255,
          "white default colors");
    Check(RwEngineStart(), "restart after state probe");
}
struct Clusters {
    RxClusterDefinition shared{const_cast<char *>("shared"), 4, 0x10, nullptr};
    RxClusterDefinition discarded{const_cast<char *>("discarded"), 4, 0x20, nullptr};
    RxClusterDefinition borrowed{const_cast<char *>("borrowed"), 4, 0x30, nullptr};
    RxClusterDefinition sameName{const_cast<char *>("discarded"), 4, 0x40, nullptr};
};
struct Context {
    Clusters *definitions{};
    RxPipeline *destination{};
    bool cross{}, drop{}, externalDiscard{}, sinkFailure{}, destroyOnly{};
    unsigned sinkCalls{};
    std::array<unsigned, 4> borrowed{31, 32, 33, 34}, discarded{21, 22, 23, 24};
    std::array<void *, 3> data{};
    std::array<RxCluster *, 3> sourceSlots{};
    RxPacket *sourcePacket{};
    RxPacket *destinationPacket{};
};
bool IsFree(void *p) {
    auto *heap = RxHeapGetGlobalHeap();
    const auto a = reinterpret_cast<std::uintptr_t>(p);
    for (unsigned i = 0; i < heap->entriesUsed; ++i) {
        const auto &e = heap->freeBlocks[i];
        const auto b = reinterpret_cast<std::uintptr_t>(e.ptr) + 32;
        if (b <= a && a < b + e.size)
            return true;
    }
    return false;
}
RwBool Sink(RxPipelineNode *node, const RxPipelineNodeParam *params) {
    auto &c = *static_cast<Context *>(params->dataParam);
    ++c.sinkCalls;
    Check(_rxExecCtxGlobal.pipeline->embeddedPacketState == rxPKST_PENDING,
          "dispatch marks destination pending before fetch");
    auto *packet = RxPacketFetch(node);
    c.destinationPacket = packet;
    Check(packet && _rxExecCtxGlobal.pipeline->embeddedPacketState == rxPKST_INUSE &&
              !RxPacketFetch(node),
          "fetch pending once");
    for (unsigned i = 0; i < 3; ++i) {
        const unsigned sourceIndex = i == 0 ? 2 : i == 1 ? 0 : 1;
        auto *value = RxClusterLockRead(packet, i);
        if (i == 2 && (c.drop || c.cross)) {
            Check(!value || !value->data, "discarded/identity-mismatched cluster absent");
            continue;
        }
        Check(value && value->data == c.data[sourceIndex] && value->numUsed == 4 &&
                  value->attributes == 0x100 + sourceIndex,
              "cluster data and attributes transferred without copying");
        const auto slot = packet->inputToClusterSlot[i];
        Check(value->clusterRef == node->slotClusterRefs[slot] && value->currentData == value->data,
              "destination reference and read cursor mapping");
        Check(static_cast<unsigned *>(value->data)[0] == (sourceIndex == 2   ? 31u
                                                          : sourceIndex == 1 ? 21u
                                                                             : 11u),
              "transferred values");
    }
    if (c.drop || c.cross)
        Check(c.externalDiscard || IsFree(c.data[1]), "discarded owned cluster released before sink");
    if (c.cross) {
        Check(c.sourcePacket != packet && c.sourcePacket->pipeline->embeddedPacketState == rxPKST_UNUSED,
              "cross-pipeline move drains source packet");
        for (auto *slot : c.sourceSlots)
            Check(!slot->data && !slot->clusterRef, "source cluster ownership cleared");
    }
    return !c.sinkFailure;
}
RwBool Source(RxPipelineNode *node, const RxPipelineNodeParam *params) {
    auto &c = *static_cast<Context *>(params->dataParam);
    auto *packet = RxPacketCreate(node);
    Check(packet, "source packet");
    c.sourcePacket = packet;
    for (unsigned i = 0; i < 3; ++i) {
        auto *slot = RxClusterLockWrite(packet, i, node);
        Check(slot, "source cluster slot");
        c.sourceSlots[i] = slot;
        if (i == 2 || i == 1 && c.externalDiscard)
            Check(RxClusterSetExternalData(slot, i == 2 ? c.borrowed.data() : c.discarded.data(), 4,
                                           4) == slot,
                  "borrowed source data");
        else {
            Check(RxClusterInitializeData(slot, 4, 4) == slot, "owned source data");
            for (unsigned k = 0; k < 4; ++k)
                static_cast<unsigned *>(slot->data)[k] = i == 0 ? 11 + k : 21 + k;
            slot->numUsed = 4;
        }
        slot->attributes = 0x100 + i;
        slot->currentData = static_cast<unsigned *>(slot->data) + 1;
        c.data[i] = slot->data;
    }
    if (c.destroyOnly) {
        packet->flags = 0x55;
        _rxPacketDestroy(packet);
        Check(packet->flags == 0 && packet->pipeline->embeddedPacketState == rxPKST_UNUSED,
              "destroy packet flags/state");
        for (unsigned i = 0; i < 3; ++i) {
            auto *slot = c.sourceSlots[i];
            Check(!slot->flags && !slot->stride && !slot->data && !slot->numAlloced && !slot->numUsed &&
                      !slot->clusterRef && slot->currentData == static_cast<unsigned *>(c.data[i]) + 1 &&
                      slot->attributes == 0x100 + i,
                  "destroy resets ownership but retains cursor and attributes");
        }
        Check(IsFree(c.data[0]) && IsFree(c.data[1]) && c.borrowed[0] == 31,
              "destroy frees owned data only");
        return TRUE;
    }
    if (!c.cross)
        Check(bool(node->slotsContinue[0] & 1) == !c.drop, "all-clusters continuation shortcut");
    // SDK Release dispatch macros ignore the packet argument, routing the active packet.
    if (c.cross)
        RxPacketDispatchToPipeline(nullptr, c.destination, node);
    else
        RxPacketDispatch(nullptr, 0, node);
    if (c.sinkFailure) {
        if (c.cross)
            RxPacketDispatchToPipeline(nullptr, c.destination, node);
        else
            RxPacketDispatch(nullptr, 0, node);
        Check(c.sinkCalls == 1, "sticky failure suppresses second dispatch");
    }
    return TRUE; // Must not overwrite the downstream failure in execution context.
}
struct Definition {
    std::array<RxClusterRef, 3> refs{};
    std::array<RxClusterValidityReq, 3> needs{};
    std::array<RxClusterValid, 3> valid{};
    RxOutputSpec output{};
    RxNodeDefinition node{};
    Definition(Clusters &types, bool source, bool drop = false, bool cross = false) {
        const std::array<RxClusterDefinition *, 3> keys =
            source
                ? std::array<RxClusterDefinition *, 3>{&types.shared, &types.discarded, &types.borrowed}
                : std::array<RxClusterDefinition *, 3>{&types.borrowed, &types.shared,
                                                       cross ? &types.sameName : &types.discarded};
        for (unsigned i = 0; i < 3; ++i) {
            refs[i] = {keys[i], rxCLFORCEPRESENT, 0};
            needs[i] = source ? rxCLREQ_DONTWANT
                              : (i == 2 && (drop || cross) ? rxCLREQ_OPTIONAL : rxCLREQ_REQUIRED);
            valid[i] = drop && i == 1 ? rxCLVALID_INVALID : rxCLVALID_VALID;
        }
        node.name = const_cast<char *>(source ? "packet-source" : "packet-sink");
        node.nodeMethods.nodeBody = source ? Source : Sink;
        node.editable = rxNODEDEFCONST;
        node.io.numClustersOfInterest = 3;
        node.io.clustersOfInterest = refs.data();
        node.io.inputRequirements = needs.data();
        if (source) {
            output = {const_cast<char *>("next"), valid.data(), rxCLVALID_NOCHANGE};
            node.io.numOutputs = 1;
            node.io.outputs = &output;
        }
    }
};
struct Pipeline {
    RxPipeline *value{};
    explicit Pipeline(Definition &first, Definition *second = nullptr) {
        value = RxPipelineCreate();
        Check(value && RxPipelineLock(value), "test pipeline create/lock");
        Check(RxLockedPipeAddFragment(value, nullptr, &first.node,
                                      static_cast<RxNodeDefinition *>(nullptr)),
              "first node");
        if (second) {
            Check(RxLockedPipeAddFragment(value, nullptr, &second->node,
                                          static_cast<RxNodeDefinition *>(nullptr)),
                  "second node");
            auto *a = RxPipelineFindNodeByName(value, first.node.name, nullptr, nullptr);
            auto *b = RxPipelineFindNodeByName(value, second->node.name, nullptr, nullptr);
            Check(RxLockedPipeAddPath(value, RxPipelineNodeFindOutputByIndex(a, 0),
                                      RxPipelineNodeFindInput(b)),
                  "connect packet nodes");
        }
        Check(RxLockedPipeUnlock(value) == value, "unlock packet pipeline");
    }
    ~Pipeline() { RxPipelineDestroy(value); }
};
void Transfer(bool cross, bool drop, bool external, bool failure, bool destroy = false) {
    Clusters clusters;
    Definition source(clusters, true, drop, cross), sink(clusters, false, drop, cross);
    Context c{};
    c.definitions = &clusters;
    c.cross = cross;
    c.drop = drop;
    c.externalDiscard = external;
    c.sinkFailure = failure;
    c.destroyOnly = destroy;
    Pipeline target(sink), origin(source, cross || destroy ? nullptr : &sink);
    c.destination = target.value;
    auto *result = RxPipelineExecute(origin.value, &c, TRUE);
    Check(bool(result) == !failure && c.sinkCalls == (destroy ? 0u : 1u),
          "pipeline result retains downstream error");
    Check(origin.value->embeddedPacketState == rxPKST_UNUSED &&
              target.value->embeddedPacketState <= rxPKST_UNUSED,
          "dispatch/execute cleans packets");
    Check(IsFree(c.data[0]) && (external || IsFree(c.data[1])) && c.borrowed[0] == 31 &&
              c.discarded[0] == 21,
          "final ownership and external buffers");
}
DWORD GuardDestroy(RxPacket *packet) {
    __try {
        _rxPacketDestroy(packet);
        return 0;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return GetExceptionCode();
    }
}
void ZeroClusters() {
    SYSTEM_INFO info{};
    GetSystemInfo(&info);
    auto page = info.dwPageSize;
    auto *area = static_cast<unsigned char *>(
        VirtualAlloc(nullptr, 2 * page, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE));
    Check(area, "packet guard allocation");
    DWORD old{};
    Check(VirtualProtect(area + page, page, PAGE_NOACCESS, &old), "packet guard page");
    auto *packet = reinterpret_cast<RxPacket *>(area + page - 20);
    std::memset(packet, 0, 20);
    RxPipeline pipeline{};
    pipeline.embeddedPacketState = rxPKST_INUSE;
    packet->pipeline = &pipeline;
    const auto code = GuardDestroy(packet);
    VirtualFree(area, 0, MEM_RELEASE);
    Check(code == EXCEPTION_ACCESS_VIOLATION && pipeline.embeddedPacketState == rxPKST_UNUSED,
          "zero-cluster do-while faults after marking pipeline unused");
}
} // namespace
int main(int argc, char **argv) {
    SetUnhandledExceptionFilter([](EXCEPTION_POINTERS *e) -> LONG {
        std::fprintf(stderr, "native fault %08lX exe offset %zx\n", e->ExceptionRecord->ExceptionCode,
                     reinterpret_cast<size_t>(e->ExceptionRecord->ExceptionAddress) -
                         reinterpret_cast<size_t>(GetModuleHandleA(nullptr)));
        return EXCEPTION_EXECUTE_HANDLER;
    });
    try {
        Check(argc == 2, "one case");
        {
            Session session;
            std::string_view name = argv[1];
            if (name == "platform")
                Platform();
            else if (name == "platform_failure")
                PlatformFailure();
            else if (name == "platform_move_failure")
                PlatformMoveFailure();
            else if (name == "state_vector")
                StateVector();
            else if (name == "nodes_keep")
                Transfer(false, false, false, false);
            else if (name == "nodes_drop")
                Transfer(false, true, false, false);
            else if (name == "cross_owned")
                Transfer(true, true, false, false);
            else if (name == "cross_external")
                Transfer(true, true, true, false);
            else if (name == "dispatch_failure") {
                Transfer(false, true, false, true);
                Transfer(true, true, false, true);
            } else if (name == "destroy")
                Transfer(false, false, false, false, true);
            else if (name == "zero_clusters")
                ZeroClusters();
            else
                throw std::runtime_error("unknown packet case");
        }
        Check(live.empty() && !invalidFree, "all test engine allocations released");
        std::puts("PASS");
        return 0;
    } catch (const std::exception &e) {
        std::fprintf(stderr, "FAIL: %s\n", e.what());
        return 1;
    }
}
