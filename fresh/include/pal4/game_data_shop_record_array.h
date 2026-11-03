#pragma once

#include <cstdint>

namespace pal4 {

class IGameDataShopRecordArrayRuntime {
public:
    virtual ~IGameDataShopRecordArrayRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    virtual void Free(std::uint32_t address) = 0;
    virtual void CopyConstruct(std::uint32_t destination,
                               std::uint32_t source,
                               std::uint32_t record_bytes) = 0;
    virtual void Assign(std::uint32_t destination,
                        std::uint32_t source,
                        std::uint32_t record_bytes) = 0;
    virtual void DestroyRange(std::uint32_t begin, std::uint32_t end,
                              std::uint32_t record_bytes) = 0;
};

class GameDataShopRecordArray final {
public:
    static constexpr std::uint32_t kPropertyBytes = 56;
    static constexpr std::uint32_t kGoodsBytes = 36;

    // IDA 0x4f0830: insert one 56-byte ShopProperty record.
    [[nodiscard]] static std::uint32_t InsertProperty(
        std::uint32_t array, std::uint32_t position, std::uint32_t source,
        IGameDataShopRecordArrayRuntime& runtime);
    // IDA 0x4f0a30: count active ShopProperty records.
    [[nodiscard]] static std::uint32_t SizeProperty(
        std::uint32_t array, IGameDataShopRecordArrayRuntime& runtime);
    // IDA 0x4f0a60: copy-construct a half-open ShopProperty range.
    [[nodiscard]] static std::uint32_t CopyPropertyRange(
        std::uint32_t begin, std::uint32_t end,
        std::uint32_t destination,
        IGameDataShopRecordArrayRuntime& runtime);
    // IDA 0x4f0aa0: insert one 36-byte Good record.
    [[nodiscard]] static std::uint32_t InsertGoods(
        std::uint32_t array, std::uint32_t position, std::uint32_t source,
        IGameDataShopRecordArrayRuntime& runtime);
    // IDA 0x4f0c80: copy-construct a half-open Good range.
    [[nodiscard]] static std::uint32_t CopyGoodsRange(
        std::uint32_t begin, std::uint32_t end,
        std::uint32_t destination,
        IGameDataShopRecordArrayRuntime& runtime);
    // IDA 0x4f0cc0: insert repeated 56-byte ShopProperty records.
    static void InsertPropertyCopies(
        std::uint32_t array, std::uint32_t position,
        std::uint32_t count, std::uint32_t source,
        IGameDataShopRecordArrayRuntime& runtime);
    // IDA 0x4f1030: insert repeated 36-byte Good records.
    static void InsertGoodsCopies(
        std::uint32_t array, std::uint32_t position,
        std::uint32_t count, std::uint32_t source,
        IGameDataShopRecordArrayRuntime& runtime);
    // IDA 0x4f1380: copy-construct repeated 56-byte Property records.
    static void FillProperty(
        std::uint32_t destination, std::uint32_t count,
        std::uint32_t source, IGameDataShopRecordArrayRuntime& runtime);
    // IDA 0x4f13b0: copy-construct repeated 36-byte Good records.
    static void FillGoods(
        std::uint32_t destination, std::uint32_t count,
        std::uint32_t source, IGameDataShopRecordArrayRuntime& runtime);
};

}  // namespace pal4
