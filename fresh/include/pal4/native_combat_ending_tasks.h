#pragma once
#include "pal4/native_combat_task_domain.h"
#include "pal4/combat_ending_task_sequence.h"
#include <cstddef>
#include <functional>
#include <memory>
#include <string_view>
namespace pal4 {
class NativeCombatEndingWindow;class NativeCombatPlayerManager;class NativeCombatActorSkillRuntime;
class NativeMilesSound2dRuntime;struct NativeCombatSystemImage;class NativeUpgradeRecordCatalog;
class NativeCombatTaskQueue;
struct NativeCombatEndingTaskServices {
    std::function<void()> score_text;
    std::function<void(std::uint32_t)> rank_icon;
    std::function<void(std::uint32_t,std::int32_t,std::uint32_t,std::uint32_t,std::uint32_t)> level_up;
    std::function<void(std::uint32_t)> finish;
};
// Original845390 EndTask plus eight factory/GOM child types. Uses the existing
// ScriptTaskQueue/GomTaskContainer algorithms. Borrowed frame/Player/audio/DB
// owners outlive all tasks. Missing visual/level-up/end services fail explicitly.
class NativeCombatEndingTasks final : public INativeCombatTaskDomain {
public:
    using ActorSkills=std::function<NativeCombatActorSkillRuntime&(std::uint32_t)>;
    NativeCombatEndingTasks(NativeCombatEndingWindow&,NativeCombatPlayerManager&,
        NativeMilesSound2dRuntime&,NativeCombatSystemImage&,const NativeUpgradeRecordCatalog&,ActorSkills,
        NativeCombatEndingTaskServices={});
    ~NativeCombatEndingTasks();
    std::uint32_t CreateSequence();
    std::uint32_t CreateChild(CombatEndingTaskType);
    void ConfigureWord(std::uint32_t child,std::size_t offset,std::uint32_t value);
    void ConfigureByte(std::uint32_t child,std::size_t offset,std::uint8_t value);
    std::uint32_t Word(std::uint32_t task,std::size_t offset)const;
    void Add(std::uint32_t parent,std::uint32_t child,std::string_view name,float time);
    void DestroyChild(std::uint32_t);
    bool Owns(std::uint32_t)const noexcept override;
    bool Ready(std::uint32_t)const override;
    bool Enabled(std::uint32_t)const override;
    void SetEnabled(std::uint32_t,bool)override;
    void DisableForReset(std::uint32_t)override;
    std::uint32_t Identity(std::uint32_t)const override;
    bool Processed(std::uint32_t)const override;
    void Prepare(std::uint32_t)override;
    void BeginWaiting(std::uint32_t)override;
    void Update(std::uint32_t,float)override;
    void Stop(std::uint32_t)override;
    std::uint32_t ResultEvent(std::uint32_t)override;
    void Cancel(std::uint32_t)override;
    std::size_t Count()const noexcept;
    std::size_t ChildCount()const noexcept;
    std::size_t OwnedAllocations()const noexcept;
    void Close();
private:
    class Impl;std::unique_ptr<Impl> impl_;
};
}
