#pragma once
#include "pal4/portable/inventory.h"
#include <functional>
#include <optional>
namespace pal4::portable {
// 回调接平台无关的提示/声音队列，文字均已从原CP936解码。
class InventoryServices {
public:
    using TextLookup=std::function<std::optional<std::string>(std::uint32_t)>;
    using Hint=std::function<void(std::string_view,float,float,float)>;
    using Sound=std::function<void(std::string_view,std::uint32_t)>;
    InventoryServices(Inventory&,const PropertyCatalog&,TextLookup,Hint,Sound);
    ~InventoryServices();
    void Add(std::uint32_t id,std::int32_t count,bool show_hint);
    void Remove(std::uint32_t id,std::int32_t count,bool show_hint);
private:struct Impl;std::unique_ptr<Impl> impl_;
};
}
