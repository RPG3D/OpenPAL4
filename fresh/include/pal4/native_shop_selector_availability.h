#pragma once
#include <memory>
namespace pal4 {
class NativeShopSelectorLayout;
class NativeShopRecordCatalog;
class NativeTradeSystemOwner;
class NativePalGameIVTextureStorage;
// Original486AF0 visibility/packing against native database and DLL controls.
// Navigation callbacks use the layout's application handler; the controller
// must route those original mouse methods before registering global activity.
class NativeShopSelectorAvailability final {
public:
    NativeShopSelectorAvailability(NativeShopSelectorLayout&,const NativeShopRecordCatalog&,
        NativeTradeSystemOwner&,NativePalGameIVTextureStorage&);
    ~NativeShopSelectorAvailability();
    NativeShopSelectorAvailability(const NativeShopSelectorAvailability&)=delete;
    NativeShopSelectorAvailability& operator=(const NativeShopSelectorAvailability&)=delete;
    void Refresh();
private:
    class Runtime;
    std::unique_ptr<Runtime> runtime_;
};
}
