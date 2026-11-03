#pragma once
#include "pal4/ui_navigation_list.h"
namespace pal4 {
class IMenuWindowNavigationRuntime : public IUiNavigationInputRuntime {
public:
    virtual void CloseMenu(std::uint32_t menu) = 0;
};
class MenuWindowNavigation final {
public:
    // IDA 0x4B4C20. The embedded navigation is at owner+40.
    static std::uint8_t HandleKey(std::uint32_t navigation,std::uint32_t event,
                                 IMenuWindowNavigationRuntime& runtime);
};
}
