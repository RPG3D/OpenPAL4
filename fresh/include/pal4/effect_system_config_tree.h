#pragma once

#include <cstdint>
#include <string_view>

namespace pal4 {

class IUtilitiesTextRecordCopyRuntime;

class IEffectSystemConfigTreeRuntime {
public:
    virtual ~IEffectSystemConfigTreeRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t FindLowerBound(
        std::uint32_t tree, std::uint32_t key) = 0;
    [[nodiscard]] virtual std::int32_t CompareText(
        std::uint32_t left, std::uint32_t right) = 0;
    [[nodiscard]] virtual std::uint32_t CopyKey(std::uint32_t key) = 0;
    [[nodiscard]] virtual std::uint32_t CopyConfig(std::uint32_t config) = 0;
    virtual void CopyNameField(std::uint32_t copy,
                               std::uint32_t source) = 0;
    virtual void CopyTailField(std::uint32_t copy,
                               std::uint32_t source) = 0;
    [[nodiscard]] virtual std::uint8_t Insert(
        std::uint32_t tree, std::uint32_t key_copy,
        std::uint32_t config_copy) = 0;
    virtual void DestroyConfigCopy(std::uint32_t copy) = 0;
    virtual void ReleaseKeyCopy(std::uint32_t copy) = 0;
};

class IEffectSystemConfigNodeRuntime {
public:
    virtual ~IEffectSystemConfigNodeRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t SharedNil() const = 0;
    [[nodiscard]] virtual std::int32_t CompareSearch(
        std::uint32_t key, std::uint32_t node_payload) = 0;
    [[nodiscard]] virtual std::int32_t CompareText(
        std::uint32_t left_payload,
        std::uint32_t right_payload) = 0;
    [[nodiscard]] virtual std::uint32_t AllocateNode(
        std::uint32_t bytes) = 0;
    virtual void CopyPayload(std::uint32_t destination,
                             std::uint32_t source) = 0;
    virtual void FreeNode(std::uint32_t node) = 0;
};

class IEffectSystemCompositeCopyRuntime {
public:
    virtual ~IEffectSystemCompositeCopyRuntime() = default;
    [[nodiscard]] virtual std::uint8_t ReadByte(
        std::uint32_t address) = 0;
    virtual void WriteByte(std::uint32_t address,
                           std::uint8_t value) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(
        std::uint32_t bytes) = 0;
    virtual void AssignText(std::uint32_t destination,
                            std::uint32_t source) = 0;
    virtual void SetErrorFlag(std::uint32_t field,
                              std::uint8_t flag) = 0;
    virtual void CopyRecord24(std::uint32_t destination,
                              std::uint32_t source) = 0;
};

struct EffectSystemConfigInsertResult final {
    std::uint32_t node = 0u;
    bool inserted = false;
};

class IEffectSystemConfigCleanupRuntime {
public:
    virtual ~IEffectSystemConfigCleanupRuntime() = default;
    virtual void DestroyConfig(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    virtual void WriteByte(std::uint32_t address,
                           std::uint8_t value) = 0;
    [[nodiscard]] virtual std::uint32_t Free(std::uint32_t address) = 0;
};

class IEffectSystemConfigLookupRuntime {
public:
    virtual ~IEffectSystemConfigLookupRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t SharedNil() const = 0;
    [[nodiscard]] virtual std::int32_t CompareText(
        std::string_view name, std::uint32_t node_text) = 0;
    [[nodiscard]] virtual std::int32_t CompareNodeToName(
        std::uint32_t node_text, std::string_view name) = 0;
};

struct EffectSystemRecordBufferState final {
    std::uint8_t flag = 0u;
    std::uint32_t begin = 0u;
    std::uint32_t end = 0u;
    std::uint32_t capacity = 0u;
};
static_assert(sizeof(EffectSystemRecordBufferState) == 16u);

class IEffectSystemRecordBufferRuntime {
public:
    virtual ~IEffectSystemRecordBufferRuntime() = default;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    virtual void CopyRecord(std::uint32_t destination,
                            std::uint32_t source) = 0;
    virtual void ReleaseText(std::uint32_t record) = 0;
    virtual void CleanupProperty(std::uint32_t record) = 0;
    virtual void Free(std::uint32_t address) = 0;
};

class EffectSystemRecordBuffer final {
public:
    // IDA 0x650690/0x650750: release two 24-byte record variants.
    static int CleanupText(EffectSystemRecordBufferState& state,
                           IEffectSystemRecordBufferRuntime& runtime);
    static int CleanupProperties(EffectSystemRecordBufferState& state,
                                 IEffectSystemRecordBufferRuntime& runtime);
    // IDA 0x6506d0: copy a buffer and its 24-byte records.
    [[nodiscard]] static EffectSystemRecordBufferState& Copy(
        EffectSystemRecordBufferState& destination,
        const EffectSystemRecordBufferState& source,
        IEffectSystemRecordBufferRuntime& runtime);
    // IDA 0x6516f0: copy-construct a 24-byte text record.
    static void CopyRecord24(std::uint32_t destination,
                             std::uint32_t source,
                             IUtilitiesTextRecordCopyRuntime& runtime);
};

class EffectSystemConfigTree final {
public:
    // IDA 0x650370: insert a copied configuration only if its key is absent.
    [[nodiscard]] static std::uint8_t InsertIfAbsent(
        std::uint32_t owner, std::uint32_t config,
        IEffectSystemConfigTreeRuntime& runtime);
    // IDA 0x6504a0: release the nested config and name buffer.
    [[nodiscard]] static std::uint32_t CleanupValue(
        std::uint32_t value,
        IEffectSystemConfigCleanupRuntime& runtime);
    // IDA 0x651ae0: deleting destructor for a copied configuration value.
    [[nodiscard]] static std::uint32_t DeleteValue(
        std::uint32_t value, std::uint8_t flags,
        IEffectSystemConfigCleanupRuntime& runtime);
    // IDA 0x651b00: release the text field at +8 and clear its three words.
    [[nodiscard]] static std::uint32_t CleanupTextField(
        std::uint32_t value,
        IEffectSystemConfigCleanupRuntime& runtime);
    // IDA 0x650510: return the matching node's payload at +28.
    [[nodiscard]] static std::uint32_t FindProperty(
        std::uint32_t owner, const char* name,
        IEffectSystemConfigLookupRuntime& runtime);
    // IDA 0x6505f0/0x650620: indexed traversal of the ordered tree.
    [[nodiscard]] static std::uint32_t FirstProperty(
        std::uint32_t owner, std::uint32_t& cursor,
        IEffectSystemConfigLookupRuntime& runtime);
    [[nodiscard]] static std::uint32_t NextProperty(
        std::uint32_t owner, std::uint32_t& cursor,
        IEffectSystemConfigLookupRuntime& runtime);
    // IDA 0x650790: insert a unique or duplicate 80-byte tree node.
    [[nodiscard]] static EffectSystemConfigInsertResult InsertElement(
        std::uint32_t tree, std::uint32_t payload,
        IEffectSystemConfigNodeRuntime& runtime);
    // IDA 0x651250: allocate, link and rebalance an 80-byte node.
    [[nodiscard]] static std::uint32_t InsertNodeAt(
        std::uint32_t tree, std::uint32_t child,
        std::uint32_t parent, std::uint32_t payload,
        IEffectSystemConfigNodeRuntime& runtime);
    // IDA 0x6509f0: erase a node, return its successor and restore RB rules.
    [[nodiscard]] static std::uint32_t EraseElement(
        std::uint32_t tree, std::uint32_t node,
        IEffectSystemConfigNodeRuntime& nodes,
        IEffectSystemConfigCleanupRuntime& cleanup);
    // IDA 0x650e30: advance a tree-node cursor to its in-order successor.
    [[nodiscard]] static std::uint32_t AdvanceNodeCursor(
        std::uint32_t& cursor,
        IEffectSystemConfigNodeRuntime& runtime);
    // IDA 0x6511a0: lower bound by node text and query bytes.
    [[nodiscard]] static std::uint32_t FindLowerBoundNode(
        std::uint32_t tree, std::string_view name,
        IEffectSystemConfigLookupRuntime& runtime);
    // IDA 0x651200: destroy right subtrees before the left spine.
    [[nodiscard]] static std::uint32_t DestroySubtree(
        std::uint32_t root, IEffectSystemConfigNodeRuntime& nodes,
        IEffectSystemConfigCleanupRuntime& cleanup);
    // IDA 0x6514c0/0x651520: rotate a node's right/left child upward.
    static void RotateLeft(std::uint32_t tree, std::uint32_t node,
                           IEffectSystemConfigNodeRuntime& runtime);
    static void RotateRight(std::uint32_t tree, std::uint32_t node,
                            IEffectSystemConfigNodeRuntime& runtime);
    // IDA 0x651580: save the old node while advancing the cursor.
    [[nodiscard]] static std::uint32_t& AdvanceNodeCursorPost(
        std::uint32_t& cursor, std::uint32_t& old,
        IEffectSystemConfigNodeRuntime& runtime);
    // IDA 0x6515e0: lower bound for a native composite string payload.
    [[nodiscard]] static std::uint32_t FindLowerBoundPayload(
        std::uint32_t tree, std::uint32_t payload,
        IEffectSystemConfigNodeRuntime& runtime);
    // IDA 0x651670: allocate a node and set its parent/color fields.
    [[nodiscard]] static std::uint32_t CreateNode(
        std::uint32_t parent, std::uint32_t color,
        IEffectSystemConfigNodeRuntime& runtime);
    // IDA 0x651690: move a cursor to its predecessor, including header.
    [[nodiscard]] static std::uint32_t AdvanceNodeCursorBackward(
        std::uint32_t& cursor,
        IEffectSystemConfigNodeRuntime& runtime);
    // IDA 0x651850: deep-copy a 64-byte composite tree payload.
    static void CopyCompositePayload(
        std::uint32_t destination, std::uint32_t source,
        IEffectSystemCompositeCopyRuntime& runtime);
};

}  // namespace pal4
