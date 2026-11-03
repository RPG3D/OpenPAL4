#pragma once

#include <cstdint>

namespace pal4 {

struct RwTexDictionaryTreeState final {
    std::uint8_t tag{};
    std::uint8_t second{};
    std::uint8_t unused2{};
    std::uint8_t unused3{};
    std::uint32_t head{};
    std::uint8_t flag{};
    std::uint8_t unused9{};
    std::uint8_t unused10{};
    std::uint8_t unused11{};
    std::uint32_t count{};
};
static_assert(sizeof(RwTexDictionaryTreeState) == 16u);

class IRwTexDictionaryTreeReader {
public:
    virtual ~IRwTexDictionaryTreeReader() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t SharedNil() = 0;
};

class IRwTexDictionaryTreeRuntime : public IRwTexDictionaryTreeReader {
public:
    virtual ~IRwTexDictionaryTreeRuntime() = default;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    [[nodiscard]] virtual std::uint32_t Free(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    virtual void WriteByte(std::uint32_t address,
                           std::uint8_t value) = 0;
    virtual void SetSharedNil(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t Users() = 0;
    virtual void SetUsers(std::uint32_t count) = 0;
    virtual void FirstElement(RwTexDictionaryTreeState& state) = 0;
    virtual void DestroyDictionary(std::uint32_t dictionary) = 0;
};

struct RwTexDictionaryName final {
    std::uint8_t tag{};
    std::uint8_t unused1{};
    std::uint8_t unused2{};
    std::uint8_t unused3{};
    std::uint32_t buffer{};
    std::uint32_t size{};
    std::uint32_t capacity{};
};
static_assert(sizeof(RwTexDictionaryName) == 16u);

class IRwTexDictionaryInsertRuntime {
public:
    virtual ~IRwTexDictionaryInsertRuntime() = default;
    virtual void InitializeName(RwTexDictionaryName& name,
                                std::uint8_t tag) = 0;
    [[nodiscard]] virtual bool AllocateName(RwTexDictionaryName& name,
                                            std::uint32_t length,
                                            bool terminate) = 0;
    virtual void CopyName(RwTexDictionaryName& name, const char* source,
                          std::uint32_t length) = 0;
    virtual void SetNameSize(RwTexDictionaryName& name,
                             std::uint32_t length) = 0;
    virtual void SetExceptionMessage(RwTexDictionaryName& name,
                                     const char* source,
                                     std::uint32_t length) = 0;
    [[nodiscard]] virtual int CompareRange(
        std::uint32_t node_length, std::uint32_t name_buffer,
        std::uint32_t name_length) = 0;
    [[nodiscard]] virtual int CompareNodeName(
        const RwTexDictionaryName& name,
        std::uint32_t node_name_address) = 0;
    [[nodiscard]] virtual int CompareNodeAgainstName(
        std::uint32_t node_name_address,
        const RwTexDictionaryName& name) = 0;
    [[nodiscard]] virtual int CompareBytes(
        std::uint32_t left_buffer, std::uint32_t right_buffer,
        std::uint32_t length) = 0;
    virtual void SetCurrentDictionary(std::uint32_t dictionary) = 0;
    virtual void AssignName(RwTexDictionaryName& target,
                            const RwTexDictionaryName& source,
                            std::uint32_t start,
                            std::uint32_t count) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    virtual void WriteByte(std::uint32_t address,
                           std::uint8_t value) = 0;
    [[nodiscard]] virtual std::uint32_t FreeNameBuffer(
        std::uint32_t address) = 0;
};

struct RwTexDictionaryEntry final {
    RwTexDictionaryName name{};
    std::uint32_t dictionary{};
    std::uint32_t references{};
};
static_assert(sizeof(RwTexDictionaryEntry) == 24u);

struct RwTexDictionaryInsertResult final {
    std::uint32_t node{};
    bool inserted{};
};

class IRwTexDictionaryNodeInsertRuntime : public IRwTexDictionaryTreeReader {
public:
    virtual ~IRwTexDictionaryNodeInsertRuntime() = default;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    [[nodiscard]] virtual int CompareRange(
        const RwTexDictionaryName& name,
        std::uint32_t node_buffer, std::uint32_t node_length) = 0;
    [[nodiscard]] virtual int CompareNameToNode(
        const RwTexDictionaryName& name,
        std::uint32_t node_name_address) = 0;
    [[nodiscard]] virtual int CompareNodeToName(
        std::uint32_t node_name_address,
        const RwTexDictionaryName& name) = 0;
    [[nodiscard]] virtual std::uint32_t AllocateNode(
        std::uint32_t bytes) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    virtual void WriteByte(std::uint32_t address,
                           std::uint8_t value) = 0;
    [[nodiscard]] virtual std::uint32_t EmptyNameBuffer() = 0;
    virtual void SetNameErrorFlag(std::uint32_t name,
                                  std::uint8_t flag) = 0;
    [[nodiscard]] virtual bool AllocateNameBuffer(
        std::uint32_t name, std::uint32_t length,
        bool terminate) = 0;
    virtual void CopyNameBytes(std::uint32_t destination,
                               std::uint32_t source,
                               std::uint32_t length) = 0;
    virtual void DetachNameBuffer(std::uint32_t name) = 0;
};

class RwTexDictionaryTree final {
public:
    // IDA 0x6640a0 and 0x664130.
    [[nodiscard]] static RwTexDictionaryTreeState* Construct(
        RwTexDictionaryTreeState& state, std::uint8_t tag,
        std::uint8_t second, IRwTexDictionaryTreeRuntime& runtime);
    [[nodiscard]] static std::uint32_t Destruct(
        RwTexDictionaryTreeState& state,
        IRwTexDictionaryTreeRuntime& runtime);
    // IDA 0x6641f0. Keeps the shared nil and head allocated.
    [[nodiscard]] static std::uint32_t Cleanup(
        RwTexDictionaryTreeState& state,
        IRwTexDictionaryTreeRuntime& runtime);
    // IDA 0x6642f0.
    [[nodiscard]] static bool AddTexture(
        RwTexDictionaryTreeState& state, const char* name,
        std::uint32_t texture, IRwTexDictionaryInsertRuntime& names,
        IRwTexDictionaryNodeInsertRuntime& nodes);
    // IDA 0x664460, despite its generic "RemoveTexture" name.
    [[nodiscard]] static std::uint32_t ReleaseName(
        RwTexDictionaryName& name,
        IRwTexDictionaryInsertRuntime& runtime);
    // IDA 0x6644a0.
    [[nodiscard]] static bool RemoveTextureByName(
        RwTexDictionaryTreeState& state, const char* name,
        IRwTexDictionaryInsertRuntime& names,
        IRwTexDictionaryTreeRuntime& tree);
    // IDA 0x6645d0.
    [[nodiscard]] static bool SetCurrent(
        RwTexDictionaryTreeState& state, const char* name,
        IRwTexDictionaryInsertRuntime& names,
        IRwTexDictionaryTreeRuntime& tree);
    // IDA 0x664720.
    [[nodiscard]] static bool CheckTextureExists(
        RwTexDictionaryTreeState& state, const char* name,
        IRwTexDictionaryInsertRuntime& names,
        IRwTexDictionaryTreeReader& tree);
    // IDA 0x664800.
    [[nodiscard]] static RwTexDictionaryInsertResult InsertInternal(
        RwTexDictionaryTreeState& state,
        const RwTexDictionaryEntry& entry,
        IRwTexDictionaryNodeInsertRuntime& runtime);
    // IDA 0x664a60: returns the next iterator node.
    [[nodiscard]] static std::uint32_t RemoveNodeAndRebalance(
        RwTexDictionaryTreeState& state, std::uint32_t node,
        IRwTexDictionaryTreeRuntime& runtime);
    // IDA 0x664e70.
    [[nodiscard]] static std::uint32_t FindNode(
        RwTexDictionaryTreeState& state,
        const RwTexDictionaryName& name,
        IRwTexDictionaryInsertRuntime& names,
        IRwTexDictionaryTreeReader& tree);
    // IDA 0x664f20, named Predecessor in IDA but advances in order.
    [[nodiscard]] static std::uint32_t NextNode(
        std::uint32_t node, IRwTexDictionaryTreeReader& tree);
    // IDA 0x664f70: lower-bound candidate for SetCurrent.
    [[nodiscard]] static std::uint32_t FindNodeCandidate(
        RwTexDictionaryTreeState& state,
        const RwTexDictionaryName& name,
        IRwTexDictionaryInsertRuntime& names,
        IRwTexDictionaryTreeReader& tree);
    // IDA 0x664fd0: destroys the right subtree before each left-chain node.
    [[nodiscard]] static std::uint32_t DestroyRecursive(
        std::uint32_t root, IRwTexDictionaryTreeRuntime& runtime);
    // IDA 0x665020: allocates, links and balances a 40-byte tree node.
    [[nodiscard]] static std::uint32_t InsertNodeAndRebalance(
        RwTexDictionaryTreeState& state, std::uint32_t leaf,
        std::uint32_t parent, const RwTexDictionaryEntry& entry,
        IRwTexDictionaryNodeInsertRuntime& runtime);
    // IDA 0x665290 and 0x6652f0.
    [[nodiscard]] static std::uint32_t RotateLeft(
        RwTexDictionaryTreeState& state, std::uint32_t node,
        IRwTexDictionaryNodeInsertRuntime& runtime);
    [[nodiscard]] static std::uint32_t RotateRight(
        RwTexDictionaryTreeState& state, std::uint32_t node,
        IRwTexDictionaryNodeInsertRuntime& runtime);
    // IDA 0x665350: lower-bound lookup using bytewise name ordering.
    [[nodiscard]] static std::uint32_t FindNodeForExists(
        RwTexDictionaryTreeState& state,
        const RwTexDictionaryName& name,
        IRwTexDictionaryInsertRuntime& names,
        IRwTexDictionaryTreeReader& tree);
    // IDA 0x6653e0: allocates 40 bytes and initializes parent and color.
    [[nodiscard]] static std::uint32_t CreateNode(
        std::uint32_t parent, std::uint32_t color,
        IRwTexDictionaryNodeInsertRuntime& runtime);
    // IDA 0x665400: moves an iterator to its in-order predecessor.
    [[nodiscard]] static std::uint32_t PreviousNode(
        std::uint32_t node, IRwTexDictionaryTreeReader& tree);
    // IDA 0x665460: copies a 24-byte entry, including name ownership.
    static void CopyNodeData(
        std::uint32_t destination, const RwTexDictionaryEntry& source,
        IRwTexDictionaryNodeInsertRuntime& runtime);
    static void CopyNodeData(
        std::uint32_t destination, std::uint32_t source,
        IRwTexDictionaryNodeInsertRuntime& runtime);
    // IDA 0x6655c0: releases embedded name and optionally frees the payload.
    [[nodiscard]] static std::uint32_t DestroyNode(
        std::uint32_t payload, std::uint32_t flags,
        IRwTexDictionaryTreeRuntime& runtime);
};

}  // namespace pal4
