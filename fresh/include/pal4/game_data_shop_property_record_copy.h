#pragma once

#include <cstdint>

namespace pal4 {

class IGameDataShopPropertyRecordCopyRuntime {
public:
    virtual ~IGameDataShopPropertyRecordCopyRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    virtual void AssignText(std::uint32_t destination,
                            std::uint32_t source) = 0;
    [[nodiscard]] virtual bool RecordsEqual(std::uint32_t left,
                                            std::uint32_t right) = 0;
};

class GameDataShopPropertyRecordCopy final {
public:
    static constexpr std::uint32_t kRecordBytes = 56;

    // IDA 0x4f1c90: copy-construct a ShopProperty record in fresh storage.
    static void Construct(std::uint32_t destination, std::uint32_t source,
                          IGameDataShopPropertyRecordCopyRuntime& runtime);
    // IDA 0x4f2220: assign one existing ShopProperty record.
    [[nodiscard]] static std::uint32_t Assign(
        std::uint32_t destination, std::uint32_t source,
        IGameDataShopPropertyRecordCopyRuntime& runtime);
    // IDA 0x4f25e0: retain the first record of each adjacent equal run.
    [[nodiscard]] static std::uint32_t CopyUnique(
        std::uint32_t begin, std::uint32_t end,
        std::uint32_t destination,
        IGameDataShopPropertyRecordCopyRuntime& runtime);

    // IDA 0x4f1840: assign one source to every record in a half-open range.
    [[nodiscard]] static std::uint32_t AssignRepeated(
        std::uint32_t begin, std::uint32_t end,
        std::uint32_t source,
        IGameDataShopPropertyRecordCopyRuntime& runtime);
    // IDA 0x4f1a60: assign a source range backward into destination_end.
    [[nodiscard]] static std::uint32_t AssignBackward(
        std::uint32_t begin, std::uint32_t end,
        std::uint32_t destination_end,
        IGameDataShopPropertyRecordCopyRuntime& runtime);
};

}  // namespace pal4
