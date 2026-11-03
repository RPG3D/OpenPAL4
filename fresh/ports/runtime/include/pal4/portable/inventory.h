#pragma once
#include <cstdint>
#include <iosfwd>
#include <map>
#include <memory>
#include <optional>
#include <span>
#include <string>
namespace pal4::portable {
struct PropertyDefinition {
    std::uint32_t id{},category{};
    std::string name;
    // Original database text, decoded once at the package boundary.
    // Legacy P4PI catalogs contain only name/category; production reads CPK.
    std::string description,property,icon,model,texture,attached_effect;
};
using PropertyCatalog=std::map<std::uint32_t,PropertyDefinition>;
PropertyCatalog ReadPropertyCatalog(std::istream&);
struct InventoryRecord {std::uint32_t id{},category{},sequence{},count{};};
// 物品定义借用者；必须晚于Inventory销毁。记录使用现代容器。
class Inventory {
public:
    explicit Inventory(const PropertyCatalog&);
    ~Inventory();
    Inventory(const Inventory&)=delete;
    Inventory& operator=(const Inventory&)=delete;
    void Add(std::uint32_t id,std::int32_t count);
    void Remove(std::uint32_t id,std::int32_t count);
    std::uint32_t Count(std::uint32_t id) const;
    bool Seen(std::uint32_t id) const;
    std::int32_t Money() const;
    std::int32_t AdjustMoney(std::int32_t delta);
    std::span<const InventoryRecord> Records() const;
    std::span<const std::uint32_t> Recent() const;
    // Stable acquisition sequences are UI item IDs, not database IDs or pointers.
    std::optional<InventoryRecord> FindSequence(std::uint32_t sequence,std::uint32_t category=5) const;
    std::size_t CategoryCount(std::uint32_t category) const;
    std::optional<InventoryRecord> AtReverse(std::uint32_t category,std::size_t index) const;
    std::optional<InventoryRecord> Featured(std::uint32_t category) const;
    void SetFeatured(std::uint32_t category,std::uint32_t sequence);
    void WriteReport(std::ostream&) const;
private:struct Impl;std::unique_ptr<Impl> impl_;std::int32_t money_{}; // Original4C60F0 NewGame reset.
};
}
