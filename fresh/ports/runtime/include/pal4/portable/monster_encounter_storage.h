#pragma once
#include "pal4/monster_behavior_manager.h"
#include "pal4/portable/object_memory.h"
#include <array>
#include <string>
#include <string_view>
#include <vector>

namespace pal4::portable {
// 借用管理器图像和逻辑内存；必须在原怪物成员销毁前 Close。
class MonsterEncounterStorage final {
public:
    MonsterEncounterStorage(MonsterBehaviorManagerState&,ObjectMemory&);
    ~MonsterEncounterStorage();
    MonsterEncounterStorage(const MonsterEncounterStorage&)=delete;
    MonsterEncounterStorage& operator=(const MonsterEncounterStorage&)=delete;
    void Append(const std::array<std::uint32_t,3>&);
    void AddName(std::string_view);
    bool ReleaseHistoryRecord(std::uint32_t record);
    std::string Name(std::uint32_t token) const;
    std::vector<std::array<std::uint32_t,3>> Records();
    std::vector<std::string> Names();
    std::uint32_t ReadWord(std::uint32_t address);
    void ConsumeNames();
    void Close();
private:
    struct NameEntry {std::uint32_t token{},length{};};
    void SyncRecords();
    void SyncNames();
    void Reserve(std::uint32_t& address,std::uint32_t& capacity,std::uint32_t stride,std::uint32_t count);
    void Publish(std::uint32_t offset,std::uint32_t address,std::uint32_t count,std::uint32_t capacity,std::uint32_t stride);
    MonsterBehaviorManagerState& state_;ObjectMemory& memory_;
    std::vector<std::array<std::uint32_t,3>> records_;std::vector<NameEntry> names_;
    std::uint32_t records_address_{},records_capacity_{},names_address_{},names_capacity_{};
    bool closed_{};
};
}
