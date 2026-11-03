#pragma once

#include "pal4/gom_task_base.h"
#include "pal4/gom_task_container.h"
#include "pal4/native_base_object_memory.h"

#include <array>
#include <cstdint>
#include <string_view>

namespace pal4 {

class NativeCameraTaskPlayback;

// Owns the task list embedded at CameraSystemObject +0x10.
class NativeCameraTaskContainer final : private IGomTaskContainerRuntime,
                                        private IGomTaskBaseRuntime {
public:
    explicit NativeCameraTaskContainer(NativeCameraTaskPlayback& playback)
        : playback_(playback), base_(*this), container_(*this, base_) {}
    ~NativeCameraTaskContainer() { Destroy(); }

    void Attach(std::uint32_t camera);
    void Clear();
    void Add(std::uint32_t task, std::string_view name,
             const std::array<std::uint32_t, 3>& options);
    void MarkProcessed();
    [[nodiscard]] bool Advance(float elapsed);
    [[nodiscard]] bool Completed() const noexcept;
    void Destroy();
    [[nodiscard]] std::uint32_t Camera() const noexcept { return camera_; }
    [[nodiscard]] std::uint32_t Address() const noexcept {
        return camera_ ? camera_ + 0x10u : 0u;
    }
    [[nodiscard]] std::size_t OutstandingAllocations() const noexcept {
        return memory_.OutstandingAllocations();
    }

private:
    [[nodiscard]] std::uint32_t AllocateMemory(std::uint32_t bytes) override;
    [[nodiscard]] std::uint32_t FreeMemory(std::uint32_t address) override;
    [[nodiscard]] std::uint32_t ReadWord(std::uint32_t address) override;
    void WriteWord(std::uint32_t address, std::uint32_t value) override;
    void CopyCString(std::uint32_t destination, const char* source,
                     std::uint32_t bytes) override;
    void DeleteTask(std::uint32_t task, std::uint32_t flags) override;
    [[nodiscard]] GomTaskBaseState& TaskAt(std::uint32_t address) override;
    [[nodiscard]] bool IsWaitComplete(std::uint32_t task) override;
    void BeginRun(std::uint32_t task) override;
    void Tick(std::uint32_t task, float delta) override;
    [[nodiscard]] bool IsDurationComplete(std::uint32_t task) override;
    void Finish(std::uint32_t task) override;

    NativeCameraTaskPlayback& playback_;
    NativeBaseObjectMemory memory_;
    GomTaskBase base_;
    GomTaskContainer container_;
    GomTaskContainerState* state_{};
    std::uint32_t camera_{};
};

}  // namespace pal4
