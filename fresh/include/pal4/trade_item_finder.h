#pragma once

#include <cstdint>
#include <string_view>

namespace pal4 {

class ITradeItemFinderRuntime {
public:
    virtual ~ITradeItemFinderRuntime() = default;
    [[nodiscard]] virtual std::uint32_t GameDatabaseInstance() = 0;
    [[nodiscard]] virtual std::string_view ReadText(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t TradeData(
        std::uint32_t database_table) = 0;
    [[nodiscard]] virtual std::uint32_t RecordCount(
        std::uint32_t count_field) = 0;
    [[nodiscard]] virtual std::uint32_t RecordBase(
        std::uint32_t base_field) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
};

class TradeItemFinder final {
public:
    static constexpr std::uint32_t kDatabaseTableOffset = 108u;
    static constexpr std::uint32_t kCountFieldOffset = 92u;
    static constexpr std::uint32_t kBaseFieldOffset = 96u;
    static constexpr std::uint32_t kRecordBytes = 36u;
    static constexpr std::uint32_t kRecordIdOffset = 4u;
    static constexpr std::uint32_t kRecordTextOffset = 20u;

    // IDA 0x50fd10.
    [[nodiscard]] static std::uint8_t FindByTextAndId(
        std::uint32_t text_address, std::uint32_t id,
        ITradeItemFinderRuntime& runtime);
};

}  // namespace pal4
