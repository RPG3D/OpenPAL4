#pragma once

#include <cstdint>
#include <string_view>

namespace pal4 {

class IGameDataEquipmentTextRuntime {
public:
    virtual ~IGameDataEquipmentTextRuntime() = default;
    [[nodiscard]] virtual bool PrepareTemporary(std::string_view text) = 0;
    virtual void InitializeDestination(std::uint32_t destination,
                                       std::uint8_t seed) = 0;
    virtual void Assign(std::uint32_t destination,
                        std::string_view temporary_text) = 0;
    virtual void ReleaseTemporary() = 0;
};

class GameDataEquipmentTextFormatter final {
public:
    // IDA 0x4d2af0: signed decimal text copied into a 16-byte text record.
    [[nodiscard]] static std::uint32_t FromInteger(
        std::uint32_t destination, std::int32_t value, std::uint8_t seed,
        IGameDataEquipmentTextRuntime& runtime);
    // IDA 0x4d2c10: printf-style formatting through a 1024-byte buffer.
    [[nodiscard]] static std::uint32_t Format(
        std::uint32_t destination, std::uint8_t seed,
        IGameDataEquipmentTextRuntime& runtime, const char* format, ...);
};

}  // namespace pal4
