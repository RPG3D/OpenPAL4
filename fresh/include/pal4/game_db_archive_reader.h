#pragma once

#include "pal4/archive_text_reader.h"
#include "pal4/game_db_archive_writer.h"
#include "pal4/effect_system_global_callback.h"
#include "pal4/game_db_typed_value.h"

#include <array>
#include <cstdint>
#include <string_view>

namespace pal4 {

class IGameDbArchiveReadRuntime : public IArchiveStreamRuntime,
                                  public IArchiveTextReadRuntime {
public:
    virtual ~IGameDbArchiveReadRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ConstructProperty(
        const char* name) = 0;
    [[nodiscard]] virtual const char* TextData(
        const StringBufferState& text) = 0;
    virtual void SetPropertyLabel(std::uint32_t property,
                                  const char* label) = 0;
    virtual void AddEntry(std::uint32_t property, std::int32_t id,
                          const char* name) = 0;
    virtual void InsertProperty(std::uint32_t owner,
                                std::uint32_t property) = 0;
    virtual void ReleaseText(StringBufferState& text) = 0;
    virtual void DestroyProperty(std::uint32_t property) = 0;
};

class IGameDbArchiveTableRuntime : public IGameDbArchiveReadRuntime {
public:
    ~IGameDbArchiveTableRuntime() override = default;
    virtual void LogBegin(const char* name) = 0;
    virtual void LogEnd(const char* name) = 0;
    [[nodiscard]] virtual std::uint32_t FindOrLoad(
        std::uint32_t index, const char* name) = 0;
    virtual void RegisterObject(std::uint32_t index,
                                const char* name) = 0;
    [[nodiscard]] virtual bool NamesDiffer(const char* actual,
                                           const char* expected) = 0;
    virtual void SetDescription(std::uint32_t object,
                                const char* description) = 0;
    virtual void ReserveSchema(std::uint32_t count) = 0;
    virtual void AddSchemaField(const char* name, std::uint32_t type,
                                const char* label,
                                const char* detail) = 0;
    virtual void AssignSchema(std::uint32_t object,
                              std::uint32_t attribute,
                              std::uint32_t row_count) = 0;
    virtual void PrepareObject(std::uint32_t object) = 0;
    virtual void InitializeRows(std::uint32_t field_count) = 0;
    virtual void AddDefaultValue() = 0;
    virtual void ResetValue() = 0;
    virtual void SetInteger(std::uint32_t value) = 0;
    virtual void SetText(const char* value) = 0;
    virtual void SetFloat(float value) = 0;
    virtual void CommitValue(std::uint32_t field) = 0;
    virtual void CommitRow(std::uint32_t object) = 0;
    virtual void FinalizeObject(std::uint32_t object) = 0;
    virtual void CleanupRows() = 0;
    virtual void CleanupSchema() = 0;
};

class GameDbArchiveReader final {
public:
    // IDA 0x651f40: read a marked property record and insert its entries.
    [[nodiscard]] static bool ReadPropertyRecord(
        const char* name, std::uint32_t owner,
        const ArchiveStreamAdapterState& stream,
        IGameDbArchiveReadRuntime& runtime);
    // IDA 0x6523e0: read a named object's schema and typed rows.
    [[nodiscard]] static bool ReadTable(
        const char* expected_name, std::uint32_t index,
        const ArchiveStreamAdapterState& stream,
        IGameDbArchiveTableRuntime& runtime);
};

struct GameDbSchemaArrayState final {
    std::array<std::uint32_t, 4u> words{};
};
static_assert(sizeof(GameDbSchemaArrayState) == 16u);

class IGameDbSchemaArrayRuntime {
public:
    virtual ~IGameDbSchemaArrayRuntime() = default;
    [[nodiscard]] virtual std::uint32_t Allocate(
        std::uint32_t bytes) = 0;
    virtual void ConstructCopy(std::uint32_t destination,
                               std::uint32_t source) = 0;
    virtual void AssignCopy(std::uint32_t destination,
                            std::uint32_t source) = 0;
    virtual void Release(std::uint32_t element) = 0;
    virtual void Free(std::uint32_t address) = 0;
};

class IGameDbSchemaElementRuntime {
public:
    virtual ~IGameDbSchemaElementRuntime() = default;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    virtual void AssignText(std::uint32_t destination,
                            std::uint32_t source) = 0;
};

class GameDbSchemaElement final {
public:
    // IDA 0x6535c0: copy-construct one 60-byte schema entry.
    static void ConstructCopy(std::uint32_t destination,
                              std::uint32_t source,
                              IGameDbSchemaElementRuntime& runtime);
    // IDA 0x656a80: assign three text fields, kind and trailing scalars.
    [[nodiscard]] static std::uint32_t AssignCopy(
        std::uint32_t destination, std::uint32_t source,
        IGameDbSchemaElementRuntime& runtime);
};

class GameDbSchemaArray final {
public:
    // IDA 0x652e20: count 60-byte entries between begin and end.
    [[nodiscard]] static std::uint32_t Count(
        const GameDbSchemaArrayState& state);
    // IDA 0x652e50: count 60-byte slots between begin and capacity.
    [[nodiscard]] static std::uint32_t Capacity(
        const GameDbSchemaArrayState& state);
    // IDA 0x652e80: insert a copy at a 60-byte-element position.
    [[nodiscard]] static std::uint32_t InsertCopy(
        GameDbSchemaArrayState& state, std::uint32_t position,
        std::uint32_t value, IGameDbSchemaArrayRuntime& runtime);
    // IDA 0x653070: destroy the half-open element range.
    static void FreeElements(std::uint32_t begin, std::uint32_t end,
                             IGameDbSchemaArrayRuntime& runtime);
    // IDA 0x6530a0: copy-construct a half-open element range.
    [[nodiscard]] static std::uint32_t CopyElements(
        std::uint32_t begin, std::uint32_t end,
        std::uint32_t destination, IGameDbSchemaArrayRuntime& runtime);
    // IDA 0x653140: construct repeated copies of one 60-byte value.
    static void ConstructRepeated(std::uint32_t destination,
                                  std::uint32_t count, std::uint32_t value,
                                  IGameDbSchemaArrayRuntime& runtime);
    // IDA 0x653170: assign one value to a half-open range.
    [[nodiscard]] static std::uint32_t AssignElements(
        std::uint32_t begin, std::uint32_t end, std::uint32_t value,
        IGameDbSchemaArrayRuntime& runtime);
    // IDA 0x653390: assign a range backward into a destination ending address.
    [[nodiscard]] static std::uint32_t MoveElementsBackward(
        std::uint32_t begin, std::uint32_t end,
        std::uint32_t destination_end, IGameDbSchemaArrayRuntime& runtime);
    // IDA 0x655650: assign matching 60-byte elements in forward order.
    [[nodiscard]] static std::uint32_t AssignPairwise(
        std::uint32_t begin, std::uint32_t end,
        std::uint32_t destination, IGameDbSchemaArrayRuntime& runtime);
};

struct GameDbSchemaOwnerState final {
    GameDbSchemaArrayState schemas;
    std::array<std::uint32_t, 4u> pointers{};
};
static_assert(sizeof(GameDbSchemaOwnerState) == 32u);

class IGameDbSchemaOwnerRuntime : public IGameDbSchemaArrayRuntime {
public:
    ~IGameDbSchemaOwnerRuntime() override = default;
    virtual void CopyPointers(std::uint32_t begin, std::uint32_t end,
                              std::uint32_t destination) = 0;
    virtual void AppendNullPointer(GameDbSchemaOwnerState& owner) = 0;
};

class IGameDbSchemaBindingRuntime : public IEffectSystemGlobalCallbackRuntime {
public:
    ~IGameDbSchemaBindingRuntime() override = default;
    [[nodiscard]] virtual std::uint32_t ReadPointer(
        std::uint32_t address) = 0;
    virtual void WritePointer(std::uint32_t address,
                              std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t AllocatePlaceholder(
        std::uint32_t bytes) = 0;
    virtual void WriteVtable(std::uint32_t object,
                             std::uint32_t vtable) = 0;
    virtual void InitializePlaceholderText(std::uint32_t field) = 0;
    [[nodiscard]] virtual std::string_view SchemaName(
        std::uint32_t schema_element) = 0;
    virtual void SetExceptionMessage(std::string_view message) = 0;
};

class IGameDbSchemaProcessRuntime {
public:
    virtual ~IGameDbSchemaProcessRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadPointer(
        std::uint32_t address) = 0;
    virtual void InvokeFirstVirtual(std::uint32_t object,
                                    std::uint32_t flag) = 0;
};

class IGameDbSchemaBindRuntime : public IEffectSystemGlobalCallbackRuntime {
public:
    ~IGameDbSchemaBindRuntime() override = default;
    [[nodiscard]] virtual std::string_view SchemaName(
        std::uint32_t element) = 0;
    [[nodiscard]] virtual std::uint32_t SchemaKind(
        std::uint32_t element) = 0;
    [[nodiscard]] virtual std::string_view SourceName(
        std::uint32_t source) = 0;
    [[nodiscard]] virtual std::uint32_t ReadPointer(
        std::uint32_t slot) = 0;
    virtual void WritePointer(std::uint32_t slot,
                              std::uint32_t object) = 0;
    virtual void DeleteObject(std::uint32_t object,
                              std::uint32_t flag) = 0;
    [[nodiscard]] virtual std::uint32_t GlobalCallbackSlot() = 0;
    virtual void SetExceptionMessage(std::string_view message) = 0;
    [[nodiscard]] virtual std::uint32_t VirtualKind(
        std::uint32_t object) = 0;
    [[nodiscard]] virtual std::uint32_t VirtualSource(
        std::uint32_t object) = 0;
};

class GameDbBindingDiagnostics final {
public:
    // IDA 0x655050/0x655190/0x6552d0: emit field binding messages.
    static void ReportMissing(std::uint32_t source,
                              IGameDbSchemaBindRuntime& runtime);
    static void ReportRebind(std::uint32_t source,
                             IGameDbSchemaBindRuntime& runtime);
    static void ReportTypeMismatch(std::uint32_t source,
                                   IGameDbSchemaBindRuntime& runtime);
private:
    static void Report(std::string_view prefix,
                       std::uint32_t source,
                       IGameDbSchemaBindRuntime& runtime);
};

class GameDbSchemaOwner final {
public:
    // IDA 0x654690: assign schema entries, then append null pointer slots.
    [[nodiscard]] static bool AssignFrom(
        GameDbSchemaOwnerState& destination,
        const GameDbSchemaOwnerState& source,
        IGameDbSchemaOwnerRuntime& runtime);
    // IDA 0x6548e0: fill null binding slots and report unbound fields.
    static void BindMissing(GameDbSchemaOwnerState& owner,
                            std::uint32_t& temporary_offset,
                            std::uint32_t callback_slot,
                            IGameDbSchemaBindingRuntime& runtime);
    // IDA 0x654b30: invoke virtual slot zero on each non-null binding.
    [[nodiscard]] static bool ProcessBindings(
        const GameDbSchemaOwnerState& owner,
        IGameDbSchemaProcessRuntime& runtime);
    // IDA 0x654ee0: query an object's virtual kind/name before binding.
    [[nodiscard]] static bool BindFromObject(
        GameDbSchemaOwnerState& owner, std::uint32_t object,
        IGameDbSchemaBindRuntime& runtime);
    // IDA 0x654f10: bind by exact schema name and matching field kind.
    [[nodiscard]] static bool BindEffect(
        GameDbSchemaOwnerState& owner, std::uint32_t source,
        std::uint32_t object, std::uint32_t kind,
        IGameDbSchemaBindRuntime& runtime);
};

struct GameDbPlaceholderEffectState final {
    std::uint32_t vtable = 0u;
    std::uint32_t flag = 0u;
    std::uint32_t data = 0u;
    std::uint32_t length = 0u;
    std::uint32_t capacity = 0u;
};
static_assert(sizeof(GameDbPlaceholderEffectState) == 20u);

class IGameDbPlaceholderEffectRuntime {
public:
    virtual ~IGameDbPlaceholderEffectRuntime() = default;
    virtual void ReleaseText(std::uint32_t data) = 0;
    virtual void Free(std::uint32_t object) = 0;
};

class GameDbPlaceholderEffect final {
public:
    // IDA 0x654ac0: deleting destructor for 20-byte placeholder.
    [[nodiscard]] static std::uint32_t Delete(
        std::uint32_t address, GameDbPlaceholderEffectState& state,
        std::uint8_t flags, std::uint32_t& temporary_offset,
        IGameDbPlaceholderEffectRuntime& runtime);
    // IDA 0x654ae0: release text and restore base vtable.
    static void Cleanup(GameDbPlaceholderEffectState& state,
                        std::uint32_t& temporary_offset,
                        IGameDbPlaceholderEffectRuntime& runtime);
};

struct GameDbBoundEffectState final {
    std::uint32_t vtable = 0u;
    std::uint32_t value = 0u;
    std::uint32_t text_flag = 0u;
    std::uint32_t text_data = 0u;
    std::uint32_t text_length = 0u;
    std::uint32_t text_capacity = 0u;
};
static_assert(sizeof(GameDbBoundEffectState) == 24u);

class IGameDbBoundEffectRuntime {
public:
    virtual ~IGameDbBoundEffectRuntime() = default;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    [[nodiscard]] virtual GameDbBoundEffectState& Object(
        std::uint32_t address) = 0;
    virtual void AssignText(GameDbBoundEffectState& object,
                            std::uint32_t source) = 0;
    [[nodiscard]] virtual bool Bind(std::uint32_t owner,
                                    std::uint32_t source,
                                    std::uint32_t object,
                                    std::uint32_t kind) = 0;
};

class GameDbBoundEffectFactory final {
public:
    // IDA 0x654b60/0x654d20: construct two bound effect variants.
    [[nodiscard]] static bool CreateKindOne(
        std::uint32_t owner, std::uint32_t source,
        std::uint32_t value, std::uint32_t& temporary_offset,
        IGameDbBoundEffectRuntime& runtime);
    [[nodiscard]] static bool CreateKindTwo(
        std::uint32_t owner, std::uint32_t source,
        std::uint32_t value, std::uint32_t& temporary_offset,
        IGameDbBoundEffectRuntime& runtime);
private:
    [[nodiscard]] static bool Create(
        std::uint32_t owner, std::uint32_t source,
        std::uint32_t value, std::uint32_t kind,
        std::uint32_t vtable, std::uint32_t& temporary_offset,
        IGameDbBoundEffectRuntime& runtime);
};

class IGameDbBoundEffectPropertyRuntime : public IGameDbTypedValueErrorRuntime {
public:
    ~IGameDbBoundEffectPropertyRuntime() override = default;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint8_t AssignText(
        std::uint32_t destination, std::uint32_t source) = 0;
};

class GameDbBoundEffect final {
public:
    // IDA 0x655410: write tagged integer to record at object offset.
    [[nodiscard]] static std::int32_t ApplyInteger(
        const GameDbBoundEffectState& effect,
        std::uint32_t record, const GameDbTypedValueState& value,
        IGameDbBoundEffectPropertyRuntime& runtime);
    // IDA 0x655450/0x655600: cleanup both derived effect variants.
    static void CleanupKindOne(GameDbBoundEffectState& effect,
                               std::uint32_t& temporary_offset,
                               IGameDbBoundEffectPropertyRuntime& runtime);
    static void CleanupKindTwo(GameDbBoundEffectState& effect,
                               std::uint32_t& temporary_offset,
                               IGameDbBoundEffectPropertyRuntime& runtime);
    // IDA 0x6554a0: assign tagged string to record at object offset.
    [[nodiscard]] static std::uint8_t ApplyString(
        const GameDbBoundEffectState& effect,
        std::uint32_t record, const GameDbTypedValueState& value,
        GameDbTypedValueErrorState& fallback,
        std::uint8_t source_high_byte,
        IGameDbBoundEffectPropertyRuntime& runtime);
private:
    static void Cleanup(GameDbBoundEffectState& effect,
                        std::uint32_t vtable,
                        std::uint32_t& temporary_offset,
                        IGameDbBoundEffectPropertyRuntime& runtime);
};

class IGameDbObjectFactoryRuntime {
public:
    virtual ~IGameDbObjectFactoryRuntime() = default;
    [[nodiscard]] virtual std::uint32_t Allocate(
        std::uint32_t bytes) = 0;
    [[nodiscard]] virtual std::uint32_t Construct(
        std::uint32_t object, const char* name) = 0;
};

class GameDbObjectFactory final {
public:
    // IDA 0x6530e0: allocate and construct a 92-byte named object.
    [[nodiscard]] static std::uint32_t Create(
        const char* name, IGameDbObjectFactoryRuntime& runtime);
};

struct GameDbPropertyState final {
    std::array<std::uint32_t, 12u> words{};
};
static_assert(sizeof(GameDbPropertyState) == 48u);

class IGameDbPropertyRuntime {
public:
    virtual ~IGameDbPropertyRuntime() = default;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    [[nodiscard]] virtual std::uint32_t AllocateText(
        std::uint32_t length, std::uint8_t mode) = 0;
    virtual void CopyText(std::uint32_t destination, const char* source,
                          std::uint32_t length) = 0;
    virtual void CleanupRecord(std::uint32_t address) = 0;
    virtual void ConstructCopy(std::uint32_t destination,
                               std::uint32_t source) = 0;
    virtual void AssignCopy(std::uint32_t destination,
                            std::uint32_t source) = 0;
    virtual void ReleaseText(std::uint32_t address) = 0;
    [[nodiscard]] virtual int Free(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::int32_t ReadRecordId(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::string_view ReadRecordText(
        std::uint32_t address) = 0;
    virtual void InsertRecord(GameDbPropertyState& state,
                              std::uint32_t position, std::int32_t id,
                              std::uint32_t text, std::uint32_t length) = 0;
};

class GameDbProperty final {
public:
    // IDA 0x653700: construct 48-byte property and reserve eight records.
    [[nodiscard]] static GameDbPropertyState& Construct(
        GameDbPropertyState& state, const char* name,
        IGameDbPropertyRuntime& runtime);
    // IDA 0x653880: release records, both text fields, and array storage.
    [[nodiscard]] static int Destroy(GameDbPropertyState& state,
                                     IGameDbPropertyRuntime& runtime);
    // IDA 0x653960: clean live 24-byte records and reset array end.
    static void ClearRecords(GameDbPropertyState& state,
                             IGameDbPropertyRuntime& runtime);
    // IDA 0x6539c0: insert one ID/name pair if both keys are unique.
    [[nodiscard]] static bool AddEntry(GameDbPropertyState& state,
                                       std::int32_t id, const char* name,
                                       IGameDbPropertyRuntime& runtime);
    // IDA 0x653b30: return the ID of an exactly matching record name.
    [[nodiscard]] static std::int32_t FindEntryByName(
        const GameDbPropertyState& state, const char* name,
        IGameDbPropertyRuntime& runtime);
    // IDA 0x653bd0/0x653bf0: find a record by signed ID or its text field.
    [[nodiscard]] static std::uint32_t FindEntryById(
        const GameDbPropertyState& state, std::int32_t id,
        IGameDbPropertyRuntime& runtime);
    [[nodiscard]] static std::uint32_t FindEntryTextById(
        const GameDbPropertyState& state, std::int32_t id,
        IGameDbPropertyRuntime& runtime);
    // IDA 0x653c60: get a record at a zero-based position.
    [[nodiscard]] static std::uint32_t EntryAt(
        const GameDbPropertyState& state, std::uint32_t index);
    // IDA 0x653ca0: insert repeated 24-byte records in place or by growth.
    [[nodiscard]] static std::uint32_t InsertCopies(
        GameDbPropertyState& state, std::uint32_t position,
        std::uint32_t count, std::uint32_t source,
        IGameDbPropertyRuntime& runtime);
    // IDA 0x653fc0: cleanup a record and optionally release its storage.
    [[nodiscard]] static std::uint32_t DeleteRecord(
        std::uint32_t record, std::uint8_t flags,
        IGameDbPropertyRuntime& runtime);
private:
    [[nodiscard]] static std::uint32_t LowerBound(
        const GameDbPropertyState& state, std::int32_t id,
        IGameDbPropertyRuntime& runtime);
};

}  // namespace pal4
