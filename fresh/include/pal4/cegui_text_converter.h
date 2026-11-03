#pragma once

#include <cstdint>
#include <string_view>

#include "pal4/static_character_buffers.h"

namespace pal4 {

class ICeguiTextConversionRuntime {
public:
    virtual ~ICeguiTextConversionRuntime() = default;
    [[nodiscard]] virtual std::int32_t MultiByteToWideChar(
        std::uint32_t code_page, std::uint32_t flags,
        std::string_view source, std::int32_t source_length,
        std::uint32_t destination, std::int32_t capacity) = 0;
    [[nodiscard]] virtual std::int32_t WideCharToMultiByte(
        std::uint32_t code_page, std::uint32_t flags,
        std::uint32_t source, std::int32_t source_length,
        std::uint32_t destination, std::int32_t capacity) = 0;
    virtual void InsertWide(StaticCharacterBufferState& state,
                            std::uint32_t at, std::uint32_t count,
                            std::uint16_t fill) = 0;
    virtual void CopyWide(StaticCharacterBufferState& state,
                          std::uint32_t destination,
                          std::uint32_t source_end) = 0;
    virtual void InsertByte(StaticCharacterBufferState& state,
                            std::uint32_t at, std::uint32_t count,
                            std::uint8_t fill) = 0;
    virtual void WriteWide(std::uint32_t address,
                           std::uint16_t value) = 0;
    virtual void WriteByte(std::uint32_t address,
                           std::uint8_t value) = 0;
};

class CeguiTextConversionStub final : public ICeguiTextConversionRuntime {
public:
    [[nodiscard]] std::int32_t MultiByteToWideChar(
        std::uint32_t code_page, std::uint32_t flags,
        std::string_view source, std::int32_t source_length,
        std::uint32_t destination, std::int32_t capacity) override;
    [[nodiscard]] std::int32_t WideCharToMultiByte(
        std::uint32_t code_page, std::uint32_t flags,
        std::uint32_t source, std::int32_t source_length,
        std::uint32_t destination, std::int32_t capacity) override;
    void InsertWide(StaticCharacterBufferState& state,
                    std::uint32_t at, std::uint32_t count,
                    std::uint16_t fill) override;
    void CopyWide(StaticCharacterBufferState& state,
                  std::uint32_t destination,
                  std::uint32_t source_end) override;
    void InsertByte(StaticCharacterBufferState& state,
                    std::uint32_t at, std::uint32_t count,
                    std::uint8_t fill) override;
    void WriteWide(std::uint32_t address,
                   std::uint16_t value) override;
    void WriteByte(std::uint32_t address,
                   std::uint8_t value) override;
};

class CeguiTextConverter final {
public:
    [[nodiscard]] static std::uint32_t ConvertToUtf8(
        std::string_view source, StaticCharacterBufferState& state,
        ICeguiTextConversionRuntime& runtime);
};

}  // namespace pal4
