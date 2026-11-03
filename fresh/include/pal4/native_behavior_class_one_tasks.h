#pragma once

#include "pal4/native_behavior_atomic_dispatch_runtime.h"
#include "pal4/script_object.h"

#include <memory>

namespace pal4 {
class NativeBaseObjectMemory;
class NativeScriptObjectRuntime;
class NativeBehaviorTasks;

// Required native helper services for the class-one C03 target. Construct
// initializes an allocated 0xE0 image. Accept must leave it unaccepted on
// failure; Discard destroys an unaccepted image without freeing its storage.
// Release removes and destroys an accepted helper through its grid owner.
// Services and memory must outlive the task container, including owner teardown.
class INativeBehaviorClassOneHelpers {
public:
    virtual ~INativeBehaviorClassOneHelpers() = default;
    virtual void Construct(std::uint32_t helper) = 0;
    virtual void Discard(std::uint32_t helper) = 0;
    virtual void Accept(std::uint32_t target, std::uint32_t helper) = 0;
    virtual void Update(std::uint32_t target, std::uint32_t helper) = 0;
    virtual void Release(std::uint32_t target, std::uint32_t helper) = 0;
};

// Creates real task images through the owner's original factory tree and the
// existing ScriptObject/Gom algorithms. Other behavior classes fail explicitly.
class NativeBehaviorClassOneTasks final : public INativeBehaviorTaskFactory {
public:
    NativeBehaviorClassOneTasks(NativeBaseObjectMemory& memory,
        NativeScriptObjectRuntime& tasks, std::uint32_t owner,
        std::uint32_t tree_nil,
        std::shared_ptr<INativeBehaviorClassOneHelpers> helpers);
    ~NativeBehaviorClassOneTasks() override;
    std::uint32_t Create(std::uint32_t tree, std::int32_t class_id,
        std::uint32_t container, const char* name,
        const BehaviorAtomicWaitOptions& options) override;
    void Bind(std::uint32_t task, std::uint32_t owner,
               std::uint32_t record) override;

private:
    std::unique_ptr<NativeBehaviorTasks> tasks_;
};
}  // namespace pal4
