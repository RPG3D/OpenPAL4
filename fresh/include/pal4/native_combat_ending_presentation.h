#pragma once
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
namespace pal4 {
class NativeCombatEndingWindow;class NativeCombatReport;class NativeCombatPlayerManager;
class NativeCombatActorSkillRuntime;class NativeInventorySession;class NativeGameDatabase;
//545AE0/546770/546D40 over native report ownership, actual first Players and
// original DLL widgets. Sequence starter is a separate required dependency.
class NativeCombatEndingPresentation final {
public:
    using ActorSkills=std::function<NativeCombatActorSkillRuntime&(std::uint32_t)>;
    NativeCombatEndingPresentation(NativeCombatEndingWindow&,NativeCombatPlayerManager&,
        NativeInventorySession&,const NativeGameDatabase&,ActorSkills);
    ~NativeCombatEndingPresentation();
    void Show(const NativeCombatReport&);
    void BindSequenceStarter(std::function<void()>);
    std::size_t PreparedReports()const noexcept;
    std::size_t ActiveCursors()const noexcept;
    void Close();
private:
    class Impl;std::unique_ptr<Impl> impl_;
};
}
