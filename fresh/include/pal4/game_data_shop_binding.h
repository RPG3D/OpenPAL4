#pragma once

#include <cstdint>
#include <string_view>

namespace pal4 {

struct GameDataShopBindingState {
    std::uint32_t manager = 0;
    std::uint32_t shop_property = 0;
    std::uint32_t good = 0;
    std::uint32_t shop_type = 0;
    std::uint32_t goods_type = 0;
};

class IGameDataShopBindingRuntime {
public:
    virtual ~IGameDataShopBindingRuntime() = default;
    [[nodiscard]] virtual std::uint8_t RegisterHandler(
        std::uint32_t manager, std::string_view name,
        std::uint32_t callback) = 0;
    [[nodiscard]] virtual std::uint32_t FindCollection(
        std::uint32_t manager, std::string_view name) = 0;
    [[nodiscard]] virtual std::uint32_t FindStringProperty(
        std::uint32_t manager, std::string_view name) = 0;
    virtual void LogError(std::string_view message) = 0;
};

class GameDataShopBinding final {
public:
    // IDA 0x4eef50: register handlers for ShopProperty and Good.
    [[nodiscard]] static std::uint8_t RegisterHandlers(
        std::uint32_t manager, IGameDataShopBindingRuntime& runtime);
    // IDA 0x4eef80: resolve four required shop tables in order.
    [[nodiscard]] static std::uint8_t ResolveTables(
        GameDataShopBindingState& state,
        IGameDataShopBindingRuntime& runtime);
};

}  // namespace pal4
