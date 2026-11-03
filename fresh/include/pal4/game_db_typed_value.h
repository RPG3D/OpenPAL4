#pragma once

#include <cstdint>

namespace pal4 {

class LegacyCrc32;

struct GameDbTypedValueState final {
    std::uint32_t type = 0u;
    std::uint32_t value = 0u;
};
static_assert(sizeof(GameDbTypedValueState) == 8u);

struct GameDbStringObjectState final {
    std::uint32_t flag = 0u;
    std::uint32_t data = 0u;
    std::uint32_t length = 0u;
    std::uint32_t capacity = 0u;
};
static_assert(sizeof(GameDbStringObjectState) == 16u);

struct GameDbValueBoundsState final {
    std::int32_t minimum = 0;
    std::int32_t maximum = 0;
};

class IGameDbTypedValueRuntime {
public:
    virtual ~IGameDbTypedValueRuntime() = default;
    virtual void DestroyString(std::uint32_t object) = 0;
    [[nodiscard]] virtual const char* StringData(std::uint32_t object) = 0;
    [[nodiscard]] virtual std::uint32_t StringLength(
        std::uint32_t object) = 0;
    [[nodiscard]] virtual std::uint32_t AllocateStringObject(
        std::uint32_t bytes) = 0;
    [[nodiscard]] virtual GameDbStringObjectState& StringObject(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint8_t SetErrorFlag(
        GameDbStringObjectState& object, std::uint8_t flag) = 0;
    [[nodiscard]] virtual std::uint32_t AllocateStringText(
        std::uint32_t length) = 0;
    virtual void CopyStringText(std::uint32_t destination,
                                const char* source,
                                std::uint32_t length) = 0;
};

struct GameDbTypedValueErrorState final {
    bool initialized = false;
    std::uint8_t flag = 0u;
    std::uint32_t data = 0u;
    std::uint32_t length = 0u;
    std::uint32_t capacity = 0u;
};

class IGameDbTypedValueErrorRuntime {
public:
    virtual ~IGameDbTypedValueErrorRuntime() = default;
    virtual void SetErrorFlag(GameDbTypedValueErrorState& state,
                              std::uint8_t flag) = 0;
    [[nodiscard]] virtual std::uint32_t AllocateText(
        std::uint32_t length) = 0;
    virtual void CopyText(std::uint32_t destination,
                          const char* source, std::uint32_t length) = 0;
    virtual void RegisterCleanup() = 0;
    virtual void ReleaseText(std::uint32_t data) = 0;
};

class GameDbTypedValue final {
public:
    // IDA 0x654010: zero the 8-byte value then assign from a source.
    [[nodiscard]] static GameDbTypedValueState& Construct(
        GameDbTypedValueState& destination,
        const GameDbTypedValueState& source,
        IGameDbTypedValueRuntime& runtime);
    // IDA 0x654030/0x6540a0: release an owned string and clear the tag.
    static void Destroy(GameDbTypedValueState& state,
                        IGameDbTypedValueRuntime& runtime);
    static void Clear(GameDbTypedValueState& state,
                      IGameDbTypedValueRuntime& runtime);
    // IDA 0x654040: copy a tagged integer, string, or float value.
    [[nodiscard]] static GameDbTypedValueState& AssignFrom(
        GameDbTypedValueState& destination,
        const GameDbTypedValueState& source,
        IGameDbTypedValueRuntime& runtime);
    // IDA 0x654100: mismatched types return the native debug sentinel.
    [[nodiscard]] static std::int32_t GetInteger(
        const GameDbTypedValueState& state);
    // IDA 0x654110: return string object or lazily create global fallback.
    [[nodiscard]] static std::uint32_t GetStringObject(
        const GameDbTypedValueState& value,
        GameDbTypedValueErrorState& fallback,
        std::uint8_t source_high_byte,
        IGameDbTypedValueErrorRuntime& runtime);
    // IDA 0x654220: release the lazily created fallback's text.
    static void DeinitializeErrorString(
        GameDbTypedValueErrorState& fallback,
        IGameDbTypedValueErrorRuntime& runtime);
    // IDA 0x654270: type-tagged float getter.
    [[nodiscard]] static float GetFloat(const GameDbTypedValueState& state);
    // IDA 0x654280: clear old value and install integer tag/payload.
    [[nodiscard]] static std::int32_t SetInteger(
        GameDbTypedValueState& state, std::int32_t number,
        IGameDbTypedValueRuntime& runtime);
    // IDA 0x6542a0: allocate and initialize a 16-byte owned string object.
    [[nodiscard]] static std::uint8_t SetString(
        GameDbTypedValueState& state, const char* text,
        IGameDbTypedValueRuntime& runtime);
    // IDA 0x654400: clear old value and install float tag/payload.
    [[nodiscard]] static std::uint32_t SetFloat(
        GameDbTypedValueState& state, float number,
        IGameDbTypedValueRuntime& runtime);
    // IDA 0x654420: apply scalar bounds or string maximum length.
    static void ApplyBounds(GameDbTypedValueState& state,
                            const GameDbValueBoundsState& bounds,
                            IGameDbTypedValueRuntime& runtime);
};

struct GameDbValueArrayState final {
    std::uint32_t flag = 0u;
    std::uint32_t begin = 0u;
    std::uint32_t end = 0u;
    std::uint32_t capacity = 0u;
};
static_assert(sizeof(GameDbValueArrayState) == 16u);

class IGameDbValueArrayRuntime {
public:
    virtual ~IGameDbValueArrayRuntime() = default;
    [[nodiscard]] virtual std::uint32_t Allocate(
        std::uint32_t bytes) = 0;
    virtual void Free(std::uint32_t address) = 0;
};

class IGameDbValueArrayLookupRuntime : public IGameDbTypedValueRuntime,
                                       public IGameDbTypedValueErrorRuntime {
public:
    ~IGameDbValueArrayLookupRuntime() override = default;
    [[nodiscard]] virtual GameDbTypedValueState& ValueAt(
        std::uint32_t address) = 0;
};

class IGameDbValueArrayMutationRuntime : public IGameDbValueArrayLookupRuntime,
                                         public IGameDbValueArrayRuntime {
public:
    ~IGameDbValueArrayMutationRuntime() override = default;
};

class GameDbValueArray final {
public:
    // IDA 0x6558b0: initialize and reserve 8-byte typed value slots.
    [[nodiscard]] static GameDbValueArrayState& Construct(
        GameDbValueArrayState& state, std::int32_t count,
        IGameDbValueArrayRuntime& runtime);
    // IDA 0x6559b0/0x6559f0: clear values and read one integer by index.
    [[nodiscard]] static std::uint32_t CleanupProperties(
        GameDbValueArrayState& state,
        IGameDbValueArrayMutationRuntime& runtime);
    [[nodiscard]] static std::int32_t GetIntegerAt(
        const GameDbValueArrayState& state, std::uint32_t index,
        IGameDbValueArrayLookupRuntime& runtime);
    // IDA 0x655a20: read indexed string object or static row error text.
    [[nodiscard]] static std::uint32_t GetStringObjectAt(
        const GameDbValueArrayState& state, std::uint32_t index,
        GameDbTypedValueErrorState& row_fallback,
        GameDbTypedValueErrorState& value_fallback,
        IGameDbValueArrayLookupRuntime& runtime);
    // IDA 0x655af0: release static row-error text.
    static void DeinitializeRowError(
        GameDbTypedValueErrorState& row_fallback,
        IGameDbTypedValueErrorRuntime& runtime);
    // IDA 0x655b40/0x655b70: indexed float and unchecked entry address.
    [[nodiscard]] static float GetFloatAt(
        const GameDbValueArrayState& state, std::uint32_t index,
        IGameDbValueArrayLookupRuntime& runtime);
    [[nodiscard]] static std::uint32_t EntryAddressAt(
        const GameDbValueArrayState& state, std::int32_t index);
    // IDA 0x655b80: assign and bound-check one indexed value.
    [[nodiscard]] static bool SetAt(
        GameDbValueArrayState& state, std::uint32_t index,
        const GameDbTypedValueState& value,
        const GameDbValueBoundsState& bounds,
        IGameDbValueArrayLookupRuntime& runtime);
    // IDA 0x655bd0: convert an indexed temporary value to integer or CRC.
    [[nodiscard]] static std::int32_t GetValueAt(
        const GameDbValueArrayState& state, std::uint32_t index,
        IGameDbValueArrayLookupRuntime& runtime, LegacyCrc32& crc);
    // IDA 0x655d10: append one value and constrain the new last element.
    static void Add(GameDbValueArrayState& state,
                    const GameDbTypedValueState& value,
                    const GameDbValueBoundsState& bounds,
                    IGameDbValueArrayMutationRuntime& runtime);
    // IDA 0x655d60: clean all elements, release storage and reset pointers.
    [[nodiscard]] static std::uint32_t Clear(
        GameDbValueArrayState& state,
        IGameDbValueArrayMutationRuntime& runtime);
    // IDA 0x655da0: insert repeated 8-byte tagged values at an address.
    [[nodiscard]] static std::uint32_t Insert(
        GameDbValueArrayState& state, std::uint32_t position,
        std::uint32_t count, const GameDbTypedValueState& value,
        IGameDbValueArrayMutationRuntime& runtime);
    // IDA 0x656000/0x656030: cleanup and copy ranges of 8-byte values.
    static void DestroyRange(std::uint32_t begin, std::uint32_t end,
                             IGameDbValueArrayLookupRuntime& runtime);
    [[nodiscard]] static std::uint32_t CopyRange(
        std::uint32_t source_begin, std::uint32_t source_end,
        std::uint32_t destination,
        IGameDbValueArrayLookupRuntime& runtime);
    // IDA 0x656070: construct a single value only for a nonnull destination.
    [[nodiscard]] static std::uint32_t ConstructAt(
        std::uint32_t destination, const GameDbTypedValueState& value,
        IGameDbValueArrayLookupRuntime& runtime);
};

}  // namespace pal4
