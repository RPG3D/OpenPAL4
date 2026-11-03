#pragma once
#include <cstdint>
#include <cstddef>
#include <optional>
namespace pal4 {
// 只暴露游戏记录操作；owner可使用原适配器或现代容器，不模拟私有STL布局。
class IInventoryItemChangesRuntime {
public:
    virtual ~IInventoryItemChangesRuntime()=default;
    virtual std::uint32_t Definition(std::uint32_t id)=0;
    virtual std::uint32_t Slot(std::uint32_t definition)=0;
    virtual void Observe(std::uint32_t id)=0;
    virtual std::uint32_t Find(std::uint32_t slot,std::uint32_t id)=0;
    virtual std::uint32_t NextSequence()=0;
    virtual void NextSequence(std::uint32_t)=0;
    virtual std::uint32_t Append(std::uint32_t slot,std::uint32_t definition,std::uint32_t sequence,std::uint32_t count)=0;
    virtual std::uint32_t Count(std::uint32_t record)=0;
    virtual void Count(std::uint32_t record,std::uint32_t count)=0;
    virtual std::uint32_t Sequence(std::uint32_t record)=0;
    virtual void RemoveRecent(std::uint32_t sequence,bool trim_full)=0;
    virtual void AppendRecent(std::uint32_t sequence)=0;
    virtual void Erase(std::uint32_t slot,std::uint32_t sequence)=0;
};
class InventoryItemChanges {
public:
    // 原4C7000/4C71D0的游戏规则，与记录存储分离。
    static std::uint32_t Add(std::uint32_t id,std::int32_t count,IInventoryItemChangesRuntime&);
    static std::uint32_t Remove(std::uint32_t id,std::int32_t count,IInventoryItemChangesRuntime&);
};
// 原最近获得列表：优先移除同序号；新记录在至少6项时移除最早项。
template<class Read> std::optional<std::size_t> InventoryRecentEraseIndex(std::size_t size,Read read,std::uint32_t sequence,bool trim_full){
    for(std::size_t i=0;i<size;++i)if(read(i)==sequence)return i;
    if(trim_full && size>=6)return 0;
    return {};
}
}
