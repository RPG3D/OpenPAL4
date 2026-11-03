#pragma once

#include <cstdint>

namespace pal4 {

class IGameDataEquipmentAttributeCodecRuntime {
public:
    virtual ~IGameDataEquipmentAttributeCodecRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadRaw(
        std::uint32_t stream, std::uint32_t destination,
        std::uint32_t bytes) = 0;
    [[nodiscard]] virtual std::uint32_t WriteRaw(
        std::uint32_t stream, std::uint32_t source,
        std::uint32_t bytes) = 0;
    virtual void ReadText(std::uint32_t stream,
                          std::uint32_t destination) = 0;
    virtual void WriteText(std::uint32_t stream,
                           std::uint32_t source) = 0;
};

class GameDataEquipmentAttributeCodec final {
public:
    // IDA 0x4d4fe0: load two header words, 13 texts, ten words and base data.
    [[nodiscard]] static std::uint32_t Deserialize(
        std::uint32_t object, std::uint32_t stream,
        IGameDataEquipmentAttributeCodecRuntime& runtime);
    // IDA 0x4d5070: save the same fields in the same order.
    [[nodiscard]] static std::uint32_t Serialize(
        std::uint32_t object, std::uint32_t stream,
        IGameDataEquipmentAttributeCodecRuntime& runtime);
};

}  // namespace pal4
