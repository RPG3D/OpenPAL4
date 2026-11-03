#include "pal4/shop_ui_registration.h"

namespace pal4 {
namespace {

std::uint32_t Register(const std::uint32_t payload,
                       const std::uint32_t vtable,
                       const std::uint32_t key,
                       const std::uint32_t storage,
                       IShopUiRegistrationRuntime& runtime) {
    const auto callback = runtime.Allocate(8u);
    if (callback != 0u) {
        runtime.WriteWord(callback + 4u, payload);
        runtime.WriteWord(callback, vtable);
    }
    try {
        runtime.InsertOrReplace(runtime.FrameManager() + 0x104u,
                                key, callback);
    } catch (...) {
        if (callback != 0u) runtime.ReleaseCallback(callback, 1u);
        throw;
    }
    if (callback != 0u) runtime.ReleaseCallback(callback, 1u);
    return storage;
}

}  // namespace

std::uint32_t ShopUiRegistration::GetFrameworkHandler(
    IShopUiRegistrationRuntime& runtime) {
    return RegisterFramework(runtime);
}

std::uint32_t ShopUiRegistration::RegisterFramework(
    IShopUiRegistrationRuntime& runtime) {
    return Register(runtime.ReadWord(kNameSource), kNameCallbackVtable,
                    kShowSelectorEvent, kShowSelectorStorage, runtime);
}

std::uint32_t ShopUiRegistration::GetTextSetterOne(
    IShopUiRegistrationRuntime& runtime) {
    return RegisterTextSetterOne(runtime);
}

std::uint32_t ShopUiRegistration::RegisterTextSetterOne(
    IShopUiRegistrationRuntime& runtime) {
    return Register(kSetTextFunction, kTextCallbackVtable,
                    kSetShopIdEvent, kSetShopIdStorage, runtime);
}

std::uint32_t ShopUiRegistration::GetTextSetterTwo(
    IShopUiRegistrationRuntime& runtime) {
    return RegisterTextSetterTwo(runtime);
}

std::uint32_t ShopUiRegistration::RegisterTextSetterTwo(
    IShopUiRegistrationRuntime& runtime) {
    return Register(kSetTradeTaskFunction, kTextCallbackVtable,
                    kSetTradeTaskEvent, kSetTradeTaskStorage, runtime);
}

}  // namespace pal4
