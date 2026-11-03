#pragma once
#include <cstdint>
namespace pal4 {
// Typed owner boundary for ScriptTaskQueue virtual calls. The queue borrows
// domains; each domain owns its parent images/children and validates addresses.
// All operations are required: death-specific no-ops must not become attack
// defaults. ResultEvent may transfer a new task from any bound domain.
class INativeCombatTaskDomain {
public:
    virtual ~INativeCombatTaskDomain()=default;
    virtual bool Owns(std::uint32_t task) const noexcept=0;
    virtual bool Ready(std::uint32_t task) const=0;
    virtual bool Enabled(std::uint32_t task) const=0;
    virtual void SetEnabled(std::uint32_t task,bool enabled)=0;
    // ResetFlags only clears this byte while Update may be dispatching. A
    // domain whose own Update can synchronously reset its queue binds this
    // narrow operation; other domains keep their ordinary mutation guard.
    virtual void DisableForReset(std::uint32_t task){SetEnabled(task,false);}
    virtual std::uint32_t Identity(std::uint32_t task) const=0;
    virtual bool Processed(std::uint32_t task) const=0;
    virtual void Prepare(std::uint32_t task)=0;
    virtual void BeginWaiting(std::uint32_t task)=0;
    virtual void Update(std::uint32_t task,float delta)=0;
    virtual void Stop(std::uint32_t task)=0;
    virtual std::uint32_t ResultEvent(std::uint32_t task)=0;
    virtual void Cancel(std::uint32_t task)=0;
};
}
