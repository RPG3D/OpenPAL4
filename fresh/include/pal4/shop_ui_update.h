#pragma once

#include <cstdint>

namespace pal4 {

class IShopUiUpdateRuntime {
public:
    virtual ~IShopUiUpdateRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    virtual void SetVisible(std::uint32_t window, bool visible) = 0;
    virtual void SetPosition(std::uint32_t window,
                             std::uint32_t vector) = 0;
    virtual void SetupMouseEventHandlers(std::uint32_t list,
                                         std::uint32_t window) = 0;
    [[nodiscard]] virtual std::uint32_t TradeSystem() = 0;
    [[nodiscard]] virtual bool HasTradeItems(std::uint32_t system,
                                              std::uint32_t shop_text,
                                              std::uint32_t category) = 0;
    [[nodiscard]] virtual std::uint32_t CStringLength(
        std::uint32_t text) = 0;
    [[nodiscard]] virtual int CompareBytes(std::uint32_t left,
                                            std::uint32_t right,
                                            std::uint32_t length) = 0;
    [[nodiscard]] virtual std::uint32_t Game() = 0;
    [[nodiscard]] virtual std::uint32_t GameEntry(std::uint32_t game,
                                                   std::uint32_t id) = 0;
    [[nodiscard]] virtual std::uint32_t GameDatabase() = 0;
    [[nodiscard]] virtual std::uint32_t FindShopRecord(
        std::uint32_t table, std::uint32_t shop_text) = 0;
    [[nodiscard]] virtual float GetHeight(std::uint32_t window) = 0;
    virtual void SetHeight(std::uint32_t window, float height) = 0;
    [[nodiscard]] virtual float GetYPosition(std::uint32_t window) = 0;
    virtual void SetYPosition(std::uint32_t window, float y) = 0;
};

class ShopUiUpdate final {
public:
    static constexpr std::uint32_t kEmptyErrorString = 0x8dc780u;

    // IDA 0x486af0.
    [[nodiscard]] static std::uint32_t Update(
        std::uint32_t frame, IShopUiUpdateRuntime& runtime);
};

}  // namespace pal4
