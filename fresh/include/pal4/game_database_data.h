#pragma once

#include <cstdint>

namespace pal4 {

class IGameDatabaseDataRuntime {
public:
    virtual ~IGameDatabaseDataRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    virtual void ResetManager(std::uint32_t manager) = 0;
    virtual void CallHelper(std::uint32_t helper,
                            std::uint32_t component) = 0;
    [[nodiscard]] virtual std::uint8_t LoadDatabase(
        std::uint32_t manager, std::uint32_t path,
        std::uint32_t callback_table) = 0;
    [[nodiscard]] virtual std::uint32_t FindStringValue(
        std::uint32_t manager, std::uint32_t text) = 0;
};

class GameDatabaseData final {
public:
    static constexpr std::uint32_t kCallbackTable = 0x84488cu;
    static constexpr std::uint32_t kShopPath = 0x8b45a8u;

    // IDA 0x50cb70.
    [[nodiscard]] static std::uint8_t InitializeFromDatabase(
        std::uint32_t object, std::uint32_t path,
        IGameDatabaseDataRuntime& runtime);
    // IDA 0x50cca0.
    [[nodiscard]] static std::uint8_t InitializeShopData(
        std::uint32_t object, std::uint32_t path,
        IGameDatabaseDataRuntime& runtime);
    // IDA 0x50cd10.
    [[nodiscard]] static std::uint32_t Cleanup(
        std::uint32_t object, IGameDatabaseDataRuntime& runtime);
    // IDA 0x50cd30; the original name says Set, but 0x650100 searches.
    [[nodiscard]] static std::uint32_t FindStringValue(
        std::uint32_t object, std::uint32_t text,
        IGameDatabaseDataRuntime& runtime);
};

}  // namespace pal4
