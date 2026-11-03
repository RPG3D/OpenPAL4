#pragma once

#include <cstdint>

namespace pal4 {

class IArenaSequenceRuntime;

enum class EfEffectSystemTreeKind : std::uint8_t {
    First,
    AuxiliaryStorage,
    KeyStorage,
    ArenaSequence,
    EffectObject,
    NameCount,
    Primary,
    Secondary,
    AuxiliaryFirst,
    AuxiliarySecond,
};

class IEfEffectSystemTreeLifecycleRuntime {
public:
    virtual ~IEfEffectSystemTreeLifecycleRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    [[nodiscard]] virtual std::uint32_t Free(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t SharedNil(
        EfEffectSystemTreeKind kind) = 0;
    virtual void SetSharedNil(EfEffectSystemTreeKind kind,
                              std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t SharedNilReferences(
        EfEffectSystemTreeKind kind) = 0;
    virtual void SetSharedNilReferences(EfEffectSystemTreeKind kind,
                                        std::uint32_t value) = 0;
    virtual void DeleteSubtree(EfEffectSystemTreeKind kind,
                               std::uint32_t root) = 0;
    [[nodiscard]] virtual std::uint32_t Next(
        EfEffectSystemTreeKind kind, std::uint32_t node) = 0;
    virtual void EraseNode(EfEffectSystemTreeKind kind,
                           std::uint32_t tree, std::uint32_t node) = 0;
    virtual void ReadFirstElement(std::uint32_t tree) = 0;
};

class IEfEffectSystemInstanceTreeRuntime
    : public IEfEffectSystemTreeLifecycleRuntime {
public:
    virtual void CleanupInstances(std::uint32_t tree) = 0;
};

enum class EfEffectSystemTreeWrapperKind : std::uint8_t {
    AuxiliaryFirst,
    Instances,
    Active,
    Secondary,
};

class IEfEffectSystemTreeWrapperRuntime {
public:
    virtual ~IEfEffectSystemTreeWrapperRuntime() = default;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    virtual void InitializeStorage(EfEffectSystemTreeWrapperKind kind,
                                   std::uint32_t tree) = 0;
};

enum class EfEffectSystemRangeKind : std::uint8_t {
    EffectObject,
    Key,
    Primary,
    Secondary,
    AuxiliaryFirst,
    AuxiliarySecond,
};

class IEfEffectSystemRangeRuntime {
public:
    virtual ~IEfEffectSystemRangeRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    virtual void DeleteSubtree(EfEffectSystemRangeKind kind,
                               std::uint32_t root) = 0;
    virtual void ReleaseText(std::uint32_t record) = 0;
    [[nodiscard]] virtual std::uint32_t Free(std::uint32_t node) = 0;
    [[nodiscard]] virtual std::uint32_t Next(
        EfEffectSystemRangeKind kind, std::uint32_t node) = 0;
    virtual void EraseNode(EfEffectSystemRangeKind kind,
                           std::uint32_t tree, std::uint32_t node) = 0;
};

class EfEffectSystemTreeLifecycle final {
public:
    // IDA 0x6367e0, 0x636930, 0x636a80: construct three distinct trees.
    [[nodiscard]] static std::uint32_t InitializeFirst(
        std::uint32_t tree, std::uint8_t first, std::uint8_t second,
        IEfEffectSystemTreeLifecycleRuntime& runtime);
    [[nodiscard]] static std::uint32_t InitializeEffectObject(
        std::uint32_t tree, std::uint8_t first, std::uint8_t second,
        IEfEffectSystemTreeLifecycleRuntime& runtime);
    [[nodiscard]] static std::uint32_t InitializeNameCount(
        std::uint32_t tree, std::uint8_t first, std::uint8_t second,
        IEfEffectSystemTreeLifecycleRuntime& runtime);
    // IDA 0x636880, 0x6369d0, 0x636b20: release three tree variants.
    [[nodiscard]] static std::uint32_t CleanupArenaSequence(
        std::uint32_t tree, IEfEffectSystemTreeLifecycleRuntime& runtime);
    [[nodiscard]] static std::uint32_t CleanupEffectObject(
        std::uint32_t tree, IEfEffectSystemTreeLifecycleRuntime& runtime);
    [[nodiscard]] static std::uint32_t CleanupNameCount(
        std::uint32_t tree, IEfEffectSystemTreeLifecycleRuntime& runtime);
    // IDA 0x636bd0..0x636fd0: unwind and direct cleanup of +8/+24 trees.
    [[nodiscard]] static std::uint32_t CleanupPrimaryOnUnwind(
        std::uint32_t tree, IEfEffectSystemTreeLifecycleRuntime& runtime);
    [[nodiscard]] static std::uint32_t CleanupSecondaryOnUnwind(
        std::uint32_t tree, IEfEffectSystemTreeLifecycleRuntime& runtime);
    [[nodiscard]] static std::uint32_t CleanupPrimaryDirect(
        std::uint32_t tree, IEfEffectSystemTreeLifecycleRuntime& runtime);
    [[nodiscard]] static std::uint32_t CleanupSecondaryDirect(
        std::uint32_t tree, IEfEffectSystemTreeLifecycleRuntime& runtime);
    // IDA 0x637200/0x6372b0: direct cleanup without first-element read.
    [[nodiscard]] static std::uint32_t CleanupPrimaryPlain(
        std::uint32_t tree, IEfEffectSystemTreeLifecycleRuntime& runtime);
    [[nodiscard]] static std::uint32_t CleanupSecondaryPlain(
        std::uint32_t tree, IEfEffectSystemTreeLifecycleRuntime& runtime);
    // IDA 0x637090..0x6370f0: set flags before specialized storage setup.
    [[nodiscard]] static std::uint32_t InitializeAuxiliaryFirst(
        std::uint32_t tree, std::uint8_t first, std::uint8_t second,
        std::uint8_t mode, IEfEffectSystemTreeWrapperRuntime& runtime);
    [[nodiscard]] static std::uint32_t InitializeInstances(
        std::uint32_t tree, std::uint8_t first, std::uint8_t second,
        std::uint8_t mode, IEfEffectSystemTreeWrapperRuntime& runtime);
    [[nodiscard]] static std::uint32_t InitializeActive(
        std::uint32_t tree, std::uint8_t first, std::uint8_t second,
        std::uint8_t mode, IEfEffectSystemTreeWrapperRuntime& runtime);
    // IDA 0x6376e0: flags and later-address storage for the +24 tree.
    [[nodiscard]] static std::uint32_t InitializeSecondary(
        std::uint32_t tree, std::uint8_t first, std::uint8_t second,
        std::uint8_t mode, IEfEffectSystemTreeWrapperRuntime& runtime);
    // IDA 0x637120/0x637360: half-open ranges over distinct node records.
    [[nodiscard]] static std::uint32_t EraseEffectObjectRange(
        std::uint32_t tree, std::uint32_t result,
        std::uint32_t first, std::uint32_t last,
        std::uint32_t nil, IEfEffectSystemRangeRuntime& runtime);
    [[nodiscard]] static std::uint32_t EraseKeyRange(
        std::uint32_t tree, std::uint32_t result,
        std::uint32_t first, std::uint32_t last,
        std::uint32_t nil, IEfEffectSystemRangeRuntime& runtime);
    // IDA 0x637600/0x637710: ranges for singleton +8/+24 text trees.
    [[nodiscard]] static std::uint32_t ErasePrimaryRange(
        std::uint32_t tree, std::uint32_t result,
        std::uint32_t first, std::uint32_t last,
        std::uint32_t nil, IEfEffectSystemRangeRuntime& runtime);
    [[nodiscard]] static std::uint32_t EraseSecondaryRange(
        std::uint32_t tree, std::uint32_t result,
        std::uint32_t first, std::uint32_t last,
        std::uint32_t nil, IEfEffectSystemRangeRuntime& runtime);
    // IDA 0x638ce0/0x638db0: auxiliary 20-/24-byte node ranges.
    [[nodiscard]] static std::uint32_t EraseAuxiliaryFirstRange(
        std::uint32_t tree, std::uint32_t result,
        std::uint32_t first, std::uint32_t last,
        std::uint32_t nil, IEfEffectSystemRangeRuntime& runtime);
    [[nodiscard]] static std::uint32_t EraseAuxiliarySecondRange(
        std::uint32_t tree, std::uint32_t result,
        std::uint32_t first, std::uint32_t last,
        std::uint32_t nil, IEfEffectSystemRangeRuntime& runtime);
    // IDA 0x637430: advance a key-tree iterator and return EAX separately.
    [[nodiscard]] static std::uint32_t AdvanceKey(
        std::uint32_t& iterator, std::uint32_t nil,
        IEfEffectSystemTreeLifecycleRuntime& runtime);
    // IDA 0x637480..0x637580: acquire shared nil and tree head.
    [[nodiscard]] static std::uint32_t InitializeAuxiliaryStorage(
        std::uint32_t tree, IEfEffectSystemTreeLifecycleRuntime& runtime);
    [[nodiscard]] static std::uint32_t InitializeArenaSequenceStorage(
        std::uint32_t tree, IEfEffectSystemTreeLifecycleRuntime& runtime);
    [[nodiscard]] static std::uint32_t InitializeEffectObjectStorage(
        std::uint32_t tree, IEfEffectSystemTreeLifecycleRuntime& runtime);
    // IDA 0x637c10: shared nil and 20-byte key tree head.
    [[nodiscard]] static std::uint32_t InitializeKeyStorage(
        std::uint32_t tree, IEfEffectSystemTreeLifecycleRuntime& runtime);
    // IDA 0x637f00/0x637f20: leftmost node from a child link.
    [[nodiscard]] static std::uint32_t MinimumKeyFromLink(
        std::uint32_t link, std::uint32_t nil,
        IEfEffectSystemTreeLifecycleRuntime& runtime);
    [[nodiscard]] static std::uint32_t MinimumEffectObjectFromLink(
        std::uint32_t link, std::uint32_t nil,
        IEfEffectSystemTreeLifecycleRuntime& runtime);
    // IDA 0x637f40..0x638960: node, branch, head and cursor paths for
    // the primary and secondary 36-byte text trees.
    [[nodiscard]] static std::uint32_t ErasePrimaryNode(
        std::uint32_t tree, std::uint32_t result,
        std::uint32_t node, std::uint32_t nil,
        IArenaSequenceRuntime& runtime);
    [[nodiscard]] static std::uint32_t DeletePrimarySubtree(
        std::uint32_t root, std::uint32_t nil,
        IArenaSequenceRuntime& runtime);
    [[nodiscard]] static std::uint32_t InitializePrimaryStorage(
        std::uint32_t tree, IEfEffectSystemTreeLifecycleRuntime& runtime);
    [[nodiscard]] static std::uint32_t AdvancePrimaryAndCapture(
        std::uint32_t& iterator, std::uint32_t result,
        std::uint32_t nil, IEfEffectSystemTreeLifecycleRuntime& runtime);
    [[nodiscard]] static std::uint32_t EraseSecondaryNode(
        std::uint32_t tree, std::uint32_t result,
        std::uint32_t node, std::uint32_t nil,
        IArenaSequenceRuntime& runtime);
    [[nodiscard]] static std::uint32_t DeleteSecondarySubtree(
        std::uint32_t root, std::uint32_t nil,
        IArenaSequenceRuntime& runtime);
    [[nodiscard]] static std::uint32_t InitializeSecondaryStorage(
        std::uint32_t tree, IEfEffectSystemTreeLifecycleRuntime& runtime);
    [[nodiscard]] static std::uint32_t AdvanceSecondaryAndCapture(
        std::uint32_t& iterator, std::uint32_t result,
        std::uint32_t nil, IEfEffectSystemTreeLifecycleRuntime& runtime);
    // IDA 0x638a50/0x638a70: leftmost link for two text-tree nils.
    [[nodiscard]] static std::uint32_t MinimumPrimaryFromLink(
        std::uint32_t link, std::uint32_t nil,
        IEfEffectSystemTreeLifecycleRuntime& runtime);
    [[nodiscard]] static std::uint32_t MinimumSecondaryFromLink(
        std::uint32_t link, std::uint32_t nil,
        IEfEffectSystemTreeLifecycleRuntime& runtime);
    // IDA 0x638ac0/0x638b10: successor without captured old cursor.
    [[nodiscard]] static std::uint32_t AdvancePrimary(
        std::uint32_t& iterator, std::uint32_t nil,
        IEfEffectSystemTreeLifecycleRuntime& runtime);
    [[nodiscard]] static std::uint32_t AdvanceSecondary(
        std::uint32_t& iterator, std::uint32_t nil,
        IEfEffectSystemTreeLifecycleRuntime& runtime);
    // IDA 0x638e80..0x6392a0: auxiliary node removal without text teardown.
    [[nodiscard]] static std::uint32_t EraseAuxiliaryFirstNode(
        std::uint32_t tree, std::uint32_t result,
        std::uint32_t node, std::uint32_t nil,
        IArenaSequenceRuntime& runtime);
    [[nodiscard]] static std::uint32_t DeleteAuxiliaryFirstSubtree(
        std::uint32_t root, std::uint32_t nil,
        IArenaSequenceRuntime& runtime);
    [[nodiscard]] static std::uint32_t EraseAuxiliarySecondNode(
        std::uint32_t tree, std::uint32_t result,
        std::uint32_t node, std::uint32_t nil,
        IArenaSequenceRuntime& runtime);
    // IDA 0x639680..0x639860: auxiliary tree destruction and cursors.
    [[nodiscard]] static std::uint32_t DeleteAuxiliarySecondSubtree(
        std::uint32_t root, std::uint32_t nil,
        IArenaSequenceRuntime& runtime);
    [[nodiscard]] static std::uint32_t MinimumAuxiliaryFirstFromLink(
        std::uint32_t link, std::uint32_t nil,
        IEfEffectSystemTreeLifecycleRuntime& runtime);
    [[nodiscard]] static std::uint32_t MinimumAuxiliarySecondFromLink(
        std::uint32_t link, std::uint32_t nil,
        IEfEffectSystemTreeLifecycleRuntime& runtime);
    [[nodiscard]] static std::uint32_t AdvanceAuxiliaryFirst(
        std::uint32_t& iterator, std::uint32_t nil,
        IEfEffectSystemTreeLifecycleRuntime& runtime);
    [[nodiscard]] static std::uint32_t AdvanceAuxiliarySecond(
        std::uint32_t& iterator, std::uint32_t nil,
        IEfEffectSystemTreeLifecycleRuntime& runtime);
    [[nodiscard]] static std::uint32_t CleanupAuxiliaryFirst(
        std::uint32_t tree, IEfEffectSystemTreeLifecycleRuntime& runtime);
    [[nodiscard]] static std::uint32_t CleanupAuxiliarySecond(
        std::uint32_t tree, IEfEffectSystemTreeLifecycleRuntime& runtime);
    // IDA 0x639920/0x639a10: direct ArenaSequence tree teardown.
    [[nodiscard]] static std::uint32_t CleanupArenaSequenceWithInstances(
        std::uint32_t tree, IEfEffectSystemInstanceTreeRuntime& runtime);
    [[nodiscard]] static std::uint32_t CleanupArenaSequenceDirect(
        std::uint32_t tree, IEfEffectSystemTreeLifecycleRuntime& runtime);
    // IDA 0x639de0/0x639ea0: direct object and name-count teardown.
    [[nodiscard]] static std::uint32_t CleanupEffectObjectDirect(
        std::uint32_t tree, IEfEffectSystemTreeLifecycleRuntime& runtime);
    [[nodiscard]] static std::uint32_t CleanupNameCountDirect(
        std::uint32_t tree, IEfEffectSystemTreeLifecycleRuntime& runtime);
};

}  // namespace pal4
