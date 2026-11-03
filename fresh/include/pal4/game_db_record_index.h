#pragma once

#include <array>
#include <cstdint>
#include <string_view>

namespace pal4 {

class IUtilitiesTextRecordCopyRuntime;

struct GameDbRecordIndexState final {
    std::array<std::uint32_t, 8u> words{};
};
static_assert(sizeof(GameDbRecordIndexState) == 32u);

struct GameDbRecordIndexSharedTree final {
    std::uint32_t nil = 0u;
    std::uint32_t references = 0u;
};

class IGameDbRecordIndexMemory {
public:
    virtual ~IGameDbRecordIndexMemory() = default;
    [[nodiscard]] virtual std::uint32_t Allocate(
        std::uint32_t bytes) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t offset,
                           std::uint32_t value) = 0;
    virtual void Free(std::uint32_t address) = 0;
};

class IGameDbRecordIndexCleanupRuntime {
public:
    virtual ~IGameDbRecordIndexCleanupRuntime() = default;
    virtual void ClearRecords(GameDbRecordIndexState& state) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t FirstElement(
        std::uint32_t head) = 0;
    virtual void DestroySubtree(std::uint32_t root) = 0;
    [[nodiscard]] virtual std::uint32_t NextNode(
        std::uint32_t node) = 0;
    virtual void EraseNode(std::uint32_t node) = 0;
    virtual void ResetRecord(std::uint32_t address,
                             std::uint32_t flag) = 0;
    [[nodiscard]] virtual int Free(std::uint32_t address) = 0;
};

class IGameDbRecordIndexCallbackRuntime {
public:
    virtual ~IGameDbRecordIndexCallbackRuntime() = default;
    [[nodiscard]] virtual std::uint32_t Invoke(
        std::uint32_t callback, std::int32_t argument,
        std::uint32_t index_address) = 0;
};

class IGameDbRecordIndexClearRuntime {
public:
    virtual ~IGameDbRecordIndexClearRuntime() = default;
    [[nodiscard]] virtual std::uint32_t RecordObjectAt(
        std::uint32_t record) = 0;
    virtual void ResetRecordObject(std::uint32_t object,
                                   std::uint32_t flag) = 0;
    virtual void ReleaseRecordText(std::uint32_t record) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    virtual void DestroyRightSubtree(std::uint32_t root) = 0;
    virtual void CleanupNodePayload(std::uint32_t payload,
                                    std::uint32_t flag) = 0;
    virtual void FreeNode(std::uint32_t node) = 0;
    [[nodiscard]] virtual std::uint32_t NextNode(
        std::uint32_t node) = 0;
    virtual void EraseNode(std::uint32_t node) = 0;
};

class IGameDbRecordIndexRegisterRuntime {
public:
    virtual ~IGameDbRecordIndexRegisterRuntime() = default;
    [[nodiscard]] virtual bool NameEquals(
        std::uint32_t record, std::string_view name) = 0;
    virtual void InsertCopy(GameDbRecordIndexState& state,
                            std::uint32_t position,
                            std::string_view name,
                            std::uint32_t object) = 0;
};

class IGameDbRecordIndexLookupRuntime {
public:
    virtual ~IGameDbRecordIndexLookupRuntime() = default;
    [[nodiscard]] virtual std::string_view RecordName(
        std::uint32_t record) = 0;
    [[nodiscard]] virtual std::uint32_t RecordObject(
        std::uint32_t record) = 0;
    [[nodiscard]] virtual std::uint32_t RecordFactory(
        std::uint32_t record) = 0;
    [[nodiscard]] virtual std::uint32_t InvokeFactory(
        std::uint32_t factory, const char* name) = 0;
    virtual void StoreRecordObject(std::uint32_t record,
                                   std::uint32_t object) = 0;
};

class IGameDbRecordIndexEraseRuntime {
public:
    virtual ~IGameDbRecordIndexEraseRuntime() = default;
    [[nodiscard]] virtual std::string_view RecordName(
        std::uint32_t record) = 0;
    [[nodiscard]] virtual std::uint32_t RecordObject(
        std::uint32_t record) = 0;
    virtual void ResetRecordObject(std::uint32_t object,
                                   std::uint32_t flag) = 0;
    virtual void AssignRecord(std::uint32_t destination,
                              std::uint32_t source) = 0;
    virtual void ReleaseRecordText(std::uint32_t record) = 0;
};

class IGameDbRecordArrayRuntime {
public:
    virtual ~IGameDbRecordArrayRuntime() = default;
    [[nodiscard]] virtual std::uint32_t Allocate(
        std::uint32_t bytes) = 0;
    virtual void ConstructCopy(std::uint32_t destination,
                               std::uint32_t source) = 0;
    virtual void AssignCopy(std::uint32_t destination,
                            std::uint32_t source) = 0;
    virtual void ReleaseRecordText(std::uint32_t record) = 0;
    virtual void Free(std::uint32_t allocation) = 0;
};

class GameDbRecordIndex final {
public:
    // IDA 0x64fb80: initialize a 32-byte owner and shared 80-byte tree nodes.
    [[nodiscard]] static GameDbRecordIndexState& Initialize(
        GameDbRecordIndexState& state,
        GameDbRecordIndexSharedTree& shared,
        IGameDbRecordIndexMemory& memory);
    // IDA 0x64fc20: clear records, tree, shared nil and array storage.
    [[nodiscard]] static int Cleanup(
        GameDbRecordIndexState& state,
        GameDbRecordIndexSharedTree& shared,
        IGameDbRecordIndexCleanupRuntime& runtime);
    // IDA 0x64fd70: release only the embedded 80-byte-node tree.
    [[nodiscard]] static int CleanupTree(
        GameDbRecordIndexState& state,
        GameDbRecordIndexSharedTree& shared,
        IGameDbRecordIndexCleanupRuntime& runtime);
    // IDA 0x64fe30: forward to an optional virtual callback.
    [[nodiscard]] static std::uint8_t InvokeLoader(
        std::uint32_t index_address, std::int32_t argument,
        std::uint32_t callback,
        IGameDbRecordIndexCallbackRuntime& runtime);
    // IDA 0x64fe50: clear 24-byte records and the 80-byte-node tree.
    [[nodiscard]] static std::uint32_t ClearRecords(
        GameDbRecordIndexState& state,
        const GameDbRecordIndexSharedTree& shared,
        IGameDbRecordIndexClearRuntime& runtime);
    // IDA 0x64ff80: append a non-null named object when no name matches.
    [[nodiscard]] static bool RegisterNamedRecord(
        GameDbRecordIndexState& state, const char* name,
        std::uint32_t object,
        IGameDbRecordIndexRegisterRuntime& runtime);
    // IDA 0x650100: find the first byte-exact named record.
    [[nodiscard]] static std::uint32_t FindNamedRecord(
        const GameDbRecordIndexState& state, const char* name,
        IGameDbRecordIndexLookupRuntime& runtime);
    // IDA 0x650180: create a matched record's object on first access.
    [[nodiscard]] static std::uint32_t FindOrLoadNamedRecord(
        const GameDbRecordIndexState& state, const char* name,
        IGameDbRecordIndexLookupRuntime& runtime);
    // IDA 0x650210: remove the first matching 24-byte record.
    static void RemoveNamedRecord(GameDbRecordIndexState& state,
                                  const char* name,
                                  IGameDbRecordIndexEraseRuntime& runtime);
    // IDA 0x6502f0: return the first record and reset the supplied cursor.
    [[nodiscard]] static std::uint32_t FirstRecord(
        const GameDbRecordIndexState& state, std::uint32_t& cursor);
    // IDA 0x650330: advance a 24-byte record cursor.
    [[nodiscard]] static std::uint32_t NextRecord(
        const GameDbRecordIndexState& state, std::uint32_t& cursor);
    // IDA 0x650e80: insert copies into a 24-byte record array.
    static void InsertRecordCopies(GameDbRecordIndexState& state,
                                   std::uint32_t position,
                                   std::uint32_t count,
                                   std::uint32_t value,
                                   IGameDbRecordArrayRuntime& runtime);
    // IDA 0x651980: copy-construct a 24-byte named GameDB record.
    static void CopyRecord24(std::uint32_t destination,
                             std::uint32_t source,
                             IUtilitiesTextRecordCopyRuntime& runtime);
};

}  // namespace pal4
