#pragma once

#include <cstdint>
#include <map>

namespace pal4 {

class IStringBufferStorageRuntime {
public:
    virtual ~IStringBufferStorageRuntime() = default;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t size) = 0;
    [[nodiscard]] virtual std::uint32_t Free(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t ReadDword(std::uint32_t address) = 0;
    virtual void WriteDword(std::uint32_t address, std::uint32_t value) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    // Stable storage for shared records in this allocation domain. Independent
    // address spaces/allocators retain their own service by default.
    [[nodiscard]] virtual IStringBufferStorageRuntime& SharedRecordStorage() noexcept {
        return *this;
    }
};

class IStringBufferRuntime : public IStringBufferStorageRuntime {
public:
    ~IStringBufferRuntime() override = default;

    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    virtual void Copy(std::uint32_t destination, std::uint32_t source,
                      std::uint32_t size) = 0;
    [[nodiscard]] virtual std::uint32_t FindSubstring(
        std::uint32_t string, std::uint32_t substring) = 0;
    [[nodiscard]] virtual std::uint32_t FindCharacter(
        std::uint32_t string, std::int32_t character) = 0;
    [[nodiscard]] virtual std::uint32_t FindLastCharacter(
        std::uint32_t string, std::int32_t character) = 0;
    [[nodiscard]] virtual std::int32_t CompareCaseInsensitive(
        std::uint32_t left, std::uint32_t right) = 0;
    virtual void ConvertUppercase(std::uint32_t string) = 0;
    virtual void ConvertLowercase(std::uint32_t string) = 0;
};

class IStringBufferFormatRuntime {
public:
    virtual ~IStringBufferFormatRuntime() = default;
    [[nodiscard]] virtual std::int32_t Vsprintf(
        std::uint32_t destination, std::uint32_t format,
        std::uint32_t arguments) = 0;
};

struct StringBufferState {
    std::uint32_t data{};
    std::uint32_t length{};
    std::uint32_t capacity{};
};

class IStringAssignRuntime {
public:
    virtual ~IStringAssignRuntime() = default;
    virtual void ThrowRange() = 0;
    virtual void ThrowLength() = 0;
    virtual void DetachAndReset(StringBufferState& state) = 0;
    [[nodiscard]] virtual bool AllocateText(StringBufferState& state,
                                             std::uint32_t length,
                                             std::uint8_t mode) = 0;
    virtual void SetTextSize(StringBufferState& state,
                             std::uint32_t length) = 0;
    virtual void SetErrorFlag(StringBufferState& state,
                              std::uint8_t flag) = 0;
    virtual void Reallocate(StringBufferState& state,
                            std::uint32_t length) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    virtual void Copy(std::uint32_t destination, std::uint32_t source,
                      std::uint32_t length) = 0;
    virtual void Free(std::uint32_t address) = 0;
};

class IStringResetRuntime {
public:
    virtual ~IStringResetRuntime() = default;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    [[nodiscard]] virtual std::uint32_t Free(std::uint32_t address) = 0;
};

class IStringUniqueRuntime {
public:
    virtual ~IStringUniqueRuntime() = default;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    virtual void WriteByte(std::uint32_t address,
                           std::uint8_t value) = 0;
    [[noreturn]] virtual void ThrowLength() = 0;
    [[nodiscard]] virtual std::uint32_t SetSize(
        StringBufferState& state, std::uint32_t length) = 0;
    [[nodiscard]] virtual std::uint32_t Reallocate(
        StringBufferState& state, std::uint32_t length) = 0;
};

class StringUniqueRuntimeStub : public IStringUniqueRuntime {
public:
    [[nodiscard]] std::uint8_t ReadByte(
        std::uint32_t address) override;
    void WriteByte(std::uint32_t address,
                   std::uint8_t value) override;
    [[noreturn]] void ThrowLength() override;
    [[nodiscard]] std::uint32_t SetSize(
        StringBufferState& state, std::uint32_t length) override;
    [[nodiscard]] std::uint32_t Reallocate(
        StringBufferState& state, std::uint32_t length) override;

protected:
    std::map<std::uint32_t, std::uint8_t> bytes_;
};

class IStringReallocateRuntime {
public:
    virtual ~IStringReallocateRuntime() = default;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    virtual void Copy(std::uint32_t destination, std::uint32_t source,
                      std::uint32_t bytes) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    virtual void Free(std::uint32_t address) = 0;
};

class IStringDetachRuntime : public IStringReallocateRuntime {
public:
    virtual void ThrowLength() = 0;
    virtual void SetErrorFlag(StringBufferState& state,
                              std::uint8_t flag) = 0;
};

class IStringCompareRangeRuntime {
public:
    virtual ~IStringCompareRangeRuntime() = default;
    virtual void ThrowRange() = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
};

class StringBuffer final {
public:
    StringBuffer() = default;
    explicit StringBuffer(StringBufferState state) : state_(state) {}
    [[nodiscard]] static constexpr std::uint32_t MaxSize() noexcept {
        return 0xfffffffdu;
    }
    // IDA 0x4ee730: address of the shared empty log string.
    [[nodiscard]] static std::uint32_t EmptyLogMessageAddress() noexcept { return 0x841964u; }
    // IDA 0x5c3c40: the native record stores its data pointer at +4.
    [[nodiscard]] static std::uint32_t CharacterAddressAt(
        std::uint32_t record, std::int32_t index,
        IStringBufferRuntime& runtime);
    [[nodiscard]] static StringBuffer Create(std::uint32_t source,
                                             std::uint32_t length,
                                             IStringBufferRuntime& runtime);
    // IDA 0x5eca40: construct from at most the requested source prefix.
    [[nodiscard]] static StringBuffer CreatePrefix(
        const StringBufferState& source, std::int32_t requested_length,
        IStringBufferRuntime& runtime);
    // IDA 0x4bb860.
    [[nodiscard]] static std::uint32_t ExtractTailAt(
        std::uint32_t source_record,
        std::uint32_t destination_record,
        std::int32_t requested_length,
        IStringBufferRuntime& runtime);
    [[nodiscard]] StringBuffer& CopyConstruct(
        const StringBuffer& source, IStringBufferRuntime& runtime);
    [[nodiscard]] static StringBuffer Concatenate(
        std::uint32_t first, std::uint32_t first_length,
        std::uint32_t second, std::uint32_t second_length,
        IStringBufferRuntime& runtime);
    [[nodiscard]] StringBuffer& ConcatenateRecords(
        const StringBufferState& first, const StringBufferState& second,
        IStringBufferRuntime& runtime);
    [[nodiscard]] StringBuffer& ConcatenateRecordAndCString(
        const StringBufferState& first, std::uint32_t c_string,
        IStringBufferRuntime& runtime);

    [[nodiscard]] StringBuffer& InitializeEmpty(
        IStringBufferRuntime& runtime);
    [[nodiscard]] static std::uint32_t InitializeEmptyAt(
        std::uint32_t self_address, IStringBufferStorageRuntime& runtime);
    [[nodiscard]] StringBuffer& InitializeFromCString(
        std::uint32_t source, IStringBufferRuntime& runtime);
    [[nodiscard]] static std::uint32_t InitializeAt(
        std::uint32_t self_address, std::uint32_t source,
        std::uint32_t length, IStringBufferRuntime& runtime);
    [[nodiscard]] static std::uint32_t CleanupAt(
        std::uint32_t self_address, IStringBufferStorageRuntime& runtime);
    [[nodiscard]] std::uint32_t Cleanup(IStringBufferRuntime& runtime);
    void ReallocateAndInitialize(std::uint32_t requested_length,
                                 IStringBufferRuntime& runtime);
    void Reallocate(std::uint32_t requested_length, bool copy_existing,
                    IStringBufferRuntime& runtime);
    [[nodiscard]] StringBuffer& FormatVariadic(
        std::uint32_t format, std::uint32_t arguments,
        IStringBufferRuntime& runtime,
        IStringBufferFormatRuntime& formatter);
    // Returns the match offset from data, or -1. start_offset uses 32-bit address arithmetic.
    [[nodiscard]] std::int32_t FindSubstring(
        std::uint32_t substring, std::uint32_t start_offset,
        IStringBufferRuntime& runtime) const;
    [[nodiscard]] std::int32_t FindSubstringOffset(
        std::uint32_t substring_pointer, std::uint32_t start_offset,
        IStringBufferRuntime& runtime) const;
    [[nodiscard]] std::int32_t FindLastCharacter(
        std::int32_t character, IStringBufferRuntime& runtime) const;
    [[nodiscard]] static std::int32_t Compare(
        std::uint32_t left_record, std::uint32_t right_record,
        IStringBufferRuntime& runtime);
    [[nodiscard]] static std::int32_t Compare(
        const StringBuffer& left, const StringBuffer& right,
        IStringBufferRuntime& runtime);
    [[nodiscard]] std::int32_t CompareCaseInsensitive(
        std::uint32_t other_record, IStringBufferRuntime& runtime) const;
    [[nodiscard]] std::int32_t CompareCaseInsensitive(
        const StringBuffer& other, IStringBufferRuntime& runtime) const;
    [[nodiscard]] StringBuffer& ToUpper(IStringBufferRuntime& runtime);
    [[nodiscard]] StringBuffer& ToLower(IStringBufferRuntime& runtime);
    [[nodiscard]] StringBuffer& TrimLeft(std::uint32_t characters,
                                         IStringBufferRuntime& runtime);
    [[nodiscard]] StringBuffer& TrimRight(std::uint32_t characters,
                                          IStringBufferRuntime& runtime);
    [[nodiscard]] StringBuffer& Trim(std::uint32_t characters,
                                     IStringBufferRuntime& runtime);
    [[nodiscard]] StringBuffer& Initialize(std::uint32_t source,
                                           std::uint32_t length,
                                           IStringBufferRuntime& runtime);
    [[nodiscard]] StringBuffer& Append(std::uint32_t source,
                                       std::uint32_t length,
                                       IStringBufferRuntime& runtime);
    [[nodiscard]] StringBuffer& AppendRange(
        const StringBuffer& source, std::uint32_t start,
        std::uint32_t count, IStringAssignRuntime& runtime);
    [[nodiscard]] StringBuffer& AppendRaw(
        std::uint32_t source, std::uint32_t length,
        IStringAssignRuntime& runtime);
    // IDA 0x4d5100: append a null-terminated address-backed byte string.
    [[nodiscard]] StringBuffer& AppendCString(
        std::uint32_t source, IStringAssignRuntime& runtime);
    [[nodiscard]] StringBuffer& Assign(const StringBuffer& source,
                                       std::uint32_t start,
                                       std::uint32_t count,
                                       IStringAssignRuntime& runtime);
    [[nodiscard]] std::uint8_t SetErrorFlag(std::uint8_t flag,
                                             IStringResetRuntime& runtime);
    [[nodiscard]] std::uint32_t SetSize(std::uint32_t size,
                                         IStringResetRuntime& runtime);
    [[nodiscard]] bool AllocateText(std::uint32_t requested_length,
                                    std::uint8_t mode,
                                    IStringAssignRuntime& runtime);
    [[nodiscard]] std::uint32_t ReallocateWithReferenceHeader(
        std::uint32_t requested_length, IStringReallocateRuntime& runtime);
    [[nodiscard]] std::uint32_t DetachAndReset(
        IStringDetachRuntime& runtime);
    // 0x4185f0. Returns the observed low EAX byte after side effects.
    [[nodiscard]] static std::uint8_t EnsureExclusive(
        StringBufferState& state, IStringUniqueRuntime& runtime);
    [[nodiscard]] std::int32_t CompareRange(
        std::uint32_t start, std::uint32_t count,
        std::uint32_t other, std::uint32_t other_length,
        IStringCompareRangeRuntime& runtime) const;
    [[nodiscard]] StringBuffer& Erase(std::uint32_t start,
                                      std::uint32_t count,
                                      IStringAssignRuntime& runtime);

    [[nodiscard]] const StringBufferState& State() const noexcept;

private:
    StringBufferState state_{};
};

}  // namespace pal4
