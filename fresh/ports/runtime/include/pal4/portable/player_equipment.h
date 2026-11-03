#pragma once
#include "pal4/portable/player.h"
#include <unordered_map>
#include <vector>

namespace pal4::portable {
struct EquipmentDefinition {
    std::uint32_t id{}, type{}, skill{};
    PlayerStats stats;
    std::vector<RoleField> fields; // 保留完整原表字段；文本已转UTF-8。
};
struct EquipmentCatalog {
    std::array<std::uint32_t,7> classes{};
    std::unordered_map<std::uint32_t,EquipmentDefinition> definitions;
};
// 用户原数据库经现有解析器生成的P4EQ v1本地缓存。
EquipmentCatalog ReadEquipment(std::istream& input);
struct EquipmentInstance {
    std::uint32_t sequence{},definition{};
    EquipmentDefinition snapshot; // 原4C44A0复制定义；每个实例拥有独立字段和属性。
};
class PlayerEquipment {
public:
    PlayerEquipment(PlayerRoster& players,const EquipmentCatalog& catalog);
    ~PlayerEquipment();
    PlayerEquipment(const PlayerEquipment&)=delete;
    PlayerEquipment& operator=(const PlayerEquipment&)=delete;
    void Add(std::int32_t player,std::uint32_t equipment);
    void WriteReport(std::ostream& output) const;
    const std::vector<EquipmentInstance>& Instances() const;
private:struct Impl;std::unique_ptr<Impl> impl_;
};
}
