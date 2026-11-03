#pragma once

#include "pal4/monster_manager_access.h"

#include <cstdint>

namespace pal4 {

class IMonsterManagerCleanupRuntime {
public:
    virtual ~IMonsterManagerCleanupRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadDword(std::uint32_t address) = 0;
    virtual void WriteDword(std::uint32_t address, std::uint32_t value) = 0;
    virtual void CallMonsterSlot2c(std::uint32_t monster) = 0;
    virtual void DeleteMonster(std::uint32_t monster,
                               std::uint32_t deleting_flag) = 0;
    virtual void CleanupTreeNodes(std::uint32_t tree,
                                  std::uint32_t root) = 0;
    virtual void ReleaseText(std::uint32_t text,
                             std::uint32_t argument) = 0;
    virtual void FreeNode(std::uint32_t node) = 0;
    [[nodiscard]] virtual std::uint32_t NextTreeNode(
        std::uint32_t node) = 0;
    virtual void DeleteTreeNode(std::uint32_t tree,
                                std::uint32_t node) = 0;
    virtual void CleanupAuxiliaryTree(std::uint32_t manager) = 0;
    virtual void ClearGridElements(std::uint32_t grid) = 0;
    virtual void GetPreviousGridElement(std::uint32_t grid) = 0;
    [[nodiscard]] virtual std::uint32_t ResetGrid(std::uint32_t grid) = 0;
};

class MonsterManagerCleanupRuntimeStub : public IMonsterManagerCleanupRuntime {
public:
    std::uint32_t ReadDword(std::uint32_t) override;
    void WriteDword(std::uint32_t, std::uint32_t) override;
    void CallMonsterSlot2c(std::uint32_t) override;
    void DeleteMonster(std::uint32_t, std::uint32_t) override;
    void CleanupTreeNodes(std::uint32_t, std::uint32_t) override;
    void ReleaseText(std::uint32_t, std::uint32_t) override;
    void FreeNode(std::uint32_t) override;
    std::uint32_t NextTreeNode(std::uint32_t) override;
    void DeleteTreeNode(std::uint32_t, std::uint32_t) override;
    void CleanupAuxiliaryTree(std::uint32_t) override;
    void ClearGridElements(std::uint32_t) override;
    void GetPreviousGridElement(std::uint32_t) override;
    std::uint32_t ResetGrid(std::uint32_t) override;
};

class MonsterManagerCleanup final {
public:
    [[nodiscard]] static std::uint32_t Run(
        std::uint32_t manager, const MonsterManagerTreeGlobals& globals,
        IMonsterManagerCleanupRuntime& runtime);
};

}  // namespace pal4
