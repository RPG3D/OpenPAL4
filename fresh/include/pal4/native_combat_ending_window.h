#pragma once
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
namespace pal4 {
class NativeCeguiUiSession;class NativeEventHandlerArray;class NativeScriptEventRouter;
class NativeCombatHints;class INativeSkillSystemServices;class NativeMilesSound2dRuntime;
struct PlayerManagerObjectState;
class NativeCombatReport;
class NativeCombatTaskQueue;
// Owns original5433A0 frame fields, report storage, task queue, four named
// hints and the543F80 controls. Dependencies outlive Close. Prize rendering
// and ending sequences must be bound explicitly; a loaded layout is not one.
class NativeCombatEndingWindow final {
public:
    NativeCombatEndingWindow(NativeCeguiUiSession&,NativeEventHandlerArray&,
        NativeScriptEventRouter&,NativeCombatHints&,PlayerManagerObjectState&,
        INativeSkillSystemServices&,NativeMilesSound2dRuntime&);
    ~NativeCombatEndingWindow();
    NativeCombatEndingWindow(const NativeCombatEndingWindow&)=delete;
    NativeCombatEndingWindow& operator=(const NativeCombatEndingWindow&)=delete;
    std::uint32_t Address()const noexcept;
    std::uint32_t Root()const noexcept;
    bool Visible()const;
    std::uint32_t Word(std::size_t offset)const;
    std::size_t TaskCount()const noexcept;
    NativeCombatTaskQueue& Tasks();
    // Original543F60 narrows double time then updates this frame's queue.
    void Update(double seconds);
    //576440 may hide this pinned frame and reset its live queue synchronously.
    // Storage destruction/rebinding and nested finishes remain prohibited.
    std::uint32_t RunFinish(const std::function<std::uint32_t()>&);
    std::size_t ControlCount()const noexcept;
    class PresentationLease final {
    public:
        ~PresentationLease();
        PresentationLease(const PresentationLease&)=delete;
        PresentationLease& operator=(const PresentationLease&)=delete;
    private:
        friend class NativeCombatEndingWindow;
        explicit PresentationLease(NativeCombatEndingWindow&);
        NativeCombatEndingWindow& owner_;
    };
    PresentationLease BorrowPresentation();
    const NativeCombatReport& Report()const;
    void WritePresentationWord(std::size_t offset,std::uint32_t value);
    void CopyReportTree(unsigned index,const NativeCombatReport& source);
    void CopyReportPlayer(unsigned index,const NativeCombatReport& source);
    std::uint32_t RoleIcon(unsigned slot);
    bool OwnsActivity(std::uint32_t)const noexcept;
    std::uint32_t InvokeVirtual(std::uint32_t,std::uint32_t slot);
    std::uint8_t InvokePredicate(std::uint32_t,std::uint32_t slot,std::uint32_t event);
    void SetVisible(bool,std::uint32_t parent=0u);
    void BindPrizeHandler(std::function<void(std::uint32_t)>);
    void BindInputHandler(std::function<bool(std::uint32_t,bool mouse)>);
    void RethrowMouseError();
    void Close();
private:
    class Impl;std::unique_ptr<Impl> impl_;
};
}
