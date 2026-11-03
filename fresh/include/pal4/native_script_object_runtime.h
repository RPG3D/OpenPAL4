#pragma once

#include "pal4/gom_task_base.h"
#include "pal4/gom_task_container.h"
#include "pal4/script_object.h"
#include "pal4/string_buffer.h"

#include <cstddef>
#include <memory>
#include <optional>
#include <unordered_map>

namespace pal4 {

// Address-backed tasks use owned C++ dispatch services, never original EXE
// virtual addresses. The service remains alive until the task is deleted.
class INativeScriptTaskLifecycle {
public:
    virtual ~INativeScriptTaskLifecycle() = default;
    virtual void Delete(std::uint32_t task, std::uint32_t flags) = 0;
    virtual void Begin(std::uint32_t task) = 0;
    virtual void Tick(std::uint32_t task, float elapsed) = 0;
    [[nodiscard]] virtual bool IsDurationComplete(std::uint32_t task) = 0;
    virtual void Finish(std::uint32_t task) = 0;
    // A derived vtable+20 may replace the ordinary base wait predicate.
    virtual std::optional<bool> IsWaitComplete(std::uint32_t) { return std::nullopt; }
};

class NativeScriptObjectRuntime final : public IScriptObjectRuntime,
                                        public IGomTaskContainerRuntime,
                                        public IGomTaskBaseRuntime {
public:
    explicit NativeScriptObjectRuntime(IStringBufferStorageRuntime& memory)
        : memory_(memory), base_(*this), container_(*this, base_) {}

    void InitializeTaskContainer(std::uint32_t address) override;
    std::uint32_t DestroyTaskContainer(std::uint32_t address) override;
    std::uint32_t ClearTaskContainer(std::uint32_t address);
    std::uint32_t UpdateTaskContainer(std::uint32_t address, float elapsed);
    std::uint32_t BeginWaiting(std::uint32_t address);
    void RegisterTask(std::uint32_t task,
                       std::shared_ptr<INativeScriptTaskLifecycle> lifecycle);
    void AddTask(std::uint32_t container, std::uint32_t task,
                 const char* name, const GomTaskTimingConfig& timing);
    [[nodiscard]] std::size_t RegisteredTasks() const noexcept { return tasks_.size(); }

    std::uint32_t AllocateMemory(std::uint32_t bytes) override;
    std::uint32_t FreeMemory(std::uint32_t address) override;
    std::uint32_t ReadWord(std::uint32_t address) override;
    void WriteWord(std::uint32_t address, std::uint32_t value) override;
    void CopyCString(std::uint32_t destination, const char* source,
                     std::uint32_t bytes) override;
    void DeleteTask(std::uint32_t task, std::uint32_t flags) override;
    GomTaskBaseState& TaskAt(std::uint32_t address) override;
    bool IsWaitComplete(std::uint32_t task) override;
    void BeginRun(std::uint32_t task) override;
    void Tick(std::uint32_t task, float delta) override;
    bool IsDurationComplete(std::uint32_t task) override;
    void Finish(std::uint32_t task) override;

private:
    [[nodiscard]] std::shared_ptr<INativeScriptTaskLifecycle> Lifecycle(
        std::uint32_t task) const;
    IStringBufferStorageRuntime& memory_;
    GomTaskBase base_;
    GomTaskContainer container_;
    std::unordered_map<std::uint32_t, std::shared_ptr<INativeScriptTaskLifecycle>> tasks_;
};

}  // namespace pal4
