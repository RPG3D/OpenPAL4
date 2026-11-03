#pragma once
#include <cstdint>
#include <string_view>
namespace pal4 {
class ICombatMonsterCreationRuntime {
public:
    virtual ~ICombatMonsterCreationRuntime()=default;
    virtual std::uint32_t GetChildByIndex(std::uint32_t,std::uint32_t)=0;
    virtual std::int32_t GetConfigInt(std::uint32_t,std::string_view,std::int32_t)=0;
    virtual void CreateMonster(std::int32_t,std::uint32_t,std::int32_t)=0;
};
class CombatMonsterCreation final {
public:
    // IDA5756D0: free explicit slots first, deferred children at6 in input order.
    static void Create(std::uint32_t config,std::int32_t child_count,ICombatMonsterCreationRuntime&);
};
}
