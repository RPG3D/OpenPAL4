#pragma once
#include <array>
#include <cstdint>
#include <memory>
#include <span>
#include <string_view>
namespace pal4 {
class NativeGameDatabase;
struct NativeGameDbTable;
// Immutable native ShopProperty/Good views; text belongs to host std::string.
// Reuses the existing original CRC lookup and category-presence algorithms.
class NativeShopRecordCatalog final {
public:
    using ShopRecord=std::array<std::uint32_t,14>;
    using GoodRecord=std::array<std::uint32_t,9>;
    explicit NativeShopRecordCatalog(const NativeGameDatabase&);
    NativeShopRecordCatalog(const NativeGameDbTable& shops,const NativeGameDbTable& goods);
    ~NativeShopRecordCatalog();
    NativeShopRecordCatalog(const NativeShopRecordCatalog&)=delete;
    NativeShopRecordCatalog& operator=(const NativeShopRecordCatalog&)=delete;
    std::span<const ShopRecord> Shops() const noexcept;
    std::span<const GoodRecord> Goods() const noexcept;
    std::uint32_t FindShop(std::string_view id) const;
    bool HasGoods(std::string_view shop,std::uint32_t category) const;
    bool AllowsSmith(std::string_view shop) const;
    std::string_view Text(std::uint32_t descriptor) const;
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};
}
