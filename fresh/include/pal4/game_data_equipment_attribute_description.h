#pragma once

#include <cstdint>
#include <string>

namespace pal4 {

class IGameDataEquipmentAttributeDescriptionRuntime {
public:
    virtual ~IGameDataEquipmentAttributeDescriptionRuntime() = default;
    [[nodiscard]] virtual std::uint32_t IndexedValue(
        std::uint32_t object, std::uint32_t index) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual float ReadFloat(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::string Label(std::uint32_t key) = 0;
    virtual void CopyDescription(std::uint32_t destination,
                                 const std::string& text) = 0;
};

class GameDataEquipmentAttributeDescription final {
public:
    // IDA 0x4d2d20: build attribute text from integer and percent fields.
    [[nodiscard]] static std::uint32_t Build(
        std::uint32_t object, std::uint32_t destination,
        IGameDataEquipmentAttributeDescriptionRuntime& runtime);
};

}  // namespace pal4
