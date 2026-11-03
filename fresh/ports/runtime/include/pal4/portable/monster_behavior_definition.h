#pragma once
#include "pal4/binary_xml_config.h"
#include <iosfwd>

namespace pal4::portable {
struct MonsterBehaviorChoice {
    std::int32_t class_id{},proportion{},wait_mode{};
    float wait_time{},expect_time{};
};
struct MonsterBehaviorGroup {
    std::vector<MonsterBehaviorChoice> choices;
    // 原加载器提前返回时不会写组模式；不可擅自补默认值。
    std::optional<std::int32_t> mode;
};
struct MonsterBehaviorDefinition {
    std::optional<MonsterBehaviorGroup> fuzzy;
};
// 复用原431CE0创建事件/原子项的顺序，输出拥有所有权的数据，尚不执行事件。
MonsterBehaviorDefinition ReadMonsterBehaviorDefinition(const BinaryXmlConfig& behavior);
void WriteMonsterBehaviorDefinition(std::ostream&,const MonsterBehaviorDefinition&);
} // namespace pal4::portable
