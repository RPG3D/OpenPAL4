#pragma once

#include "pal4/archive_stream_adapter.h"
#include "pal4/string_buffer.h"

#include <cstdint>

namespace pal4 {

class IArchiveTextReadRuntime {
public:
    virtual ~IArchiveTextReadRuntime() = default;
    [[nodiscard]] virtual const char* EmptyData() = 0;
    [[nodiscard]] virtual bool AllocateText(StringBufferState& text,
                                            std::uint32_t length,
                                            std::uint8_t mode) = 0;
    virtual void SetErrorFlag(StringBufferState& text, std::uint8_t flag) = 0;
    virtual void Reallocate(StringBufferState& text,
                            std::uint32_t length) = 0;
    virtual void SetTextSize(StringBufferState& text,
                             std::uint32_t length) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    virtual void Copy(std::uint32_t destination, const char* source,
                      std::uint32_t length) = 0;
};

class ArchiveTextReadRuntimeStub final : public IArchiveTextReadRuntime {
public:
    [[nodiscard]] const char* EmptyData() override;
    [[nodiscard]] bool AllocateText(StringBufferState& text,
                                    std::uint32_t length,
                                    std::uint8_t mode) override;
    void SetErrorFlag(StringBufferState& text, std::uint8_t flag) override;
    void Reallocate(StringBufferState& text, std::uint32_t length) override;
    void SetTextSize(StringBufferState& text, std::uint32_t length) override;
    [[nodiscard]] std::uint8_t ReadByte(std::uint32_t address) override;
    void WriteByte(std::uint32_t address, std::uint8_t value) override;
    void Copy(std::uint32_t destination, const char* source,
              std::uint32_t length) override;
};

class ArchiveTextReader final {
public:
    ArchiveTextReader(ArchiveStreamAdapter& adapter,
                      IArchiveTextReadRuntime& runtime)
        : adapter_(adapter), runtime_(runtime) {}

    // 0x5ec380: read a length-prefixed, NUL-terminated text record.
    [[nodiscard]] std::uint8_t Read(const ArchiveStreamAdapterState& stream,
                                    StringBufferState& destination);

private:
    ArchiveStreamAdapter& adapter_;
    IArchiveTextReadRuntime& runtime_;
};

}  // namespace pal4
