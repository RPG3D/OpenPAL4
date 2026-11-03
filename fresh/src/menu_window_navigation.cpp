#include "pal4/menu_window_navigation.h"
namespace pal4 {
std::uint8_t MenuWindowNavigation::HandleKey(std::uint32_t navigation,std::uint32_t event,
                                            IMenuWindowNavigationRuntime& runtime) {
    const auto key=runtime.ReadWord(event+16u);
    if(key==1u) { runtime.CloseMenu(navigation-40u);return 1u; }
    if(key!=28u)return UiNavigationList::HandleVerticalKey(navigation,event,runtime);
    const auto begin=runtime.ReadWord(navigation+8u);
    if(begin && runtime.ReadWord(navigation+12u)!=begin) {
        runtime.MouseAtCenter(runtime.ReadWord(begin+runtime.ReadWord(navigation+20u)*4u));
    } else runtime.CloseMenu(navigation-40u);
    return 1u;
}
}
