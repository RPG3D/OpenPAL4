#pragma once
#include "pal4/portable/object_memory.h"
#include "pal4/portable/scene_data.h"
namespace pal4::portable::detail {
// MST数值视图和XML身份；文本/文档仍借用已验证的SceneMonster解析结果。
// 不提供旧x86字符串或XML对象ABI。记录和存储须比本视图活得久。
class MonsterComponentSource final {
public:
    MonsterComponentSource(ObjectMemory&,const SceneMonster&);
    ~MonsterComponentSource();
    MonsterComponentSource(const MonsterComponentSource&)=delete;
    MonsterComponentSource& operator=(const MonsterComponentSource&)=delete;
    std::uint32_t Token() const;
    std::uint32_t ChildToken() const;
    std::uint32_t Document(std::uint32_t source) const;
    std::uint32_t FirstChild(std::uint32_t document,std::uint32_t index) const;
    const SceneMonster& Record() const;
    void Close();
private:struct Impl;std::unique_ptr<Impl> impl_;
};
}
