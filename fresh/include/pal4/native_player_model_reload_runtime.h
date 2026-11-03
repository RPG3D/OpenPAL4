#pragma once

#include "pal4/script_process_and_cleanup.h"
#include "pal4/player_manager_object.h"

namespace pal4 {
class NativeBaseObjectMemory;
class NativePkFileManagerRuntime;
class IBaseObjectActivityRuntime;
class IGameResourceManagerRuntime;

// Native boundary for IDA4400E0. The host must provide real actor lifecycle
// effects, including releasing animation bindings before destroying clumps.
// This adapter borrows all services and never substitutes lifecycle stubs.
class NativePlayerModelReloadRuntime final : public IScriptProcessRuntime {
public:
    NativePlayerModelReloadRuntime(PlayerManagerObjectState& players,
        NativeBaseObjectMemory& memory, NativePkFileManagerRuntime& packages,
        IBaseObjectActivityRuntime& activity, IGameResourceManagerRuntime& transforms)
        : players_(players), memory_(memory), packages_(packages),
          activity_(activity), transforms_(transforms) {}
    std::uint32_t ReadDword(std::uint32_t address) override;
    std::uint32_t EmptyStringAddress() override;
    std::int32_t CompareCaseInsensitive(std::uint32_t left, std::uint32_t right) override;
    void CopyName(char* destination, std::uint32_t source, std::uint32_t count) override;
    void ResetObject(std::uint32_t object) override;
    void AddAndProcessScript() override;
    void InitializeObject(std::uint32_t object, const char* name,
        std::uint32_t model, std::uint32_t mode) override;
    void CleanupAllScripts() override;
    void ApplyFirst(std::uint32_t object,
        const std::array<std::uint32_t, 3>& values, std::uint32_t mode) override;
    std::int32_t ApplySecond(std::uint32_t object,
        const std::array<std::uint32_t, 3>& values, std::uint32_t mode) override;
private:
    BaseObjectState& Object(std::uint32_t address);
    PlayerManagerObjectState& players_;
    NativeBaseObjectMemory& memory_;
    NativePkFileManagerRuntime& packages_;
    IBaseObjectActivityRuntime& activity_;
    IGameResourceManagerRuntime& transforms_;
    bool scope_active_{};
};
}  // namespace pal4
