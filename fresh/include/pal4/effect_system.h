#pragma once

#include "pal4/monster_manager_tree_insert.h"
#include "pal4/monster_manager_tree_cursor.h"

#include <cstdint>
#include <string_view>

namespace pal4 {

class IArenaSequenceRuntime;
class IEffectSystemPruneRuntime;
class IEffectSystemCleanupEffectsRuntime;
class IEffectSystemTimedRuntime;
class IEffectSystemActiveFrameRuntime;
class IEffectSystemSecondaryTimerRuntime;
class IEffectSystemObjectGroupRuntime;
class IEffectSystemTreeUpdateRuntime;
class IEffectSystemGroupDrawRuntime;
class IEffectSystemRenderStateRuntime;

class IEffectSystemCleanupRuntime {
public:
    virtual ~IEffectSystemCleanupRuntime() = default;
    [[nodiscard]] virtual std::uint32_t InitializeSystem() = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void DestroyXmlNode(std::uint32_t node) = 0;
    virtual void Free(std::uint32_t pointer) = 0;
    [[nodiscard]] virtual std::uint32_t RemoveInstance(
        std::uint32_t tree, std::uint32_t node) = 0;
};

class IEffectSystemLoadRuntime {
public:
    virtual ~IEffectSystemLoadRuntime() = default;
    [[nodiscard]] virtual std::uint32_t InitializeSystem() = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t FindCached(
        std::uint32_t system, std::string_view name) = 0;
    [[nodiscard]] virtual std::uint32_t CreateInstance() = 0;
    [[nodiscard]] virtual bool LoadInstance(
        std::uint32_t instance, std::uint32_t xml) = 0;
    virtual void DestroyInstance(std::uint32_t system,
                                 std::uint32_t instance) = 0;
    virtual void BeginScriptScope() = 0;
    virtual void EndScriptScope() = 0;
    [[nodiscard]] virtual std::string_view EffectDirectory() = 0;
    [[nodiscard]] virtual std::uint32_t OpenFile(std::string_view path) = 0;
    virtual void CloseFile(std::uint32_t file) = 0;
    [[nodiscard]] virtual std::uint32_t CreateXmlDocument(
        std::uint32_t bytes) = 0;
    [[nodiscard]] virtual std::uint32_t CreateFileAdapter(
        std::uint32_t file) = 0;
    [[nodiscard]] virtual bool ParseXml(
        std::uint32_t adapter, std::uint32_t xml) = 0;
    virtual void DestroyFileAdapter(std::uint32_t adapter) = 0;
    virtual void InsertCached(std::uint32_t system,
                              std::string_view name, std::uint32_t xml) = 0;
    virtual void DestroyXmlNode(std::uint32_t xml) = 0;
    virtual void FreeXml(std::uint32_t xml) = 0;
};

class IEffectSystemTreeRuntime {
public:
    virtual ~IEffectSystemTreeRuntime() = default;
    [[nodiscard]] virtual std::uint32_t SharedNil() const = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::int32_t CompareText(
        std::uint32_t node_text, std::uint32_t key) = 0;
};

class IEffectSystemInitRuntime {
public:
    virtual ~IEffectSystemInitRuntime() = default;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    virtual void InitializeFirstTree(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint8_t InitializeSecondTree(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t CreateListHead() = 0;
    virtual void InitializeInlineState(std::uint32_t address) = 0;
    virtual void InitializeFourthTree(std::uint32_t address) = 0;
    virtual void InitializeFifthTree(std::uint32_t address) = 0;
};

class IEffectSystemDisableRuntime {
public:
    virtual ~IEffectSystemDisableRuntime() = default;
    [[nodiscard]] virtual std::uint8_t ReadByte(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    virtual void WriteByte(std::uint32_t address,
                           std::uint8_t value) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    virtual void CleanupEffects(std::uint32_t system) = 0;
    virtual void InitializeGeometryData(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t FirstListElement(
        std::uint32_t list) = 0;
    virtual void RemoveInlineEffects(std::uint32_t list,
                                     std::uint32_t first,
                                     std::uint32_t head) = 0;
    virtual void RemoveListElements(std::uint32_t list,
                                    std::uint32_t first,
                                    std::uint32_t head) = 0;
    [[nodiscard]] virtual std::uint32_t InitializeSystem() = 0;
    virtual void RemoveEffect(std::uint32_t manager,
                              std::uint32_t system) = 0;
};

class IEffectSystemDestroyRuntime : public IEffectSystemDisableRuntime {
public:
    ~IEffectSystemDestroyRuntime() override = default;
    virtual void CleanupTrailingTree(std::uint32_t address) = 0;
    virtual void Free(std::uint32_t pointer) = 0;
    virtual void CleanupConfigList(std::uint32_t list) = 0;
    virtual void ReleaseRange(std::uint32_t first,
                              std::uint32_t last) = 0;
    virtual void CleanupSecondTree(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t CleanupFirstTree(
        std::uint32_t address) = 0;
};

class IEffectSystemCountRuntime {
public:
    virtual ~IEffectSystemCountRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
};

class IEffectSystemEnableRuntime : public IEffectSystemDisableRuntime {
public:
    ~IEffectSystemEnableRuntime() override = default;
    virtual void AddEffect(std::uint32_t manager,
                           std::uint32_t system) = 0;
};

class IEffectSystemFrameRuntime {
public:
    virtual ~IEffectSystemFrameRuntime() = default;
    [[nodiscard]] virtual std::uint8_t ReadByte(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual float ReadFloat(
        std::uint32_t address) = 0;
    virtual void UpdateProperties2(double seconds) = 0;
    virtual void UpdateTimers2(std::uint32_t system, double seconds) = 0;
    virtual void UpdateProperties12(std::uint32_t system,
                                    double seconds) = 0;
    virtual void UpdateProperties10(std::uint32_t system,
                                    double seconds) = 0;
    virtual void UpdateProperties11(std::uint32_t system,
                                    double seconds) = 0;
    [[nodiscard]] virtual int UpdateTimers(std::uint32_t system,
                                           double seconds) = 0;
};

class IEffectSystemDrawRuntime {
public:
    virtual ~IEffectSystemDrawRuntime() = default;
    [[nodiscard]] virtual std::uint8_t ReadByte(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual int SetRenderState(
        std::uint32_t state, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t GetDeviceState28() = 0;
    virtual void SetDeviceState28(std::uint32_t value) = 0;
    virtual void DrawProperties13(std::uint32_t system,
                                  std::uint32_t context) = 0;
    virtual void DrawProperties15(std::uint32_t system,
                                  std::uint32_t context) = 0;
    virtual void DrawProperties(std::uint32_t system,
                                std::uint32_t context) = 0;
    virtual void DrawProperties16(std::uint32_t system,
                                  std::uint32_t context) = 0;
    virtual void DrawProperties14(std::uint32_t system,
                                  std::uint32_t context) = 0;
};

class IEffectSystemClumpRuntime {
public:
    virtual ~IEffectSystemClumpRuntime() = default;
    [[nodiscard]] virtual std::uint8_t ReadByte(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint8_t ProcessAtomics(
        std::uint32_t clump, std::uint32_t geometry_data) = 0;
    [[nodiscard]] virtual std::uint8_t ProcessAtomicsWrapper(
        std::uint32_t clump) = 0;
};

class IEffectSystemObjectClassifyRuntime {
public:
    virtual ~IEffectSystemObjectClassifyRuntime() = default;
    [[nodiscard]] virtual std::uint8_t ReadByte(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::string_view ObjectTypeName(
        std::uint32_t object) = 0;
    [[nodiscard]] virtual bool UseFallbackQueue(
        std::uint32_t object) = 0;
    [[nodiscard]] virtual bool UseCameraQueue(
        std::uint32_t object) = 0;
};

class IEffectSystemObjectQueueRuntime
    : public IEffectSystemObjectClassifyRuntime {
public:
    ~IEffectSystemObjectQueueRuntime() override = default;
    virtual void AddToList(std::uint32_t list,
                           std::uint32_t head,
                           std::uint32_t object) = 0;
    virtual void AddInlineEffect(std::uint32_t list,
                                 std::uint32_t object,
                                 std::uint8_t enabled) = 0;
};

class IEffectSystemObjectRemoveRuntime
    : public IEffectSystemObjectClassifyRuntime {
public:
    ~IEffectSystemObjectRemoveRuntime() override = default;
    virtual void WriteByte(std::uint32_t address,
                           std::uint8_t value) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t FindInlineEffect(
        std::uint32_t tree, std::uint32_t object) = 0;
    virtual void FreeNode(std::uint32_t node) = 0;
};

struct EffectSystemHandle {
    // DNameNode_Construct (0x639F60), also used for creation failures.
    std::uint32_t id{0xffffffffu};
    std::uint32_t system{};
    bool operator==(const EffectSystemHandle&) const = default;
};

class IEffectSystemCreateRuntime {
public:
    virtual ~IEffectSystemCreateRuntime() = default;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteByte(std::uint32_t address,
                           std::uint8_t value) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    virtual void BeginScriptScope() = 0;
    virtual void EndScriptScope() = 0;
    [[nodiscard]] virtual bool UppercaseNames() = 0;
    [[nodiscard]] virtual std::uint32_t AcquireEffect(
        std::string_view name) = 0;
    virtual void ReturnEffect(std::uint32_t effect,
                              std::uint8_t mode) = 0;
    [[nodiscard]] virtual std::uint32_t FindXmlAttribute(
        std::uint32_t xml, std::string_view key) = 0;
    virtual void SetXmlString(std::uint32_t attribute,
                              std::string_view value) = 0;
    virtual void AddXmlString(std::uint32_t xml,
                              std::string_view key,
                              std::string_view value) = 0;
    virtual void SetXmlInt(std::uint32_t attribute,
                           std::uint32_t value) = 0;
    virtual void AddXmlInt(std::uint32_t xml,
                           std::string_view key,
                           std::uint32_t value) = 0;
    virtual void SetPosition(std::uint32_t effect,
                             std::uint32_t position,
                             std::uint8_t mode) = 0;
    [[nodiscard]] virtual bool StartPlayback(std::uint32_t effect) = 0;
    [[nodiscard]] virtual bool PausePlayback(std::uint32_t effect) = 0;
    virtual void InsertEffect(std::uint32_t tree, std::uint32_t id,
                              std::uint32_t effect,
                              std::uint32_t repeat_count) = 0;
    [[nodiscard]] virtual EffectSystemHandle CopyHandle(
        std::uint32_t id, std::uint32_t system) = 0;
    virtual void ResetTemporaryHandle(EffectSystemHandle& handle) = 0;
    virtual void NotifyCreated(std::uint32_t system,
                               const EffectSystemHandle& handle,
                               std::uint32_t position,
                               std::uint32_t repeat_count) = 0;
};

class IEffectSystemXmlRuntime {
public:
    virtual ~IEffectSystemXmlRuntime() = default;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t CreatePrimaryEffect(
        std::uint32_t xml, std::uint32_t context,
        std::uint32_t system) = 0;
    [[nodiscard]] virtual std::uint32_t CreateSecondaryEffect(
        std::uint32_t xml, std::uint32_t context,
        std::uint32_t system) = 0;
    virtual void InsertXmlEffect(std::uint32_t tree,
                                 std::uint32_t id,
                                 std::uint32_t effect) = 0;
};

class IEffectSystemXmlFileRuntime : public IEffectSystemXmlRuntime {
public:
    ~IEffectSystemXmlFileRuntime() override = default;
    virtual void BeginScriptScope() = 0;
    virtual void EndScriptScope() = 0;
    [[nodiscard]] virtual std::uint32_t OpenFile(
        std::string_view path) = 0;
    virtual void CloseFile(std::uint32_t file) = 0;
    [[nodiscard]] virtual std::uint32_t CreateFileAdapter(
        std::uint32_t file) = 0;
    virtual void DestroyFileAdapter(std::uint32_t adapter) = 0;
    [[nodiscard]] virtual std::uint32_t CreateXmlDocument() = 0;
    virtual void DestroyXmlDocument(std::uint32_t xml) = 0;
    [[nodiscard]] virtual bool ParseXml(std::uint32_t adapter,
                                        std::uint32_t xml) = 0;
};

class EffectSystemTree final {
public:
    // IDA 0x62f430: find the first cached name not below a key.
    [[nodiscard]] static std::uint32_t LowerBound(
        std::uint32_t tree, std::uint32_t key,
        IEffectSystemTreeRuntime& runtime);
    // IDA 0x62f490: byte-comparison variant of the same lookup.
    [[nodiscard]] static std::uint32_t LowerBoundByBytes(
        std::uint32_t tree, std::uint32_t key,
        IEffectSystemTreeRuntime& runtime);
};

enum class EffectSystemEmbeddedTreeKind : std::uint8_t {
    First,
    Second,
    Third,
    Fourth,
    Fifth,
};

class IEffectSystemEmbeddedTreeRuntime {
public:
    virtual ~IEffectSystemEmbeddedTreeRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t SharedNil(
        EffectSystemEmbeddedTreeKind kind) = 0;
    virtual void ClearSharedNil(EffectSystemEmbeddedTreeKind kind) = 0;
    [[nodiscard]] virtual std::uint32_t ReleaseSharedNilReference(
        EffectSystemEmbeddedTreeKind kind) = 0;
    virtual void DeleteSubtree(EffectSystemEmbeddedTreeKind kind,
                               std::uint32_t root) = 0;
    virtual void ReadFirstElement(std::uint32_t tree) = 0;
    [[nodiscard]] virtual std::uint32_t Next(
        EffectSystemEmbeddedTreeKind kind,
        std::uint32_t node) = 0;
    virtual void RemoveNode(EffectSystemEmbeddedTreeKind kind,
                            std::uint32_t tree,
                            std::uint32_t node) = 0;
    [[nodiscard]] virtual std::uint32_t Free(
        std::uint32_t pointer) = 0;
};

class IEffectSystemEmbeddedTreeConstructRuntime {
public:
    virtual ~IEffectSystemEmbeddedTreeConstructRuntime() = default;
    virtual void WriteByte(std::uint32_t address,
                           std::uint8_t value) = 0;
    virtual void InitializeTree(std::uint32_t tree) = 0;
};

class IEffectSystemTreeStorageRuntime {
public:
    virtual ~IEffectSystemTreeStorageRuntime() = default;
    [[nodiscard]] virtual std::uint32_t SharedNil(
        EffectSystemEmbeddedTreeKind kind) = 0;
    virtual void SetSharedNil(EffectSystemEmbeddedTreeKind kind,
                              std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t SharedNilReferences(
        EffectSystemEmbeddedTreeKind kind) = 0;
    virtual void SetSharedNilReferences(EffectSystemEmbeddedTreeKind kind,
                                        std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    virtual void Free(std::uint32_t pointer) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
};

class IEffectSystemFirstTreeInsertRuntime
    : public IMonsterManagerTreeInsertRuntime {
public:
    ~IEffectSystemFirstTreeInsertRuntime() override = default;
    [[nodiscard]] std::uint32_t NodeSize() const final { return 28u; }
    [[nodiscard]] std::uint32_t ColorOffset() const final { return 24u; }
    [[nodiscard]] std::int32_t CompareRange(
        std::uint32_t input, std::uint32_t node_data) final;
    [[nodiscard]] std::int32_t CompareText(
        std::uint32_t first, std::uint32_t second) final;
    void AssignText(std::uint32_t destination,
                    std::uint32_t source) final;
    void SetNodeData(MonsterManagerTreeInsertResult& result,
                     std::uint32_t candidate) final;
};

class IEffectSystemThirdTreeInsertRuntime
    : public IMonsterManagerTreeInsertRuntime {
public:
    ~IEffectSystemThirdTreeInsertRuntime() override = default;
    [[nodiscard]] std::uint32_t NodeSize() const final { return 24u; }
    [[nodiscard]] std::uint32_t ColorOffset() const final { return 20u; }
    [[nodiscard]] std::int32_t CompareRange(
        std::uint32_t input, std::uint32_t node_data) final;
    [[nodiscard]] std::int32_t CompareText(
        std::uint32_t first, std::uint32_t second) final;
    void AssignText(std::uint32_t destination,
                    std::uint32_t source) final;
    void SetNodeData(MonsterManagerTreeInsertResult& result,
                     std::uint32_t candidate) final;
};

class EffectSystemEmbeddedTrees final {
public:
    // IDA 0x63a9c0: exact unsigned lookup in the first embedded tree.
    [[nodiscard]] static std::uint32_t FindFirstExact(
        std::uint32_t tree, std::uint32_t key,
        std::uint32_t nil, IArenaSequenceRuntime& runtime);
    // IDA 0x63aa30: write the first key not below the input to a result slot.
    static void LowerBoundFirstInto(
        std::uint32_t tree, std::uint32_t& result, std::uint32_t key,
        std::uint32_t nil, IArenaSequenceRuntime& runtime);
    // IDA 0x63ace0: directly return the first key not below the input.
    [[nodiscard]] static std::uint32_t LowerBoundFirst(
        std::uint32_t tree, std::uint32_t key,
        std::uint32_t nil, IArenaSequenceRuntime& runtime);
    // IDA 0x63aa70: insert a two-word record at a known second-tree position.
    [[nodiscard]] static std::uint32_t InsertSecondAt(
        std::uint32_t tree, std::uint32_t child,
        std::uint32_t parent, std::uint32_t record,
        IEffectSystemThirdTreeInsertRuntime& runtime);
    // IDA 0x63ad10: decrement a second-tree iterator (including --end).
    [[nodiscard]] static MonsterManagerTreeStep PreviousSecond(
        std::uint32_t& iterator, std::uint32_t nil,
        IArenaSequenceRuntime& runtime);
    // IDA 0x63af90: lower bound in the fourth embedded tree.
    [[nodiscard]] static std::uint32_t LowerBoundFourth(
        std::uint32_t tree, std::uint32_t key,
        std::uint32_t nil, IArenaSequenceRuntime& runtime);
    // IDA 0x634740: release a first-tree branch right-first.
    [[nodiscard]] static std::uint32_t DeleteFirstSubtree(
        std::uint32_t root, std::uint32_t nil,
        IArenaSequenceRuntime& runtime);
    // IDA 0x634780: acquire shared nil and initialize a 28-byte tree head.
    [[nodiscard]] static std::uint32_t InitializeFirstStorage(
        std::uint32_t tree, IEffectSystemTreeStorageRuntime& runtime);
    // IDA 0x634800: allocate and link a first-tree node at a known parent.
    [[nodiscard]] static std::uint32_t InsertFirstAt(
        std::uint32_t tree, std::uint32_t child,
        std::uint32_t parent, std::uint32_t record,
        IEffectSystemFirstTreeInsertRuntime& runtime);
    // IDA 0x634a70: promote the right child.
    [[nodiscard]] static std::uint32_t PromoteFirstRightChild(
        std::uint32_t tree, std::uint32_t node,
        IEffectSystemFirstTreeInsertRuntime& runtime);
    // IDA 0x634ad0: find the leftmost node from a child link.
    [[nodiscard]] static std::uint32_t FirstMinimumFromLink(
        std::uint32_t link, std::uint32_t nil,
        IArenaSequenceRuntime& runtime);
    // IDA 0x634af0: promote the left child.
    [[nodiscard]] static std::uint32_t PromoteFirstLeftChild(
        std::uint32_t tree, std::uint32_t node,
        IEffectSystemFirstTreeInsertRuntime& runtime);
    // IDA 0x634b50..0x634c70: second/third-tree storage and branch helpers.
    [[nodiscard]] static std::uint32_t DeleteSecondSubtree(
        std::uint32_t root, std::uint32_t nil,
        IArenaSequenceRuntime& runtime);
    [[nodiscard]] static std::uint32_t InitializeSecondStorage(
        std::uint32_t tree, IEffectSystemTreeStorageRuntime& runtime);
    [[nodiscard]] static std::uint32_t SecondMinimumFromLink(
        std::uint32_t link, std::uint32_t nil,
        IArenaSequenceRuntime& runtime);
    [[nodiscard]] static std::uint32_t DeleteThirdSubtree(
        std::uint32_t root, std::uint32_t nil,
        IArenaSequenceRuntime& runtime);
    [[nodiscard]] static std::uint32_t InitializeThirdStorage(
        std::uint32_t tree, IEffectSystemTreeStorageRuntime& runtime);
    // IDA 0x634cf0..0x634fe0: positioned insert and link helpers for tree three.
    [[nodiscard]] static std::uint32_t InsertThirdAt(
        std::uint32_t tree, std::uint32_t child,
        std::uint32_t parent, std::uint32_t record,
        IEffectSystemThirdTreeInsertRuntime& runtime);
    [[nodiscard]] static std::uint32_t PromoteThirdRightChild(
        std::uint32_t tree, std::uint32_t node,
        IEffectSystemThirdTreeInsertRuntime& runtime);
    [[nodiscard]] static std::uint32_t ThirdMinimumFromLink(
        std::uint32_t link, std::uint32_t nil,
        IArenaSequenceRuntime& runtime);
    [[nodiscard]] static std::uint32_t PromoteThirdLeftChild(
        std::uint32_t tree, std::uint32_t node,
        IEffectSystemThirdTreeInsertRuntime& runtime);
    // IDA 0x635040: erase one fourth-tree node and repair red-black links.
    static void EraseFourthNode(
        std::uint32_t tree, std::uint32_t result,
        std::uint32_t node, std::uint32_t nil,
        IArenaSequenceRuntime& runtime);
    // IDA 0x635420..0x6354e0: fourth-tree release, storage and insertion.
    [[nodiscard]] static std::uint32_t DeleteFourthSubtree(
        std::uint32_t root, std::uint32_t nil,
        IArenaSequenceRuntime& runtime);
    [[nodiscard]] static std::uint32_t InitializeFourthStorage(
        std::uint32_t tree, IEffectSystemTreeStorageRuntime& runtime);
    [[nodiscard]] static std::uint32_t InsertFourthAt(
        std::uint32_t tree, std::uint32_t child,
        std::uint32_t parent, std::uint32_t record,
        IEffectSystemThirdTreeInsertRuntime& runtime);
    // IDA 0x635750..0x635810: fifth-tree release, storage and insertion.
    [[nodiscard]] static std::uint32_t DeleteFifthSubtree(
        std::uint32_t root, std::uint32_t nil,
        IArenaSequenceRuntime& runtime);
    [[nodiscard]] static std::uint32_t InitializeFifthStorage(
        std::uint32_t tree, IEffectSystemTreeStorageRuntime& runtime);
    [[nodiscard]] static std::uint32_t InsertFifthAt(
        std::uint32_t tree, std::uint32_t child,
        std::uint32_t parent, std::uint32_t record,
        IEffectSystemThirdTreeInsertRuntime& runtime);
    // IDA 0x635a80: allocate a bare first-tree node.
    [[nodiscard]] static std::uint32_t CreateFirstNode(
        std::uint32_t parent, std::uint32_t color,
        IEffectSystemTreeStorageRuntime& runtime);
    // IDA 0x635aa0..0x635c80: predecessor cursors and fourth-tree rotations.
    [[nodiscard]] static MonsterManagerTreeStep PreviousFirst(
        std::uint32_t& iterator, std::uint32_t nil,
        IArenaSequenceRuntime& runtime);
    [[nodiscard]] static MonsterManagerTreeStep PreviousThird(
        std::uint32_t& iterator, std::uint32_t nil,
        IArenaSequenceRuntime& runtime);
    [[nodiscard]] static std::uint32_t PromoteFourthRightChild(
        std::uint32_t tree, std::uint32_t node,
        IEffectSystemThirdTreeInsertRuntime& runtime);
    [[nodiscard]] static std::uint32_t PromoteFourthLeftChild(
        std::uint32_t tree, std::uint32_t node,
        IEffectSystemThirdTreeInsertRuntime& runtime);
    [[nodiscard]] static MonsterManagerTreeStep PreviousFourth(
        std::uint32_t& iterator, std::uint32_t nil,
        IArenaSequenceRuntime& runtime);
    [[nodiscard]] static MonsterManagerTreeStep PreviousFifth(
        std::uint32_t& iterator, std::uint32_t nil,
        IArenaSequenceRuntime& runtime);
    // IDA 0x62f7c0: clear the first embedded effect tree.
    [[nodiscard]] static std::uint32_t CleanupFirst(
        std::uint32_t tree, IEffectSystemEmbeddedTreeRuntime& runtime);
    // IDA 0x62f880: clear the second embedded effect tree.
    [[nodiscard]] static std::uint32_t CleanupSecond(
        std::uint32_t tree, IEffectSystemEmbeddedTreeRuntime& runtime);
    // IDA 0x62f940: clear the third embedded effect tree.
    [[nodiscard]] static std::uint32_t CleanupThird(
        std::uint32_t tree, IEffectSystemEmbeddedTreeRuntime& runtime);
    // IDA 0x62fa00: clear the fourth embedded effect tree.
    [[nodiscard]] static std::uint32_t CleanupFourth(
        std::uint32_t tree, IEffectSystemEmbeddedTreeRuntime& runtime);
    // IDA 0x62fd90: clear the fifth embedded effect tree.
    [[nodiscard]] static std::uint32_t CleanupFifth(
        std::uint32_t tree, IEffectSystemEmbeddedTreeRuntime& runtime);
    // IDA 0x632750..0x632a10: destructor-path variants omit the first-element read.
    [[nodiscard]] static std::uint32_t CleanupFirstDirect(
        std::uint32_t tree, IEffectSystemEmbeddedTreeRuntime& runtime);
    [[nodiscard]] static std::uint32_t CleanupSecondDirect(
        std::uint32_t tree, IEffectSystemEmbeddedTreeRuntime& runtime);
    [[nodiscard]] static std::uint32_t CleanupThirdDirect(
        std::uint32_t tree, IEffectSystemEmbeddedTreeRuntime& runtime);
    [[nodiscard]] static std::uint32_t CleanupFourthDirect(
        std::uint32_t tree, IEffectSystemEmbeddedTreeRuntime& runtime);
    [[nodiscard]] static std::uint32_t CleanupFifthDirect(
        std::uint32_t tree, IEffectSystemEmbeddedTreeRuntime& runtime);
    // IDA 0x632ac0: copy three tree flags before initializing its head and nil.
    [[nodiscard]] static std::uint32_t InitializeFirst(
        std::uint32_t tree, std::uint8_t first_flag,
        std::uint8_t second_flag, std::uint8_t duplicate_mode,
        IEffectSystemEmbeddedTreeConstructRuntime& runtime);
    // IDA 0x632af0: insert a three-word record keyed by its first word.
    [[nodiscard]] static MonsterManagerTreeInsertResult InsertFirst(
        std::uint32_t tree, std::uint32_t record,
        IEffectSystemFirstTreeInsertRuntime& runtime);
    // IDA 0x632d10: erase a first-tree node without text-record teardown.
    static void EraseFirstNode(std::uint32_t tree, std::uint32_t result,
                               std::uint32_t node, std::uint32_t nil,
                               IArenaSequenceRuntime& runtime);
    // IDA 0x6330f0: erase a range or the complete first tree.
    static void EraseFirstRange(std::uint32_t tree, std::uint32_t result,
                                std::uint32_t first, std::uint32_t last,
                                std::uint32_t nil,
                                IArenaSequenceRuntime& runtime);
    // IDA 0x6331c0: advance a first-tree iterator in key order.
    [[nodiscard]] static MonsterManagerTreeStep AdvanceFirst(
        std::uint32_t& iterator, std::uint32_t nil,
        IArenaSequenceRuntime& runtime);
    // IDA 0x633210: initialize the second embedded tree's flags and head.
    [[nodiscard]] static std::uint32_t InitializeSecond(
        std::uint32_t tree, std::uint8_t first_flag,
        std::uint8_t second_flag, std::uint8_t duplicate_mode,
        IEffectSystemEmbeddedTreeConstructRuntime& runtime);
    // IDA 0x633240: unlink and rebalance a second-tree node (color at +20).
    static void EraseSecondNode(std::uint32_t tree, std::uint32_t result,
                                std::uint32_t node, std::uint32_t nil,
                                IArenaSequenceRuntime& runtime);
    // IDA 0x633620: erase a half-open second-tree range.
    static void EraseSecondRange(std::uint32_t tree, std::uint32_t result,
                                 std::uint32_t first, std::uint32_t last,
                                 std::uint32_t nil,
                                 IArenaSequenceRuntime& runtime);
    // IDA 0x6336f0: advance a second-tree iterator.
    [[nodiscard]] static MonsterManagerTreeStep AdvanceSecond(
        std::uint32_t& iterator, std::uint32_t nil,
        IArenaSequenceRuntime& runtime);
    // IDA 0x633810: insert a two-word record in the third embedded tree.
    [[nodiscard]] static MonsterManagerTreeInsertResult InsertThird(
        std::uint32_t tree, std::uint32_t record,
        IEffectSystemThirdTreeInsertRuntime& runtime);
    // IDA 0x633a30: erase one third-tree node.
    static void EraseThirdNode(std::uint32_t tree, std::uint32_t result,
                               std::uint32_t node, std::uint32_t nil,
                               IArenaSequenceRuntime& runtime);
    // IDA 0x633e10: erase a half-open third-tree range.
    static void EraseThirdRange(std::uint32_t tree, std::uint32_t result,
                                std::uint32_t first, std::uint32_t last,
                                std::uint32_t nil,
                                IArenaSequenceRuntime& runtime);
    // IDA 0x633ee0: find an exact unsigned key or return the header.
    [[nodiscard]] static std::uint32_t FindThird(
        std::uint32_t tree, std::uint32_t key,
        std::uint32_t nil, IArenaSequenceRuntime& runtime);
    // IDA 0x633f50: advance a third-tree iterator.
    [[nodiscard]] static MonsterManagerTreeStep AdvanceThird(
        std::uint32_t& iterator, std::uint32_t nil,
        IArenaSequenceRuntime& runtime);
    // IDA 0x633fa0: construct the third tree's flags and head.
    [[nodiscard]] static std::uint32_t InitializeThird(
        std::uint32_t tree, std::uint8_t first_flag,
        std::uint8_t second_flag, std::uint8_t duplicate_mode,
        IEffectSystemEmbeddedTreeConstructRuntime& runtime);
    // IDA 0x633fd0: insert a two-word record in the fourth tree.
    [[nodiscard]] static MonsterManagerTreeInsertResult InsertFourth(
        std::uint32_t tree, std::uint32_t record,
        IEffectSystemThirdTreeInsertRuntime& runtime);
    // IDA 0x6341f0: empty the fourth tree while keeping its head and nil.
    [[nodiscard]] static std::uint32_t ClearFourth(
        std::uint32_t tree, std::uint32_t result,
        std::uint32_t nil, IArenaSequenceRuntime& runtime);
    // IDA 0x634290: advance a fourth-tree iterator.
    [[nodiscard]] static MonsterManagerTreeStep AdvanceFourth(
        std::uint32_t& iterator, std::uint32_t nil,
        IArenaSequenceRuntime& runtime);
    // IDA 0x6342e0: construct the fifth tree's flags and head.
    [[nodiscard]] static std::uint32_t InitializeFifth(
        std::uint32_t tree, std::uint8_t first_flag,
        std::uint8_t second_flag, std::uint8_t duplicate_mode,
        IEffectSystemEmbeddedTreeConstructRuntime& runtime);
    // IDA 0x634310: erase one fifth-tree node.
    static void EraseFifthNode(std::uint32_t tree, std::uint32_t result,
                               std::uint32_t node, std::uint32_t nil,
                               IArenaSequenceRuntime& runtime);
    // IDA 0x6346f0: advance a fifth-tree iterator.
    [[nodiscard]] static MonsterManagerTreeStep AdvanceFifth(
        std::uint32_t& iterator, std::uint32_t nil,
        IArenaSequenceRuntime& runtime);
};

class EffectSystem final {
public:
    static constexpr std::uint32_t kObjectSize = 260u;

    // IDA 0x62f070: release XML payloads and erase all instance nodes.
    [[nodiscard]] static std::uint32_t CleanupInstances(
        std::uint32_t system, IEffectSystemCleanupRuntime& runtime);
    // IDA 0x62f0c0: create an effect from cached or newly parsed XML.
    [[nodiscard]] static std::uint32_t LoadEffectFromFile(
        std::uint32_t system, std::string_view name,
        IEffectSystemLoadRuntime& runtime);
    // IDA 0x62f550: initialize trees, list heads and effect-system vtable.
    [[nodiscard]] static std::uint32_t Initialize(
        std::uint32_t system, IEffectSystemInitRuntime& runtime);
    // IDA 0x62fac0: tear down embedded lists and trees in reverse order.
    [[nodiscard]] static std::uint32_t Destroy(
        std::uint32_t system, IEffectSystemDestroyRuntime& runtime);
    // IDA 0x62fe50: sum twelve list counts and the inline-state count.
    [[nodiscard]] static std::uint32_t TotalCount(
        std::uint32_t system, IEffectSystemCountRuntime& runtime);
    // IDA 0x62feb0: reset and attach an effect to the active manager.
    [[nodiscard]] static std::uint8_t Enable(
        std::uint32_t system, std::uint32_t effect,
        std::uint32_t ignored, IEffectSystemEnableRuntime& runtime);
    // IDA 0x62ff00: detach an enabled effect and clear its lists.
    static void Disable(std::uint32_t system,
                        IEffectSystemDisableRuntime& runtime);
    // IDA 0x6300d0: update active effect stages with scaled frame time.
    [[nodiscard]] static int UpdateFrame(
        std::uint32_t system, double seconds,
        IEffectSystemFrameRuntime& runtime);
    // IDA 0x630160: render five effect groups under temporary device state.
    [[nodiscard]] static std::uint8_t Draw(
        std::uint32_t system, std::uint32_t context,
        IEffectSystemDrawRuntime& runtime);
    // IDA 0x630270: process clump atomics with the +52 geometry data.
    [[nodiscard]] static std::uint8_t ProcessClumpAtomics(
        std::uint32_t system, std::uint32_t clump,
        IEffectSystemClumpRuntime& runtime);
    // IDA 0x630290: process clump atomics through the RenderWare wrapper.
    [[nodiscard]] static std::uint8_t ProcessClumpAtomicsWrapper(
        std::uint32_t system, std::uint32_t clump,
        IEffectSystemClumpRuntime& runtime);
    // IDA 0x6302b0: dispatch an effect object to its type-specific queue.
    [[nodiscard]] static std::uint8_t QueueObject(
        std::uint32_t system, std::uint32_t object,
        IEffectSystemObjectQueueRuntime& runtime);
    // IDA 0x630600: remove an effect object from its type-specific queue.
    [[nodiscard]] static bool RemoveObject(
        std::uint32_t system, std::uint32_t object,
        IEffectSystemObjectRemoveRuntime& runtime);
    // IDA 0x630990: acquire, start, pause and register an effect object.
    [[nodiscard]] static EffectSystemHandle CreateEffectObject(
        std::uint32_t system, std::string_view name,
        std::uint32_t position, std::uint32_t repeat_count,
        std::uint8_t flag, IEffectSystemCreateRuntime& runtime);
    // IDA 0x630e10: create an effect object without pausing playback.
    [[nodiscard]] static EffectSystemHandle CreateEffectObjectWithoutPause(
        std::uint32_t system, std::string_view name,
        std::uint32_t position, std::uint32_t repeat_count,
        std::uint8_t flag, IEffectSystemCreateRuntime& runtime);
    // IDA 0x631280: load an XML effect through two factory paths.
    [[nodiscard]] static EffectSystemHandle LoadEffectFromXml(
        std::uint32_t system, std::uint32_t xml,
        std::uint32_t context, IEffectSystemXmlRuntime& runtime);
    // IDA 0x631340: open and parse a named .hef script before loading.
    [[nodiscard]] static EffectSystemHandle LoadEffectFromXmlFile(
        std::uint32_t system, std::string_view name,
        std::uint32_t context, IEffectSystemXmlFileRuntime& runtime);
    // IDA 0x6315c0: release objects in four effect trees.
    [[nodiscard]] static std::uint32_t CleanupEffects(
        std::uint32_t system, IEffectSystemCleanupEffectsRuntime& runtime);
    // IDA 0x631880: erase inactive entries from the inline effect tree.
    [[nodiscard]] static std::uint32_t ProcessEffectList(
        std::uint32_t system, IEffectSystemPruneRuntime& runtime);
    // IDA 0x631900: insert an object and its lifetime into the timed tree.
    [[nodiscard]] static bool InsertTimedEffect(
        std::uint32_t system, std::uint32_t effect,
        float lifetime, IEffectSystemTimedRuntime& runtime);
    // IDA 0x6319c0: update and retire timed effects.
    static void UpdateEffectTimers(
        std::uint32_t system, double seconds,
        IEffectSystemTimedRuntime& runtime);
    // IDA 0x631a80: draw each object in the timed-effect tree.
    static void DrawTimedEffects(
        std::uint32_t system, std::uint32_t context,
        IEffectSystemTimedRuntime& runtime);
    // IDA 0x631af0: update active playback and repetition state.
    static void UpdateActiveEffects(
        std::uint32_t system, double seconds,
        IEffectSystemActiveFrameRuntime& runtime);
    // IDA 0x631f40: expire effects in the second active tree.
    static void UpdateSecondaryTimers(
        std::uint32_t system, double seconds,
        IEffectSystemSecondaryTimerRuntime& runtime);
    // IDA 0x631ff0: update twelve object queues in native order.
    static void UpdateObjectGroups(
        std::uint32_t system, std::uint32_t first,
        std::uint32_t second, IEffectSystemObjectGroupRuntime& runtime);
    // IDA 0x632270: update objects in the XML-effect tree.
    static void UpdateXmlEffects(
        std::uint32_t system, std::uint32_t first,
        std::uint32_t second, IEffectSystemTreeUpdateRuntime& runtime);
    // IDA 0x6322f0: update active inline objects, then prune inactive nodes.
    static void UpdateInlineEffects(
        std::uint32_t system, std::uint32_t first,
        std::uint32_t second, IEffectSystemTreeUpdateRuntime& runtime);
    // IDA 0x632390: traverse active effects through a native no-op draw hook.
    static void DrawActivePlaceholder(
        std::uint32_t system, std::uint32_t context,
        IEffectSystemGroupDrawRuntime& runtime);
    // IDA 0x632400: draw twelve object groups around a render-state change.
    static void DrawObjectGroups(
        std::uint32_t system, std::uint32_t context,
        IEffectSystemGroupDrawRuntime& runtime);
    // IDA 0x632590: draw XML effects through object slot +16.
    static void DrawXmlEffects(
        std::uint32_t system, std::uint32_t context,
        IEffectSystemGroupDrawRuntime& runtime);
    // IDA 0x632600: draw all inline effects through object slot +76.
    static void DrawInlineEffects(
        std::uint32_t system, std::uint32_t context,
        IEffectSystemGroupDrawRuntime& runtime);
    // IDA 0x632670: bracket geometry sorting with temporary D3D9 states.
    static void SetRenderStates(
        std::uint32_t system, IEffectSystemRenderStateRuntime& runtime);
};

}  // namespace pal4
