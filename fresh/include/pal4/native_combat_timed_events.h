#pragma once
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <string_view>
namespace pal4 {
// CombatSystem+412 borrows this owner's existing native dual callback queue.
// Combat storage and callback services must outlive the owner.
class NativeCombatTimedEvents final {
public:
    using Dispatch=std::function<void(std::uint32_t owner,std::uint32_t method)>;
    using ValueDispatch=std::function<void(std::uint32_t owner,std::uint32_t method,std::uint32_t result)>;
    NativeCombatTimedEvents(std::uint32_t combat,std::uint32_t& slot,Dispatch,ValueDispatch={});
    void ScheduleEnd(std::string_view name,float delay,std::uint32_t owner,std::uint32_t result);
    ~NativeCombatTimedEvents();
    NativeCombatTimedEvents(const NativeCombatTimedEvents&)=delete;
    NativeCombatTimedEvents& operator=(const NativeCombatTimedEvents&)=delete;
    void Schedule(std::string_view name,float delay,std::uint32_t owner,std::uint32_t method);
    void Repeat(std::string_view name,float period,std::uint32_t owner,std::uint32_t method);
    std::uint32_t CancelEvent(std::string_view name);
    std::uint32_t CancelTimer(std::string_view name);
    void Update(float elapsed);
    // Start5756D0's collection reset preserves actual member412 and nodes.
    void ReleaseCallbacks();
    void Close();
    std::uint32_t Address() const;
    std::uint32_t EventCount() const;
    std::uint32_t TimerCount() const;
    std::size_t OutstandingAllocations() const;
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};
}
