#pragma once

#include <cstdint>
#include <initializer_list>
#include <string_view>

namespace pal4 {

enum class ArenaSequenceCall : std::uint8_t {
    AllocateTreeHead,
    DeleteTreeSubtree,
    TreeFirstElement,
    EraseRemainingTreeNodes,
    UpdateAnimationTime,
    RenderClump,
    FindSequenceNode,
    ReadFps,
    ReadCount,
    ReadDir,
    ReadClumpName,
    ReadTextureName,
    ConstructClump,
    GetAssetHost,
    LoadClump,
    DestroyClump,
    OpenSequenceFile,
    InitXmlDocument,
    ParseXmlFile,
    CloseSequenceFile,
    DestroyXmlDocument,
    FirstSequenceNode,
    NextSequenceNode,
    ReadSequenceLabel,
    CopyFirstSequenceChild,
    InsertSequenceEntry,
    GetEmptyString,
    DestroyXmlNode,
    ClearTree,
    CleanupClump,
    AllocateTreeNode,
};

class IArenaSequenceRuntime {
public:
    virtual ~IArenaSequenceRuntime() = default;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t size) = 0;
    [[nodiscard]] virtual std::uint32_t Free(std::uint32_t pointer) = 0;
    [[nodiscard]] virtual std::uint32_t Call(
        ArenaSequenceCall operation,
        std::initializer_list<std::uint32_t> arguments) = 0;
};

class ArenaSequenceRuntimeStub final : public IArenaSequenceRuntime {
public:
    [[nodiscard]] std::uint8_t ReadByte(std::uint32_t) override { return 0u; }
    void WriteByte(std::uint32_t, std::uint8_t) override {}
    [[nodiscard]] std::uint32_t ReadWord(std::uint32_t) override { return 0u; }
    void WriteWord(std::uint32_t, std::uint32_t) override {}
    [[nodiscard]] std::uint32_t Allocate(std::uint32_t) override { return 0u; }
    [[nodiscard]] std::uint32_t Free(std::uint32_t) override { return 0u; }
    [[nodiscard]] std::uint32_t Call(
        ArenaSequenceCall,
        std::initializer_list<std::uint32_t>) override { return 0u; }
};

class ArenaSequenceResources final {
public:
    static constexpr std::uint32_t kObjectSize = 52u;

    // IDA 0x51ef90: construct an uppercase lookup key from a text record.
    [[nodiscard]] static std::uint32_t UppercaseKey(
        std::uint32_t destination, std::uint32_t source,
        IArenaSequenceRuntime& runtime);
    // IDA 0x51f0b0: initialize the embedded collection and shared tree nil.
    [[nodiscard]] static std::uint32_t Initialize(
        std::uint32_t object, std::uint32_t& shared_nil,
        std::uint32_t& shared_references, IArenaSequenceRuntime& runtime);
    // IDA 0x51f160: release text, resource array, tree and shared nil.
    [[nodiscard]] static std::uint32_t Cleanup(
        std::uint32_t object, std::uint32_t& shared_nil,
        std::uint32_t& shared_references, IArenaSequenceRuntime& runtime);
    // IDA 0x51f260: update every resource with float-converted frame time.
    static void Update(std::uint32_t object, double seconds,
                       IArenaSequenceRuntime& runtime);
    // IDA 0x51f2a0: render every resource for one pass.
    static void Render(std::uint32_t object, std::uint32_t pass,
                       IArenaSequenceRuntime& runtime);
    // IDA 0x51f2d0: load a named clump from sequence attributes.
    [[nodiscard]] static std::uint32_t LoadClump(
        std::uint32_t object, std::uint32_t name, std::uint32_t enabled_gate,
        std::uint32_t temporary, IArenaSequenceRuntime& runtime);
    // IDA 0x51f5c0: reset and load sequence.xml into the lookup tree.
    [[nodiscard]] static std::uint32_t LoadSequenceXml(
        std::uint32_t object, std::uint32_t source,
        std::uint32_t document, std::uint32_t temporary,
        std::uint32_t shared_nil,
        IArenaSequenceRuntime& runtime);
    // IDA 0x51f9b0: release a reference-counted 16-byte text record.
    static void ReleaseText(std::uint32_t record, IArenaSequenceRuntime& runtime);
    // IDA 0x51f9f0: apply all ordered sequence entries to the live scene.
    static void ApplySequenceEntries(
        std::uint32_t object, std::uint32_t gate_bits,
        std::uint32_t shared_nil, std::uint32_t temporary,
        IArenaSequenceRuntime& runtime);
    // IDA 0x51fb00: clear XML tree, loaded clumps and active path.
    static void Reset(std::uint32_t object, std::uint32_t shared_nil,
                      IArenaSequenceRuntime& runtime);
    // IDA 0x51fc80: construct a text-record substring.
    [[nodiscard]] static std::uint32_t Substring(
        std::uint32_t destination, std::uint32_t source,
        std::uint32_t start, std::uint32_t count,
        IArenaSequenceRuntime& runtime);
    // IDA 0x51fe10: reverse-find any byte from a character set.
    [[nodiscard]] static std::int32_t FindLastOf(
        std::uint32_t record, std::string_view characters,
        std::uint32_t position, IArenaSequenceRuntime& runtime);
};

class ArenaSequenceTree final {
public:
    // IDA 0x51fe80: insert a 20-byte sequence entry into the ordered tree.
    static void Insert(std::uint32_t tree, std::uint32_t result,
                       std::uint32_t entry, std::uint32_t nil,
                       IArenaSequenceRuntime& runtime);
    // IDA 0x5200e0: erase an iterator range, including the whole-tree path.
    static void EraseRange(std::uint32_t tree, std::uint32_t result,
                           std::uint32_t first, std::uint32_t last,
                           std::uint32_t nil, IArenaSequenceRuntime& runtime,
                           std::uint32_t color_offset = 32u,
                           bool release_text_record = true);
    // IDA 0x5201c0: return the matching node or the tree head.
    static void FindExact(std::uint32_t tree, std::uint32_t result,
                          std::uint32_t key, std::uint32_t nil,
                          IArenaSequenceRuntime& runtime);
    // IDA 0x520270: advance a node iterator in tree order.
    static void Next(std::uint32_t iterator, std::uint32_t nil,
                     IArenaSequenceRuntime& runtime);
    // IDA 0x5202c0: unlink, rebalance and free one 36-byte tree node.
    static void EraseNode(std::uint32_t tree, std::uint32_t result,
                          std::uint32_t node, std::uint32_t nil,
                          IArenaSequenceRuntime& runtime,
                          std::uint32_t color_offset = 32u,
                          bool release_text_record = true);
    // IDA 0x5206d0: release a tree branch right-first along its left spine.
    static std::uint32_t DeleteSubtree(std::uint32_t node,
                                       std::uint32_t nil,
                                       IArenaSequenceRuntime& runtime,
                                       bool release_text_record = true);
    // IDA 0x520720: allocate and insert a node at a known tree position.
    static void InsertAt(std::uint32_t tree, std::uint32_t result,
                         std::uint32_t cursor, std::uint32_t parent,
                         std::uint32_t entry, std::uint32_t nil,
                         IArenaSequenceRuntime& runtime);
    // IDA 0x520990: find the leftmost node beneath a child link.
    [[nodiscard]] static std::uint32_t MinimumFromLink(
        std::uint32_t link, std::uint32_t nil,
        IArenaSequenceRuntime& runtime);
    // IDA 0x5209b0: advance an iterator and return its original node.
    static void NextAndCapture(std::uint32_t iterator,
                               std::uint32_t original,
                               std::uint32_t nil,
                               IArenaSequenceRuntime& runtime);
    // IDA 0x520a10/0x520a70: rotate around a node without moving payloads.
    [[nodiscard]] static std::uint32_t RotateRight(
        std::uint32_t tree, std::uint32_t node, std::uint32_t nil,
        IArenaSequenceRuntime& runtime);
    [[nodiscard]] static std::uint32_t RotateLeft(
        std::uint32_t tree, std::uint32_t node, std::uint32_t nil,
        IArenaSequenceRuntime& runtime);
    // IDA 0x520ad0: move an iterator to its preceding tree node.
    static void Previous(std::uint32_t iterator, std::uint32_t nil,
                         IArenaSequenceRuntime& runtime);
    // IDA 0x520b30: copy the 20-byte key and XML child record.
    static void CopyEntry(std::uint32_t destination, std::uint32_t source,
                          IArenaSequenceRuntime& runtime);
};

}  // namespace pal4
