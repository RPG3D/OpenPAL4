#pragma once
#include "pal4/native_behavior_atomic_dispatch_runtime.h"
#include "pal4/script_object.h"
#include <memory>
#include <array>

namespace pal4 {
class NativeBaseObjectMemory;
class NativeScriptObjectRuntime;

// Concrete helper/grid/animation services are required. Construct initializes
// the already allocated original helper image (BC/C0/E0 bytes by class).
// Accept must leave it unaccepted on failure; Discard destroys that image but
// does not free it. Release removes/destroys/frees an accepted helper, including
// cancellation. IsComplete is used only by class2. Services must outlive tasks.
class INativeBehaviorHelpers {
public:
    virtual ~INativeBehaviorHelpers() = default;
    virtual bool Supports(std::int32_t kind) const = 0;
    virtual void Construct(std::int32_t kind,std::uint32_t helper) = 0;
    virtual void Discard(std::int32_t kind,std::uint32_t helper) = 0;
    virtual void Accept(std::int32_t kind,std::uint32_t target,std::uint32_t helper) = 0;
    virtual void Update(std::int32_t kind,std::uint32_t target,std::uint32_t helper) = 0;
    virtual void Release(std::int32_t kind,std::uint32_t target,std::uint32_t helper) = 0;
    virtual bool IsComplete(std::int32_t kind,std::uint32_t target,std::uint32_t helper) = 0;
};

class NativeBehaviorTasks final : public INativeBehaviorTaskFactory,
                                  private IScriptEventNodeRuntime {
public:
    NativeBehaviorTasks(NativeBaseObjectMemory& memory,NativeScriptObjectRuntime& tasks,
        std::uint32_t owner,std::uint32_t nil,std::shared_ptr<INativeBehaviorHelpers> helpers);
    ~NativeBehaviorTasks() override;
    std::uint32_t Create(std::uint32_t tree,std::int32_t kind,std::uint32_t container,
        const char* name,const BehaviorAtomicWaitOptions& options) override;
    void Bind(std::uint32_t task,std::uint32_t binder,std::uint32_t record) override;
private:
    class Execution;
    template<int Kind> class TypedExecution;
    std::uint32_t NilAddress() override;
    std::uint32_t ReadDword(std::uint32_t address) override;
    std::uint32_t CallFactory(std::uint32_t function) override;
    void AddTaskAndInitBase(std::uint32_t container,std::uint32_t task,
        std::uint32_t name,std::uint32_t timing) override;
    NativeBaseObjectMemory& memory_;
    NativeScriptObjectRuntime& tasks_;
    std::uint32_t owner_,nil_,pending_{};
    bool creating_{};
    std::array<std::shared_ptr<Execution>,5> executions_;
};
}
