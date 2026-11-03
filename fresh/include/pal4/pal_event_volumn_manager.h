#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <string_view>

#include "pal4/scene_evf_record.h"

namespace pal4 {

struct PalEventVolumnRegistrationState final {
    std::uint8_t guard = 0u;
    std::uint32_t instance = 0u;
};

struct PalEventVolumnState final {
    std::array<std::uint32_t, 13u> words{};
};
static_assert(sizeof(PalEventVolumnState) == 52u);

struct PalEventVolumnSharedTree final {
    std::uint32_t nil = 0u;
    std::uint32_t references = 0u;
};

class IPalEventVolumnMemory {
public:
    virtual ~IPalEventVolumnMemory() = default;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    virtual void Free(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t byte_offset,
                           std::uint32_t value) = 0;
    [[nodiscard]] virtual PalEventVolumnState& ObjectAt(
        std::uint32_t address) = 0;
};

class IPalEventVolumnCleanupRuntime {
public:
    virtual ~IPalEventVolumnCleanupRuntime() = default;
    [[nodiscard]] virtual PalEventVolumnState& ObjectAt(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    virtual void DestroyRecord(std::uint32_t address) = 0;
    virtual void ClearTree(PalEventVolumnState& state,
                           PalEventVolumnSharedTree& shared) = 0;
    [[nodiscard]] virtual int Free(std::uint32_t address) = 0;
};

class IPalEventVolumnSelectionRuntime {
public:
    virtual ~IPalEventVolumnSelectionRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadRecordPointer(
        std::uint32_t slot) = 0;
    [[nodiscard]] virtual SceneEvfRecordState& RecordAt(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual ISceneEvfColorRuntime& Colors() = 0;
    [[nodiscard]] virtual ISceneEvfIntersectionRuntime& Intersections() = 0;
};

class IPalEventVolumnNameRuntime : public ISceneEvfRecordRuntime,
                                   public ISceneEvfRecordStringRuntime,
                                   public ISceneEvfRecordCleanupRuntime {
public:
    ~IPalEventVolumnNameRuntime() override = default;
    [[nodiscard]] virtual std::uint32_t ReadRecordPointer(
        std::uint32_t slot) = 0;
    [[nodiscard]] virtual bool CompareRecordName(
        std::uint32_t record, std::string_view name) = 0;
};

class IPalEventVolumnRegistryRuntime {
public:
    virtual ~IPalEventVolumnRegistryRuntime() = default;
    [[nodiscard]] virtual std::uint32_t SingletonManager() = 0;
    [[nodiscard]] virtual std::uint32_t Register(
        std::uint32_t manager, std::string_view name,
        std::uint32_t singleton_id, std::uint32_t factory_address,
        std::uint32_t destructor_address) = 0;
};

class IPalEventVolumnSceneLoadRuntime {
public:
    virtual ~IPalEventVolumnSceneLoadRuntime() = default;
    [[nodiscard]] virtual std::uint32_t OpenStream(
        std::string_view path, std::uint32_t mode,
        std::uint32_t flags) = 0;
    [[nodiscard]] virtual std::uint32_t ReadRecordCount(
        std::uint32_t stream) = 0;
    [[nodiscard]] virtual std::uint32_t AllocateRecordBytes(
        std::uint32_t bytes) = 0;
    [[nodiscard]] virtual SceneEvfRecordState& RecordAt(
        std::uint32_t address) = 0;
    virtual void InsertRecord(PalEventVolumnState& state,
                              std::uint32_t record) = 0;
    virtual void CloseStream(std::uint32_t stream,
                             std::uint32_t flags) = 0;
    [[nodiscard]] virtual int FinishTemporaryPath() = 0;
};

class IPalEventVolumnTreeRuntime {
public:
    virtual ~IPalEventVolumnTreeRuntime() = default;
    [[nodiscard]] virtual std::uint32_t SharedNil() = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    virtual void InsertAt(PalEventVolumnState& state,
                          std::uint32_t child, std::uint32_t parent,
                          std::uint32_t key, std::uint32_t value) = 0;
};

class IPalEventVolumnTreeSubtreeRuntime {
public:
    virtual ~IPalEventVolumnTreeSubtreeRuntime() = default;
    [[nodiscard]] virtual std::uint32_t SharedNil() = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    virtual void FreeNode(std::uint32_t node) = 0;
};

class IPalEventVolumnTreeCleanupRuntime
    : public IPalEventVolumnTreeSubtreeRuntime {
public:
    virtual ~IPalEventVolumnTreeCleanupRuntime() = default;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    virtual void EraseNode(std::uint32_t node) = 0;
    virtual void ObserveFirstElement(std::uint32_t head) = 0;
    [[nodiscard]] virtual int Free(std::uint32_t address) = 0;
};

class IPalEventVolumnTreeEraseRuntime
    : public IPalEventVolumnTreeSubtreeRuntime {
public:
    virtual ~IPalEventVolumnTreeEraseRuntime() = default;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
};

class IPalEventVolumnTreeNodeInsertRuntime
    : public IPalEventVolumnTreeEraseRuntime {
public:
    ~IPalEventVolumnTreeNodeInsertRuntime() override = default;
    [[nodiscard]] virtual std::uint32_t AllocateNode(
        std::uint32_t bytes) = 0;
};

struct PalEventVolumnLoadServices final {
    IPalEventVolumnSceneLoadRuntime& scene;
    IPalEventVolumnCleanupRuntime& cleanup;
    ISceneEvfRecordRuntime& record;
    ISceneEvfStreamRuntime& stream;
    ISceneEvfRecordTypeRuntime& type;
    ISceneEvfGeometryRuntime& geometry;
    ISceneEvfAtomicRuntime& atomic;
};

class PalEventVolumnManager final {
public:
    // IDA 0x64ea10: register singleton ID 1012 once.
    [[nodiscard]] static std::uint32_t RegisterSingleton(
        PalEventVolumnRegistrationState& state,
        IPalEventVolumnRegistryRuntime& runtime);
    // IDA 0x64ea50: allocate a 52-byte instance, then construct it.
    [[nodiscard]] static std::uint32_t Create(
        PalEventVolumnSharedTree& shared, IPalEventVolumnMemory& memory);
    // IDA 0x64ead0: initialize instance fields and shared tree sentinels.
    [[nodiscard]] static PalEventVolumnState& Initialize(
        PalEventVolumnState& state, PalEventVolumnSharedTree& shared,
        IPalEventVolumnMemory& memory);
    // IDA 0x64eab0: deleting wrapper for a non-null instance.
    [[nodiscard]] static int Destroy(
        std::uint32_t address, PalEventVolumnSharedTree& shared,
        IPalEventVolumnCleanupRuntime& runtime);
    // IDA 0x64eb90: clear owned records and tree, then release buffers.
    [[nodiscard]] static int Cleanup(
        PalEventVolumnState& state, PalEventVolumnSharedTree& shared,
        IPalEventVolumnCleanupRuntime& runtime);
    // IDA 0x64ecc0: destroy each owned scene EVF record in the array.
    [[nodiscard]] static std::uint32_t ClearRecords(
        PalEventVolumnState& state, IPalEventVolumnCleanupRuntime& runtime);
    // IDA 0x64ed20: select the nearest visible EVF volume under a pixel.
    [[nodiscard]] static std::uint32_t PickAtPixel(
        PalEventVolumnState& state, std::uint32_t camera,
        std::uint32_t pixel, bool include_inactive,
        IPalEventVolumnSelectionRuntime& runtime);
    // IDA 0x64edc0: find the first EVF record with a matching name.
    [[nodiscard]] static std::uint32_t FindByName(
        PalEventVolumnState& state, std::uint32_t manager_address,
        std::uint32_t temporary_address, const char* name,
        IPalEventVolumnNameRuntime& runtime);
    // IDA 0x64eeb0: load <prefix><folder>\<scene>\<scene>.evf.
    [[nodiscard]] static int LoadSceneEvfRecords(
        PalEventVolumnState& state, std::uint32_t manager_address,
        std::string_view prefix, std::string_view folder,
        std::string_view scene, PalEventVolumnLoadServices services);
    // IDA 0x64f140: insert a signed key/value pair into the manager tree.
    [[nodiscard]] static bool InsertTreeEntry(
        PalEventVolumnState& state, std::uint32_t key,
        std::uint32_t value, IPalEventVolumnTreeRuntime& runtime);
    // IDA 0x64f240: return the value stored at an exact signed tree key.
    [[nodiscard]] static std::uint32_t FindTreeValue(
        const PalEventVolumnState& state, std::uint32_t key,
        IPalEventVolumnTreeRuntime& runtime);
    // IDA 0x64f800: first tree node whose signed key is at least key.
    [[nodiscard]] static std::uint32_t LowerBoundTreeNode(
        const PalEventVolumnState& state, std::uint32_t key,
        IPalEventVolumnTreeRuntime& runtime);
    // IDA 0x64f2a0: destroy the embedded tree and release shared nil.
    [[nodiscard]] static int CleanupTree(
        PalEventVolumnState& state, PalEventVolumnSharedTree& shared,
        IPalEventVolumnTreeCleanupRuntime& runtime);
    // IDA 0x64f380: erase one 24-byte red-black tree node.
    [[nodiscard]] static std::uint32_t EraseTreeNode(
        PalEventVolumnState& state, std::uint32_t node,
        IPalEventVolumnTreeEraseRuntime& runtime);
    // IDA 0x64f760: release a tree using right recursion and a left walk.
    [[nodiscard]] static std::uint32_t DestroyTreeSubtree(
        std::uint32_t root, IPalEventVolumnTreeSubtreeRuntime& runtime);
    // IDA 0x64f7a0: post-increment a tree iterator and return its old node.
    [[nodiscard]] static std::uint32_t AdvanceTreeIterator(
        std::uint32_t& cursor, IPalEventVolumnTreeSubtreeRuntime& runtime);
    // IDA 0x64f830: advance an in-order cursor in place.
    static void AdvanceTreeCursor(
        std::uint32_t& cursor, IPalEventVolumnTreeSubtreeRuntime& runtime);
    // IDA 0x64f880: allocate, link and rebalance a 24-byte tree node.
    [[nodiscard]] static std::uint32_t InsertTreeNode(
        PalEventVolumnState& state, std::uint32_t child,
        std::uint32_t parent, std::uint32_t key,
        std::uint32_t value, IPalEventVolumnTreeNodeInsertRuntime& runtime);
    // IDA 0x64faf0: previous node in signed-key tree order.
    [[nodiscard]] static std::uint32_t PredecessorTreeNode(
        std::uint32_t node, IPalEventVolumnTreeRuntime& runtime);
};

}  // namespace pal4
