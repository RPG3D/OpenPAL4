#pragma once

#include <cstdint>
#include <map>

namespace pal4 {

class ICeguiStreamBufferRuntime {
public:
    virtual ~ICeguiStreamBufferRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadDword(
        std::uint32_t address) = 0;
    virtual void WriteDword(std::uint32_t address,
                            std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(
        std::uint32_t address) = 0;
    virtual void WriteByte(std::uint32_t address,
                           std::uint8_t value) = 0;
    virtual void CopyBytes(std::uint32_t destination,
                           std::uint32_t source,
                           std::uint32_t bytes) = 0;
    [[nodiscard]] virtual std::int32_t Underflow(
        std::uint32_t streambuf) = 0;
    [[nodiscard]] virtual std::int32_t ApplyUnderflowResult(
        std::uint32_t streambuf, std::int32_t value) = 0;
    [[nodiscard]] virtual std::int32_t Uflow(
        std::uint32_t streambuf) = 0;
    [[nodiscard]] virtual std::int32_t Overflow(
        std::uint32_t streambuf, std::uint8_t value) = 0;
    [[nodiscard]] virtual std::uint32_t FposLocale() = 0;
};

class CeguiStreamBufferRuntimeStub : public ICeguiStreamBufferRuntime {
public:
    [[nodiscard]] std::uint32_t ReadDword(
        std::uint32_t address) override;
    void WriteDword(std::uint32_t address,
                    std::uint32_t value) override;
    [[nodiscard]] std::uint8_t ReadByte(
        std::uint32_t address) override;
    void WriteByte(std::uint32_t address,
                   std::uint8_t value) override;
    void CopyBytes(std::uint32_t destination,
                   std::uint32_t source,
                   std::uint32_t bytes) override;
    [[nodiscard]] std::int32_t Underflow(
        std::uint32_t streambuf) override;
    [[nodiscard]] std::int32_t ApplyUnderflowResult(
        std::uint32_t streambuf, std::int32_t value) override;
    [[nodiscard]] std::int32_t Uflow(
        std::uint32_t streambuf) override;
    [[nodiscard]] std::int32_t Overflow(
        std::uint32_t streambuf,
        std::uint8_t value) override;
    [[nodiscard]] std::uint32_t FposLocale() override;

private:
    std::map<std::uint32_t, std::uint32_t> words_;
    std::map<std::uint32_t, std::uint8_t> bytes_;
};

class CeguiStreamBufferOperations final {
public:
    // 0x416e00, 0x416e30, 0x416ed0.
    [[nodiscard]] static std::int32_t ReadChar(
        std::uint32_t streambuf, ICeguiStreamBufferRuntime& runtime);
    [[nodiscard]] static std::uint32_t ReadBytes(
        std::uint32_t streambuf, std::uint32_t destination,
        std::int32_t bytes, ICeguiStreamBufferRuntime& runtime);
    [[nodiscard]] static std::uint32_t WriteBytes(
        std::uint32_t streambuf, std::uint32_t source,
        std::int32_t bytes, ICeguiStreamBufferRuntime& runtime);

    // 0x416f70, 0x416fa0, 0x416fd0.
    [[nodiscard]] static std::uint32_t SeekOffFailure(
        std::uint32_t output, ICeguiStreamBufferRuntime& runtime);
    [[nodiscard]] static std::uint32_t InitializeFailurePosition(
        std::uint32_t output, ICeguiStreamBufferRuntime& runtime);
    [[nodiscard]] static std::uint32_t SetBuffer(
        std::uint32_t streambuf, std::uint32_t ignored_buffer,
        std::uint32_t ignored_size) noexcept;
    // 0x417360.
    [[nodiscard]] static std::int32_t PeekChar(
        std::uint32_t streambuf, ICeguiStreamBufferRuntime& runtime);

private:
    [[nodiscard]] static std::uint32_t WriteFailurePosition(
        std::uint32_t output, ICeguiStreamBufferRuntime& runtime);
};

}  // namespace pal4
