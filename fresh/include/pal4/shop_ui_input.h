#pragma once
#include "pal4/ui_navigation_list.h"
namespace pal4 {
class IShopUiInputRuntime {
public:
    virtual ~IShopUiInputRuntime()=default;
    virtual void CloseShopFrame(std::uint32_t frame)=0;
};
class ShopUiInput final {
public:
    // IDA486DB0, first slot of the navigation member at frame+28.
    static std::uint8_t HandleKey(std::uint32_t list,std::uint32_t event,
        IUiNavigationInputRuntime& navigation,IShopUiInputRuntime& actions);
};
}
