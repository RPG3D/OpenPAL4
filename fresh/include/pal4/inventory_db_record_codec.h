#pragma once

#include <cstdint>

namespace pal4 {

class IInventoryDbRecordRuntime {
public:
    virtual ~IInventoryDbRecordRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t ReadStreamWord(std::uint32_t stream) = 0;
    [[nodiscard]] virtual std::uint32_t WriteStreamWord(
        std::uint32_t stream, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t FindGameData(std::uint32_t id) = 0;
};

class InventoryDbRecordCodec final {
public:
    // IDA 0x4c6da0: read two fields and resolve an ID to a GameDB pointer.
    [[nodiscard]] static bool Read(std::uint32_t record, std::uint32_t stream,
                                   IInventoryDbRecordRuntime& runtime);
    // IDA 0x4c6e00: write two fields and the pointed-to record's ID.
    [[nodiscard]] static std::uint32_t Write(
        std::uint32_t record, std::uint32_t stream,
        IInventoryDbRecordRuntime& runtime);
};

}  // namespace pal4
