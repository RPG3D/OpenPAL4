#include "pal4/inventory_item_collection.h"
#include "pal4/inventory_item_changes.h"

#include <new>
#include <bit>
#include <stdexcept>

namespace pal4 {

namespace {

void RemoveRecent(const std::uint32_t object,
                  const std::uint32_t sequence,
                  const bool trim_full,
                  IInventoryItemRuntime& runtime) {
    const auto begin = runtime.ReadWord(object + 112u);
    const auto end = runtime.ReadWord(object + 116u);
    const auto index=InventoryRecentEraseIndex(static_cast<std::size_t>((end-begin)/4u),
        [&](std::size_t i){return runtime.ReadWord(begin+static_cast<std::uint32_t>(i)*4u);},sequence,trim_full);
    if(!index)return;
    const auto found=begin+static_cast<std::uint32_t>(*index)*4u;
    for (auto cursor = found + 4u; cursor != end; cursor += 4u)
        runtime.WriteWord(cursor - 4u, runtime.ReadWord(cursor));
    runtime.WriteWord(object + 116u, end - 4u);
}

class ChangesAdapter final:public IInventoryItemChangesRuntime {
    std::uint32_t object;IInventoryItemRuntime& runtime;
public:
    ChangesAdapter(std::uint32_t o,IInventoryItemRuntime& r):object(o),runtime(r){}
    std::uint32_t Definition(std::uint32_t id) override{return runtime.FindGameData(id);}
    std::uint32_t Slot(std::uint32_t definition) override{return InventoryItemCollection::SlotAddress(object,std::bit_cast<std::int32_t>(runtime.ReadWord(definition+4u)),runtime);}
    void Observe(std::uint32_t id) override{runtime.ObserveItem(id);}
    std::uint32_t Find(std::uint32_t slot,std::uint32_t id) override{return InventoryItemCollection::FindIdInSlot(slot,id,runtime);}
    std::uint32_t NextSequence() override{return runtime.ReadWord(object+4u);}
    void NextSequence(std::uint32_t value) override{runtime.WriteWord(object+4u,value);}
    std::uint32_t Append(std::uint32_t slot,std::uint32_t definition,std::uint32_t sequence,std::uint32_t count) override{
        (void)InventoryItemCollection::InsertAt(slot,runtime.ReadWord(slot+8u),definition,sequence,count,runtime);
        const auto begin=runtime.ReadWord(slot+4u),end=runtime.ReadWord(slot+8u);
        if(!begin || std::bit_cast<std::int32_t>(end-begin)/12<=0)InventoryItemCollection::ThrowInvalidElement();
        return end-12u;
    }
    std::uint32_t Count(std::uint32_t record) override{return runtime.ReadWord(record+8u);}
    void Count(std::uint32_t record,std::uint32_t value) override{runtime.WriteWord(record+8u,value);}
    std::uint32_t Sequence(std::uint32_t record) override{return runtime.ReadWord(record+4u);}
    void RemoveRecent(std::uint32_t sequence,bool trim_full) override{::pal4::RemoveRecent(object,sequence,trim_full,runtime);}
    void AppendRecent(std::uint32_t sequence) override{runtime.AppendRecent(object+108u,sequence);}
    void Erase(std::uint32_t slot,std::uint32_t sequence) override{(void)InventoryItemCollection::EraseSequence(slot,std::bit_cast<std::int32_t>(sequence),runtime);}
};

}  // namespace

std::uint32_t InventoryItemCollection::Add(std::uint32_t object,std::uint32_t id,std::int32_t count,IInventoryItemRuntime& runtime){
    ChangesAdapter adapter(object,runtime);return InventoryItemChanges::Add(id,count,adapter);
}
std::uint32_t InventoryItemCollection::Remove(std::uint32_t object,std::uint32_t id,std::int32_t count,IInventoryItemRuntime& runtime){
    ChangesAdapter adapter(object,runtime);return InventoryItemChanges::Remove(id,count,adapter);
}

std::uint32_t InventoryItemCollection::FindBySequence(
    const std::uint32_t object, const std::int32_t sequence,
    const std::uint32_t index, IInventoryItemRuntime& runtime) {
    if (index <= 4u) {
        const auto slot = SlotAddress(object, static_cast<std::int32_t>(index),
                                      runtime);
        return slot == 0u ? 0u :
            FindSequenceInSlot(slot, sequence, runtime);
    }
    for (std::uint32_t slot_index = 0u; slot_index < 5u; ++slot_index) {
        const auto found = FindSequenceInSlot(object + 8u + slot_index * 16u,
                                               sequence, runtime);
        if (found != 0u) return found;
    }
    return 0u;
}

std::uint32_t InventoryItemCollection::FindById(
    const std::uint32_t object, const std::uint32_t id,
    std::uint32_t index, IInventoryItemRuntime& runtime) {
    if (index > 4u) {
        const auto game_data = runtime.FindGameData(id);
        if (game_data != 0u)
            index = runtime.ReadWord(game_data + 4u);
    }
    const auto slot = SlotAddress(object, static_cast<std::int32_t>(index),
                                  runtime);
    return slot == 0u ? 0u : FindIdInSlot(slot, id, runtime);
}

std::int32_t InventoryItemCollection::Count(
    const std::uint32_t object, const std::int32_t index,
    IInventoryItemRuntime& runtime) {
    const auto slot = SlotAddress(object, index, runtime);
    if (slot == 0u) return 0;
    const auto begin = runtime.ReadWord(slot + 4u);
    if (begin == 0u) return 0;
    return static_cast<std::int32_t>(
        runtime.ReadWord(slot + 8u) - begin) / 12;
}

std::uint32_t InventoryItemCollection::AtReverse(
    const std::uint32_t object, const std::int32_t index,
    const std::uint32_t reverse_offset,
    IInventoryItemRuntime& runtime) {
    const auto slot = SlotAddress(object, index, runtime);
    if (slot == 0u) return 0u;
    const auto begin = runtime.ReadWord(slot + 4u);
    if (begin == 0u) return 0u;
    const auto count = (runtime.ReadWord(slot + 8u) - begin) / 12u;
    if (reverse_offset >= count) return 0u;
    const auto offset = count - reverse_offset - 1u;
    if (offset >= count) ThrowInvalidElement();
    return begin + offset * 12u;
}

std::uint32_t InventoryItemCollection::SetRecentKey(
    const std::uint32_t object, const std::int32_t index,
    const std::uint32_t sequence, IInventoryItemRuntime& runtime) {
    runtime.WriteWord(object + 88u +
                          static_cast<std::uint32_t>(index) * 4u,
                      sequence);
    return sequence;
}

std::uint32_t InventoryItemCollection::FindRecent(
    const std::uint32_t object, const std::uint32_t index,
    IInventoryItemRuntime& runtime) {
    const auto sequence = runtime.ReadWord(object + 88u + index * 4u);
    return FindBySequence(object, static_cast<std::int32_t>(sequence),
                          index, runtime);
}

std::uint32_t InventoryItemCollection::SlotAddress(
    const std::uint32_t object, const std::int32_t index,
    IInventoryItemRuntime& runtime) {
    if (index < 0) {
        runtime.LogInvalidSlot(index, 0x103u);
        return 0u;
    }
    if (index >= 5) {
        runtime.LogInvalidSlot(index, 0x10au);
        return 0u;
    }
    return object + 8u + static_cast<std::uint32_t>(index) * 16u;
}

std::uint32_t InventoryItemCollection::FindSequenceInSlot(
    const std::uint32_t slot, const std::int32_t sequence,
    IInventoryItemRuntime& runtime) {
    const auto end = runtime.ReadWord(slot + 8u);
    auto lower = runtime.ReadWord(slot + 4u);
    auto count = static_cast<std::int32_t>(end - lower) / 12;
    while (count > 0) {
        const auto half = count / 2;
        const auto middle = lower + static_cast<std::uint32_t>(half) * 12u;
        if (static_cast<std::int32_t>(runtime.ReadWord(middle + 4u)) <
            sequence) {
            lower = middle + 12u;
            count -= half + 1;
        } else {
            count = half;
        }
    }
    if (lower == end ||
        sequence < static_cast<std::int32_t>(runtime.ReadWord(lower + 4u)))
        return 0u;
    return lower;
}

std::uint32_t InventoryItemCollection::FindIdInSlot(
    const std::uint32_t slot, const std::uint32_t id,
    IInventoryItemRuntime& runtime) {
    const auto end = runtime.ReadWord(slot + 8u);
    for (auto cursor = runtime.ReadWord(slot + 4u); cursor != end;
         cursor += 12u) {
        if (runtime.ReadWord(runtime.ReadWord(cursor)) == id)
            return cursor;
    }
    return 0u;
}

bool InventoryItemCollection::EraseSequence(
    const std::uint32_t slot, const std::int32_t sequence,
    IInventoryItemRuntime& runtime) {
    const auto found = FindSequenceInSlot(slot, sequence, runtime);
    if (found == 0u) return false;
    const auto end = runtime.ReadWord(slot + 8u);
    for (auto source = found + 12u; source != end; source += 12u) {
        for (std::uint32_t field = 0u; field < 12u; field += 4u)
            runtime.WriteWord(source - 12u + field,
                              runtime.ReadWord(source + field));
    }
    runtime.WriteWord(slot + 8u, end - 12u);
    return true;
}

std::uint8_t InventoryItemCollection::DeserializeSlot(
    const std::uint32_t slot, const std::uint32_t stream,
    IInventoryItemStreamRuntime& runtime) {
    auto begin = runtime.ReadWord(slot + 4u);
    auto end = begin;
    auto capacity = runtime.ReadWord(slot + 12u);
    runtime.WriteWord(slot + 8u, begin);
    const auto count = static_cast<std::int32_t>(
        runtime.ReadStreamWord(stream));
    std::uint8_t result = 1u;
    const auto temporary = runtime.TemporaryRecordAddress();
    for (std::int32_t index = 0; index < count; ++index) {
        result = InventoryDbRecordCodec::Read(temporary, stream, runtime) ?
            1u : 0u;
        if (result == 0u) continue;
        const auto game_data = runtime.ReadWord(temporary);
        const auto sequence = runtime.ReadWord(temporary + 4u);
        const auto quantity = runtime.ReadWord(temporary + 8u);
        if (static_cast<std::int32_t>(capacity - end) < 12) {
            const auto used = begin == 0u ? 0u : (end - begin) / 12u;
            const auto growth = used > 1u ? used : 1u;
            const auto new_capacity = used + growth;
            const auto replacement = runtime.AllocateMemory(
                new_capacity * 12u);
            if (replacement == 0u) throw std::bad_alloc();
            for (std::uint32_t old = 0u; old < used; ++old) {
                for (std::uint32_t field = 0u; field < 12u; field += 4u)
                    runtime.WriteWord(replacement + old * 12u + field,
                                      runtime.ReadWord(begin + old * 12u + field));
            }
            runtime.FreeMemory(begin);
            begin = replacement;
            end = replacement + used * 12u;
            capacity = replacement + new_capacity * 12u;
            runtime.WriteWord(slot + 12u, capacity);
            runtime.WriteWord(slot + 4u, begin);
        }
        runtime.WriteWord(end, game_data);
        runtime.WriteWord(end + 4u, sequence);
        runtime.WriteWord(end + 8u, quantity);
        end += 12u;
        runtime.WriteWord(slot + 8u, end);
    }
    return result;
}

std::uint8_t InventoryItemCollection::SerializeSlot(
    const std::uint32_t slot, const std::uint32_t stream,
    IInventoryItemStreamRuntime& runtime) {
    const auto begin = runtime.ReadWord(slot + 4u);
    const auto end = runtime.ReadWord(slot + 8u);
    const auto count = begin == 0u ? 0u :
        static_cast<std::uint32_t>(
            static_cast<std::int32_t>(end - begin) / 12);
    (void)runtime.WriteStreamWord(stream, count);
    for (auto cursor = begin; cursor != end; cursor += 12u)
        (void)InventoryDbRecordCodec::Write(cursor, stream, runtime);
    return 1u;
}

std::uint32_t InventoryItemCollection::InsertAt(
    const std::uint32_t slot, const std::uint32_t position,
    const std::uint32_t game_data, const std::uint32_t sequence,
    const std::uint32_t count, IInventoryItemRuntime& runtime) {
    const auto begin = runtime.ReadWord(slot + 4u);
    const auto end = runtime.ReadWord(slot + 8u);
    const auto capacity = runtime.ReadWord(slot + 12u);
    const auto offset = begin == 0u ? 0u :
        static_cast<std::uint32_t>(
            static_cast<std::int32_t>(position - begin) / 12);
    if (static_cast<std::int32_t>(capacity - end) >= 12) {
        (void)InventoryRecordMemory::CopyBackward(
            position, end, end + 12u, runtime);
        runtime.WriteWord(position, game_data);
        runtime.WriteWord(position + 4u, sequence);
        runtime.WriteWord(position + 8u, count);
        runtime.WriteWord(slot + 8u, end + 12u);
        return position;
    }

    const auto used = begin == 0u ? 0u : (end - begin) / 12u;
    const auto growth = used > 1u ? used : 1u;
    const auto new_capacity = used + growth;
    const auto replacement = runtime.AllocateMemory(new_capacity * 12u);
    if (replacement == 0u) throw std::bad_alloc();
    (void)InventoryRecordMemory::CopyRange(
        begin, position, replacement, runtime);
    const auto inserted = replacement + offset * 12u;
    runtime.WriteWord(inserted, game_data);
    runtime.WriteWord(inserted + 4u, sequence);
    runtime.WriteWord(inserted + 8u, count);
    (void)InventoryRecordMemory::CopyRange(
        position, end, inserted + 12u, runtime);
    runtime.FreeMemory(begin);
    runtime.WriteWord(slot + 12u,
                      replacement + new_capacity * 12u);
    runtime.WriteWord(slot + 4u, replacement);
    runtime.WriteWord(slot + 8u,
                      replacement + (used + 1u) * 12u);
    return inserted;
}

void InventoryItemCollection::ThrowInvalidElement() {
    throw std::out_of_range("invalid vector<T> subscript");
}

}  // namespace pal4
