#include "pal4/shop_ui_bootstrap.h"

namespace pal4 {

std::uint32_t ShopUiBootstrap::GetFrameType(
    IShopUiBootstrapRuntime& runtime) {
    return InitializeFrameType(runtime);
}

std::uint32_t ShopUiBootstrap::InitializeFrameType(
    IShopUiBootstrapRuntime& runtime) {
    const auto name = runtime.ReadWord(kNameSource);
    runtime.WriteWord(kCurrentName, name);
    return name;
}

std::uint32_t ShopUiBootstrap::Create(
    const std::uint32_t name, IShopUiBootstrapRuntime& runtime) {
    const auto storage = runtime.Allocate(0xc8u);
    if (storage == 0u) return 0u;
    try {
        return runtime.ConstructShop(storage, name);
    } catch (...) {
        runtime.Free(storage);
        throw;
    }
}

std::uint32_t ShopUiBootstrap::GetFrame(
    IShopUiBootstrapRuntime& runtime) {
    return RegisterFrame(runtime);
}

std::uint32_t ShopUiBootstrap::RegisterFrame(
    IShopUiBootstrapRuntime& runtime) {
    const auto manager = runtime.FrameManager();
    return runtime.ProcessFrameEvent(
        manager, runtime.ReadWord(kCurrentName), kFactoryAddress);
}

}  // namespace pal4
