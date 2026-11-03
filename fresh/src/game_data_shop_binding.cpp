#include "pal4/game_data_shop_binding.h"

#include <stdexcept>
#include <string>

namespace pal4 {
namespace {

void RequireTable(std::uint32_t address, std::string_view name,
                  IGameDataShopBindingRuntime& runtime) {
    if (address) return;
    const auto message =
        std::string("ERROR [ palDBShopManager::verifyDataTable < the table ") +
        std::string(name) + "  doesn't exist>]";
    runtime.LogError(message);
    throw std::runtime_error(message);
}

}  // namespace

std::uint8_t GameDataShopBinding::RegisterHandlers(
    std::uint32_t manager, IGameDataShopBindingRuntime& runtime) {
    static_cast<void>(runtime.RegisterHandler(manager, "ShopProperty",
                                              0x4ef1a0u));
    return runtime.RegisterHandler(manager, "Good", 0x4efd30u);
}

std::uint8_t GameDataShopBinding::ResolveTables(
    GameDataShopBindingState& state,
    IGameDataShopBindingRuntime& runtime) {
    state.shop_property = runtime.FindCollection(state.manager,
                                                 "ShopProperty");
    RequireTable(state.shop_property, "ShopProperty", runtime);
    state.good = runtime.FindCollection(state.manager, "Good");
    RequireTable(state.good, "Good", runtime);
    state.shop_type = runtime.FindStringProperty(state.manager, "ShopType");
    RequireTable(state.shop_type, "ShopType", runtime);
    state.goods_type = runtime.FindStringProperty(state.manager,
                                                  "GoodsType");
    RequireTable(state.goods_type, "GoodsType", runtime);
    return 1;
}

}  // namespace pal4
