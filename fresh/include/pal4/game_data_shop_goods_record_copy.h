#pragma once

#include <cstdint>

namespace pal4 {

class IGameDataShopGoodsRecordCopyRuntime {
public:
    virtual ~IGameDataShopGoodsRecordCopyRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    virtual void AssignText(std::uint32_t destination,
                            std::uint32_t source) = 0;
    [[nodiscard]] virtual bool RecordsEqual(std::uint32_t left,
                                            std::uint32_t right) = 0;
};

class GameDataShopGoodsRecordCopy final {
public:
    static constexpr std::uint32_t kRecordBytes = 36;

    // IDA 0x4f2060: copy-construct a Good record in fresh storage.
    static void Construct(std::uint32_t destination, std::uint32_t source,
                          IGameDataShopGoodsRecordCopyRuntime& runtime);
    // IDA 0x4f2460: assign one existing Good record.
    [[nodiscard]] static std::uint32_t Assign(
        std::uint32_t destination, std::uint32_t source,
        IGameDataShopGoodsRecordCopyRuntime& runtime);

    // IDA 0x4f1d40: assign one source to a half-open Good record range.
    [[nodiscard]] static std::uint32_t AssignRepeated(
        std::uint32_t begin, std::uint32_t end, std::uint32_t source,
        IGameDataShopGoodsRecordCopyRuntime& runtime);
    // IDA 0x4f1ec0: assign a Good record range backward.
    [[nodiscard]] static std::uint32_t AssignBackward(
        std::uint32_t begin, std::uint32_t end,
        std::uint32_t destination_end,
        IGameDataShopGoodsRecordCopyRuntime& runtime);
    // IDA 0x4f2850: copy only the first record of each adjacent equal run.
    [[nodiscard]] static std::uint32_t CopyUnique(
        std::uint32_t begin, std::uint32_t end,
        std::uint32_t destination,
        IGameDataShopGoodsRecordCopyRuntime& runtime);
};

}  // namespace pal4
