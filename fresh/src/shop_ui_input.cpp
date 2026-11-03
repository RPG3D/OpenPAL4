#include "pal4/shop_ui_input.h"
namespace pal4 {
std::uint8_t ShopUiInput::HandleKey(std::uint32_t list,std::uint32_t event,
    IUiNavigationInputRuntime& navigation,IShopUiInputRuntime& actions){
    if(navigation.ReadWord(event+16u)!=1u)return UiNavigationList::HandleVerticalKey(list,event,navigation);
    actions.CloseShopFrame(list-0x28u);
    return 1u;
}
}
