#pragma once
#include "pal4/combat_monster_creation.h"
#include "pal4/native_combat_monster_manager.h"
#include "pal4/native_event_document_runtime.h"
#include <functional>
#include <utility>
namespace pal4 {
// Borrows the actual config/manager. Optional follow-up is synchronous and
// preserves each creation's position in the shared original traversal.
class NativeCombatMonsterCreation final : public ICombatMonsterCreationRuntime {
public:
    NativeCombatMonsterCreation(NativeEventDocumentRuntime& d,NativeCombatMonsterManager& m,
        std::function<void(std::uint32_t)> after={}):documents_(d),manager_(m),after_(std::move(after)){}
    std::uint32_t GetChildByIndex(std::uint32_t c,std::uint32_t i) override{return documents_.ChildAt(c,i);}
    std::int32_t GetConfigInt(std::uint32_t c,std::string_view k,std::int32_t fallback) override{
        return documents_.AttributeToInt(documents_.FindNumericAttribute(c,k),fallback);
    }
    void CreateMonster(std::int32_t id,std::uint32_t slot,std::int32_t ab) override{
        const auto a=manager_.CreateMonster(id,slot,static_cast<std::uint32_t>(ab));
        if(after_)after_(a);
    }
private:
    NativeEventDocumentRuntime& documents_;
    NativeCombatMonsterManager& manager_;
    std::function<void(std::uint32_t)> after_;
};
}
