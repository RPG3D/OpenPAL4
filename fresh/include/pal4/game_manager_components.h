#pragma once

#include "pal4/monster_behavior_tree.h"

#include <array>
#include <cstdint>

namespace pal4 {

struct GameManagerComponentState {
    std::array<std::uint32_t, 0x6ecu / 4u> words{};
};
static_assert(sizeof(GameManagerComponentState) == 0x6ecu);

struct GameManagerBehaviorInsertResult {
    std::uint32_t node{};
    bool inserted{};
};

class IGameManagerComponentRuntime {
public:
    virtual ~IGameManagerComponentRuntime() = default;
    virtual void ConstructBase(GameManagerComponentState& state,
                               std::uint32_t address) = 0;
    virtual void DestroyBase(GameManagerComponentState& state,
                             std::uint32_t address) = 0;
    virtual void InitializeEventProcessor(GameManagerComponentState& state,
                                          std::uint32_t address) = 0;
    virtual void CleanupEventProcessor(GameManagerComponentState& state,
                                       std::uint32_t address) = 0;
    virtual void InitializeScriptObject(GameManagerComponentState& state,
                                        std::uint32_t address) = 0;
    virtual void CleanupScriptObject(GameManagerComponentState& state,
                                     std::uint32_t address) = 0;
    virtual void InitializeBehaviorTree(GameManagerComponentState& state,
                                        std::uint32_t address) = 0;
    virtual void DestroyBehaviorTree(GameManagerComponentState& state,
                                     std::uint32_t address) = 0;
    [[nodiscard]] virtual IMonsterBehaviorTreeRuntime& TreeRuntime() = 0;
    virtual void DestroyAuxiliary(GameManagerComponentState& state,
                                  std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t ClassIdFromGetter(
        std::uint32_t getter_address) = 0;
    [[nodiscard]] virtual GameManagerBehaviorInsertResult InsertBehavior(
        std::uint32_t tree, std::uint32_t class_id,
        std::uint32_t factory_address) = 0;
    virtual void CopyInsertionResult(
        const GameManagerBehaviorInsertResult& result) = 0;
    virtual void InsertFinalBehavior(std::uint32_t tree,
                                     std::uint32_t class_id,
                                     std::uint32_t factory_address) = 0;
};

class GameManagerComponentRuntimeStub : public IGameManagerComponentRuntime {
public:
    void ConstructBase(GameManagerComponentState&, std::uint32_t) override;
    void DestroyBase(GameManagerComponentState&, std::uint32_t) override;
    void InitializeEventProcessor(GameManagerComponentState&,
                                  std::uint32_t) override;
    void CleanupEventProcessor(GameManagerComponentState&,
                               std::uint32_t) override;
    void InitializeScriptObject(GameManagerComponentState&,
                                std::uint32_t) override;
    void CleanupScriptObject(GameManagerComponentState&,
                             std::uint32_t) override;
    void InitializeBehaviorTree(GameManagerComponentState&,
                                std::uint32_t) override;
    void DestroyBehaviorTree(GameManagerComponentState&,
                             std::uint32_t) override;
    IMonsterBehaviorTreeRuntime& TreeRuntime() override;
    void DestroyAuxiliary(GameManagerComponentState&,
                          std::uint32_t) override;
    std::uint32_t ClassIdFromGetter(std::uint32_t getter_address) override;
    GameManagerBehaviorInsertResult InsertBehavior(
        std::uint32_t tree, std::uint32_t class_id,
        std::uint32_t factory_address) override;
    void CopyInsertionResult(
        const GameManagerBehaviorInsertResult&) override;
    void InsertFinalBehavior(std::uint32_t tree,
                             std::uint32_t class_id,
                             std::uint32_t factory_address) override;

private:
    MonsterBehaviorTreeRuntimeStub tree_;
};

class GameManagerComponents final {
public:
    static constexpr std::uint32_t kEventProcessorOffset = 0x658u;
    static constexpr std::uint32_t kScriptObjectOffset = 0x688u;
    static constexpr std::uint32_t kTreeOffset = 0x69cu;
    static constexpr std::uint32_t kAuxiliaryOffset = 0x6dcu;

    // IDA 0x439020. incoming_stack_byte models the two uninitialized copies.
    [[nodiscard]] static std::uint32_t Initialize(
        GameManagerComponentState& state, std::uint32_t address,
        std::uint8_t incoming_stack_byte,
        IGameManagerComponentRuntime& runtime);
    // IDA 0x439280.
    static void Cleanup(GameManagerComponentState& state,
                        std::uint32_t address,
                        IGameManagerComponentRuntime& runtime);
    // IDA 0x439850: inserts a classID/factory record into the +0x698 tree.
    [[nodiscard]] static GameManagerBehaviorInsertResult& InsertBehaviorNode(
        GameManagerComponentState& state, std::int32_t class_id,
        std::uint32_t factory, GameManagerBehaviorInsertResult& output,
        IGameManagerComponentRuntime& runtime);
};

}  // namespace pal4
