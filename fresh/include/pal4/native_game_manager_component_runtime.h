#pragma once

#include "pal4/base_object_lifecycle.h"
#include "pal4/game_manager_components.h"
#include "pal4/script_event_processor.h"
#include "pal4/script_object.h"

namespace pal4 {

class NativeGameManagerComponentRuntime final :
    public IGameManagerComponentRuntime {
public:
    NativeGameManagerComponentRuntime(
        IBaseObjectRuntime& base, IScriptEventProcessorRuntime& processor,
        IScriptObjectRuntime& script, IMonsterBehaviorTreeRuntime& tree)
        : base_(base), processor_(processor), script_(script), tree_(tree) {}

    void ConstructBase(GameManagerComponentState& state,
                       std::uint32_t address) override;
    void DestroyBase(GameManagerComponentState& state,
                     std::uint32_t address) override;
    void InitializeEventProcessor(GameManagerComponentState& state,
                                  std::uint32_t address) override;
    void CleanupEventProcessor(GameManagerComponentState& state,
                               std::uint32_t address) override;
    void InitializeScriptObject(GameManagerComponentState& state,
                                std::uint32_t address) override;
    void CleanupScriptObject(GameManagerComponentState& state,
                             std::uint32_t address) override;
    void InitializeBehaviorTree(GameManagerComponentState& state,
                                std::uint32_t address) override;
    void DestroyBehaviorTree(GameManagerComponentState& state,
                             std::uint32_t address) override;
    IMonsterBehaviorTreeRuntime& TreeRuntime() override { return tree_; }
    void DestroyAuxiliary(GameManagerComponentState& state,
                          std::uint32_t address) override;
    std::uint32_t ClassIdFromGetter(std::uint32_t getter_address) override;
    GameManagerBehaviorInsertResult InsertBehavior(
        std::uint32_t tree, std::uint32_t class_id,
        std::uint32_t factory_address) override;
    void CopyInsertionResult(
        const GameManagerBehaviorInsertResult& result) override;
    void InsertFinalBehavior(std::uint32_t tree, std::uint32_t class_id,
                             std::uint32_t factory_address) override;

private:
    IBaseObjectRuntime& base_;
    IScriptEventProcessorRuntime& processor_;
    IScriptObjectRuntime& script_;
    IMonsterBehaviorTreeRuntime& tree_;
    GameManagerBehaviorInsertResult copied_result_{};
};

}  // namespace pal4
