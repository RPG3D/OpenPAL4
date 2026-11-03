#pragma once

#include <cstdint>

namespace pal4 {

class IGobMachineTaskFactoryRuntime {
public:
    virtual ~IGobMachineTaskFactoryRuntime() = default;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
};

enum class GobMachineTaskKind {
    Script,
    Rollback,
    GetItem,
    Camera,
    CameraAutoSeek,
    LockControl,
    UnlockControl,
    ActiveObject,
    TouchOther,
};

class IGobMachineTaskCreationRuntime {
public:
    virtual ~IGobMachineTaskCreationRuntime() = default;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t size) = 0;
    virtual void Free(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t ConstructTask(
        GobMachineTaskKind kind, std::uint32_t address) = 0;
};

class GobMachineTaskFactory final {
public:
    [[nodiscard]] static std::uint32_t CreateScript(
        IGobMachineTaskCreationRuntime& runtime);
    [[nodiscard]] static std::uint32_t CreateRollback(
        IGobMachineTaskCreationRuntime& runtime);
    [[nodiscard]] static std::uint32_t CreateGetItem(
        IGobMachineTaskCreationRuntime& runtime);
    [[nodiscard]] static std::uint32_t CreateCamera(
        IGobMachineTaskCreationRuntime& runtime);
    [[nodiscard]] static std::uint32_t CreateCameraAutoSeek(
        IGobMachineTaskCreationRuntime& runtime);
    [[nodiscard]] static std::uint32_t CreateLockControl(
        IGobMachineTaskCreationRuntime& runtime);
    [[nodiscard]] static std::uint32_t CreateUnlockControl(
        IGobMachineTaskCreationRuntime& runtime);
    [[nodiscard]] static std::uint32_t CreateActiveObject(
        IGobMachineTaskCreationRuntime& runtime);
    [[nodiscard]] static std::uint32_t CreateTouchOther(
        IGobMachineTaskCreationRuntime& runtime);
    [[nodiscard]] static std::uint32_t ConstructScript(
        std::uint32_t factory, IGobMachineTaskFactoryRuntime& runtime);
    [[nodiscard]] static std::uint32_t ConstructRollback(
        std::uint32_t factory, IGobMachineTaskFactoryRuntime& runtime);
    [[nodiscard]] static std::uint32_t ConstructGetItem(
        std::uint32_t factory, IGobMachineTaskFactoryRuntime& runtime);
    [[nodiscard]] static std::uint32_t ConstructCamera(
        std::uint32_t factory, IGobMachineTaskFactoryRuntime& runtime);
    [[nodiscard]] static std::uint32_t ConstructCameraAutoSeek(
        std::uint32_t factory, IGobMachineTaskFactoryRuntime& runtime);
    [[nodiscard]] static std::uint32_t ConstructLockControl(
        std::uint32_t factory, IGobMachineTaskFactoryRuntime& runtime);
    [[nodiscard]] static std::uint32_t ConstructUnlockControl(
        std::uint32_t factory, IGobMachineTaskFactoryRuntime& runtime);
    [[nodiscard]] static std::uint32_t ConstructActiveObject(
        std::uint32_t factory, IGobMachineTaskFactoryRuntime& runtime);
    [[nodiscard]] static std::uint32_t ConstructTouchOther(
        std::uint32_t factory, IGobMachineTaskFactoryRuntime& runtime);
};

}  // namespace pal4
