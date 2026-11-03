#pragma once

#include "pal4/string_buffer.h"

#include <cstdint>
#include <map>

namespace pal4 {

struct ModeStringBufferRecord {
    std::uint32_t mode_word{};
    StringBufferState buffer{};
};
static_assert(sizeof(ModeStringBufferRecord) == 16u);

class IModeStringBufferRuntime {
public:
    virtual ~IModeStringBufferRuntime() = default;
    [[nodiscard]] virtual std::uint32_t CStringLength(const char* source) = 0;
    [[noreturn]] virtual void ThrowLength() = 0;
    virtual void SetErrorFlag(ModeStringBufferRecord& record,
                              std::uint8_t flag) = 0;
    virtual void Reallocate(ModeStringBufferRecord& record,
                            std::uint32_t length) = 0;
    virtual void Copy(std::uint32_t destination, const char* source,
                      std::uint32_t length) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(
        std::uint32_t address) = 0;
};

class ModeStringBufferRuntimeStub final : public IModeStringBufferRuntime {
public:
    std::uint32_t CStringLength(const char* source) override;
    [[noreturn]] void ThrowLength() override;
    void SetErrorFlag(ModeStringBufferRecord& record,
                      std::uint8_t flag) override;
    void Reallocate(ModeStringBufferRecord& record,
                    std::uint32_t length) override;
    void Copy(std::uint32_t destination, const char* source,
              std::uint32_t length) override;
    void WriteByte(std::uint32_t address, std::uint8_t value) override;
    std::uint8_t ReadByte(std::uint32_t address) override;

private:
    std::uint32_t next_address_{0x10000u};
    std::map<std::uint32_t, std::uint8_t> bytes_;
};

class ModeStringBuffer final {
public:
    // IDA 0x4273a0.
    [[nodiscard]] static ModeStringBufferRecord& InitializeCString(
        ModeStringBufferRecord& record, const char* source,
        std::uint8_t mode, IModeStringBufferRuntime& runtime);
};

}  // namespace pal4
