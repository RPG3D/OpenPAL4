#include "pal4/inventory_item_changes.h"
#include <bit>
namespace pal4 {
namespace {
std::int32_t Negate(std::int32_t value){return std::bit_cast<std::int32_t>(0u-static_cast<std::uint32_t>(value));}
}
std::uint32_t InventoryItemChanges::Add(std::uint32_t id,std::int32_t count,IInventoryItemChangesRuntime& runtime){
    if(count<0)return Remove(id,Negate(count),runtime);
    const auto definition=runtime.Definition(id);if(!definition)return 0;
    const auto slot=runtime.Slot(definition);if(!slot)return 0;
    runtime.Observe(id);auto record=runtime.Find(slot,id);
    if(!record){const auto sequence=runtime.NextSequence();runtime.NextSequence(sequence+1);
        record=runtime.Append(slot,definition,sequence,static_cast<std::uint32_t>(count>99?99:count));
    }else{const auto updated=runtime.Count(record)+static_cast<std::uint32_t>(count);
        runtime.Count(record,std::bit_cast<std::int32_t>(updated)>99?99u:updated);}
    const auto sequence=runtime.Sequence(record);runtime.RemoveRecent(sequence,true);runtime.AppendRecent(sequence);return record;
}
std::uint32_t InventoryItemChanges::Remove(std::uint32_t id,std::int32_t count,IInventoryItemChangesRuntime& runtime){
    if(count<0)return Add(id,Negate(count),runtime);
    const auto definition=runtime.Definition(id);if(!definition)return 0;
    const auto slot=runtime.Slot(definition);if(!slot)return 0;
    const auto record=runtime.Find(slot,id);if(!record)return 0;
    const auto updated=runtime.Count(record)-static_cast<std::uint32_t>(count);
    if(std::bit_cast<std::int32_t>(updated)>0){runtime.Count(record,updated);return record;}
    const auto sequence=runtime.Sequence(record);runtime.RemoveRecent(sequence,false);runtime.Erase(slot,sequence);return 0;
}
}
