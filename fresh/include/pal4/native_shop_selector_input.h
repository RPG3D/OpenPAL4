#pragma once
#include <cstdint>
#include <functional>
#include <memory>
namespace pal4 {
class INativeSelectorNavigationLayout;
class NativeEventHandlerArray;
// Borrows layout/activity. The selector owner registers/removes its frame+28
// activity and supplies the actual ShopUiActions close composition.
class NativeShopSelectorInput final {
public:
    enum class KeyPolicy { shop, world_map };
    NativeShopSelectorInput(INativeSelectorNavigationLayout&,NativeEventHandlerArray&,
        std::uint32_t manager,std::function<void()> close, KeyPolicy policy=KeyPolicy::shop);
    ~NativeShopSelectorInput();
    NativeShopSelectorInput(const NativeShopSelectorInput&)=delete;
    NativeShopSelectorInput& operator=(const NativeShopSelectorInput&)=delete;
    bool OwnsActivity(std::uint32_t)const noexcept;
    std::uint32_t Callback(std::uint32_t member,std::uint32_t slot);
    std::uint8_t Predicate(std::uint32_t member,std::uint32_t slot,std::uint32_t event);
    bool Mouse(std::uint32_t method,std::uint32_t event);
private:
    class Runtime;
    std::shared_ptr<Runtime> runtime_;
};
}
