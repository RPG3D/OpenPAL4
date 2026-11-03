#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace pal4 {

class GomTaskBase;
struct GomTaskBaseState;
struct GomTaskTimingConfig;

struct GomTaskContainerState {
    std::uint8_t opaque_byte{};            // +0
    std::array<std::uint8_t, 3> padding{};
    std::uint32_t sentinel{};              // +4
    std::uint32_t count{};                 // +8
    std::uint8_t ready{};                  // +c
};
static_assert(offsetof(GomTaskContainerState, sentinel) == 4u);
static_assert(offsetof(GomTaskContainerState, ready) == 12u);
static_assert(sizeof(GomTaskContainerState) == 16u);

class IGomTaskContainerRuntime {
public:
    virtual ~IGomTaskContainerRuntime() = default;
    [[nodiscard]] virtual std::uint32_t AllocateMemory(std::uint32_t bytes) = 0;
    [[nodiscard]] virtual std::uint32_t FreeMemory(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    virtual void DeleteTask(std::uint32_t task, std::uint32_t flags) = 0;
    [[nodiscard]] virtual GomTaskBaseState& TaskAt(std::uint32_t address) = 0;
    [[nodiscard]] virtual bool IsWaitComplete(std::uint32_t task) = 0;
    virtual void BeginRun(std::uint32_t task) = 0;
    virtual void Tick(std::uint32_t task, float delta) = 0;
    [[nodiscard]] virtual bool IsDurationComplete(std::uint32_t task) = 0;
    virtual void Finish(std::uint32_t task) = 0;
};

class GomTaskContainer final {
public:
    GomTaskContainer(IGomTaskContainerRuntime& runtime, GomTaskBase& base)
        : runtime_(runtime), base_(base) {}

    // 0x5ecd80: the source byte is an uninitialized stack value in IDA.
    GomTaskContainerState& Initialize(GomTaskContainerState& state,
                                      std::uint8_t source_byte);
    // 0x5ecdb0: clean objects, unlink/free nodes, then free the sentinel.
    [[nodiscard]] std::uint32_t Destroy(GomTaskContainerState& state);
    // 0x5ece40: initialize a task and append its pointer to the list.
    [[nodiscard]] std::uint32_t AddTaskAndInitialize(
        GomTaskContainerState& state, std::uint32_t container_address,
        GomTaskBaseState& task, std::uint32_t task_address,
        const char* name, const GomTaskTimingConfig& timing);
    // 0x5eceb0: put every linked task into phase 2.
    [[nodiscard]] std::uint32_t BeginWaiting(GomTaskContainerState& state);
    // 0x5ecee0: advance task phases using their virtual callbacks.
    [[nodiscard]] std::uint32_t Update(GomTaskContainerState& state,
                                       float delta);
    // 0x5ecfc0: delete task objects and unlink/free nodes, retaining sentinel.
    [[nodiscard]] std::uint32_t CleanupObjects(GomTaskContainerState& state);
    // 0x5ed020: delegate cleanup and set ready again.
    [[nodiscard]] std::uint32_t CleanupAndReady(
        GomTaskContainerState& state);

private:
    IGomTaskContainerRuntime& runtime_;
    GomTaskBase& base_;
};

}  // namespace pal4
