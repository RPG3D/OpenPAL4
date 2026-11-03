#pragma once

#include "pal4/monster_manager_tree_insert.h"
#include "pal4/effect_system_registration.h"

#include <cstdint>

namespace pal4 {

class IEffectSystemAuxiliaryInsertRuntime
    : public IMonsterManagerTreeInsertRuntime {
public:
    ~IEffectSystemAuxiliaryInsertRuntime() override = default;
    [[nodiscard]] virtual EffectRegistrationTree Kind() const = 0;
    [[nodiscard]] std::uint32_t SharedNil() const override;
    [[nodiscard]] std::uint32_t NodeSize() const final;
    [[nodiscard]] std::uint32_t ColorOffset() const final;
    [[nodiscard]] std::int32_t CompareRange(
        std::uint32_t input, std::uint32_t node) final;
    [[nodiscard]] std::int32_t CompareText(
        std::uint32_t first, std::uint32_t second) final;
    void AssignText(std::uint32_t destination,
                    std::uint32_t source) final;
    void SetNodeData(MonsterManagerTreeInsertResult& result,
                     std::uint32_t candidate) final;
};

class EffectSystemAuxiliaryTreeInsert final {
public:
    // IDA 0x63d570/0x63d820: insert at known position in either auxiliary tree.
    [[nodiscard]] static std::uint32_t InsertAt(
        std::uint32_t tree, std::uint32_t result,
        std::uint32_t child, std::uint32_t parent,
        std::uint32_t key_address,
        IEffectSystemAuxiliaryInsertRuntime& runtime);
};

}  // namespace pal4
