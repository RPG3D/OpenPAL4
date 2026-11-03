#pragma once

#include <cstdint>

namespace pal4 {

struct ConfigStringState {
    std::uint8_t flag = 0u;
    std::uint32_t data = 0u;
    std::uint32_t size = 0u;
    std::uint32_t capacity = 0u;
};

class IConfigValueMemory {
public:
    virtual ~IConfigValueMemory() = default;
    [[nodiscard]] virtual std::uint32_t StringLength(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t ReadDword(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(
        std::uint32_t address) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    virtual void Free(std::uint32_t address) = 0;
    virtual void CopyBytes(std::uint32_t destination,
                           std::uint32_t source,
                           std::uint32_t count) = 0;
    virtual void SetErrorFlag(ConfigStringState& text,
                              std::uint8_t flag) = 0;
    virtual void Reallocate(ConfigStringState& text,
                            std::uint32_t size) = 0;
    virtual void LengthError() = 0;
    [[nodiscard]] virtual std::uint32_t EmptyStringAddress() = 0;
    [[nodiscard]] virtual std::int32_t CompareRange(
        std::uint32_t key_address, std::uint32_t start,
        std::uint32_t count, std::uint32_t other,
        std::uint32_t other_length) = 0;
};

class ConfigValueLookup final {
public:
    static constexpr std::uint32_t kNilNodeGlobal = 0x8e3dacu;

    [[nodiscard]] static ConfigStringState& AssignMessage(
        ConfigStringState& text, std::uint32_t source,
        std::uint32_t length, IConfigValueMemory& memory);
    [[nodiscard]] static std::uint32_t FindConfigItem(
        std::uint32_t tree_address, const ConfigStringState& key,
        IConfigValueMemory& memory);
    [[nodiscard]] static std::int32_t CompareText(
        const ConfigStringState& left, std::uint32_t right_key_address,
        IConfigValueMemory& memory);
    [[nodiscard]] static std::uint32_t GetConfigStringValue(
        std::uint32_t document_address, std::uint32_t item_name,
        IConfigValueMemory& memory);
};

}  // namespace pal4
