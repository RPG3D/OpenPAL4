#pragma once

#include "pal4/camera_task_object.h"

#include <cstdint>

namespace pal4 {

class ICameraSequenceTaskRuntime : public ICameraTaskObjectRuntime {
public:
    virtual ~ICameraSequenceTaskRuntime() = default;
    [[nodiscard]] virtual std::uint8_t TransientInitialByte() = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    virtual void FreeMemory(std::uint32_t address) = 0;
};

struct CameraSequenceTiming {
    std::uint32_t first;
    std::uint32_t second;
    std::uint32_t duration;
};

class ICameraSequencePlaybackRuntime : public virtual ICameraSequenceTaskRuntime {
public:
    [[nodiscard]] virtual std::uint32_t EmptyStringAddress() = 0;
    [[nodiscard]] virtual std::uint32_t GetArenaManager() = 0;
    [[nodiscard]] virtual std::uint32_t GetScriptCloudManager() = 0;
    [[nodiscard]] virtual bool LoadCameraFile(
        std::uint32_t manager, std::uint32_t world,
        std::uint32_t scene, std::uint32_t filename) = 0;
    [[nodiscard]] virtual std::uint32_t GetCameraSubsystem() = 0;
    [[nodiscard]] virtual std::uint32_t GetActiveCamera(
        std::uint32_t subsystem) = 0;
    virtual void CleanupCameraTasks(std::uint32_t collection) = 0;
    [[nodiscard]] virtual std::uint32_t FirstCamera(
        std::uint32_t manager) = 0;
    [[nodiscard]] virtual std::uint32_t NextCamera(
        std::uint32_t manager) = 0;
    [[nodiscard]] virtual std::uint32_t CreateCameraTask(
        std::uint32_t descriptor_vtable) = 0;
    virtual void AddCameraTask(std::uint32_t container,
                               std::uint32_t task,
                               CameraSequenceTiming timing) = 0;
    [[nodiscard]] virtual std::uint32_t GetInternalId(
        std::uint32_t active_camera) = 0;
    virtual void MarkCameraProcessed(std::uint32_t collection) = 0;
    [[nodiscard]] virtual std::uint32_t GetActorControl() = 0;
    virtual void ResetActorControl(std::uint32_t control,
                                   std::uint32_t value) = 0;
    virtual void SetCameraMode(std::uint32_t subsystem,
                               std::uint32_t mode) = 0;
    virtual void SetLookAtFromObject(std::uint32_t active_camera,
                                     std::uint32_t flag) = 0;
};

class ICameraSequenceSpawnRuntime : public virtual ICameraSequenceTaskRuntime {
public:
    [[nodiscard]] virtual std::uint32_t AllocateMemory(
        std::uint32_t bytes) = 0;
    [[nodiscard]] virtual std::uint32_t ConstructHelperTask(
        std::uint32_t address) = 0;
    virtual void AddHelperTask(std::uint32_t context,
                               std::uint32_t helper,
                               std::uint32_t first_timing,
                               std::uint32_t second_timing,
                               std::uint32_t third_timing) = 0;
    virtual void SetHelperOwner(std::uint32_t helper,
                                std::uint32_t owner) = 0;
    virtual void NotifyPreviousTask(std::uint32_t task) = 0;
    virtual void MarkProcessed(std::uint32_t context) = 0;
};

class ICameraSequenceInitializeRuntime : public virtual ICameraSequenceTaskRuntime {
public:
    [[nodiscard]] virtual std::uint32_t CStringLength(
        std::uint32_t source) = 0;
    [[noreturn]] virtual void ThrowLength() = 0;
    virtual void SetErrorFlag(std::uint32_t record,
                              std::uint8_t flag) = 0;
    virtual void ReallocateText(std::uint32_t record,
                                std::uint32_t length) = 0;
    virtual void CopyBytes(std::uint32_t destination,
                           std::uint32_t source,
                           std::uint32_t length) = 0;
};

class ICameraQuakeRuntime : public virtual ICameraSequenceTaskRuntime {
public:
    [[nodiscard]] virtual std::uint32_t GetCameraSubsystem() = 0;
    [[nodiscard]] virtual std::uint32_t GetActiveCameraInternalId(
        std::uint32_t subsystem) = 0;
    virtual void InitializeCameraParameters(
        std::uint32_t active, std::uint32_t time,
        std::uint32_t amplitude, std::uint32_t frequency) = 0;
    virtual void SetCameraActive(std::uint32_t active,
                                 std::uint32_t enabled) = 0;
};

class ICameraQuakeSpawnRuntime : public virtual ICameraSequenceSpawnRuntime {
public:
    virtual void AddQuakeTask(std::uint32_t context,
                              std::uint32_t task,
                              std::uint32_t timing_address) = 0;
    virtual void InsertOwnerTask(std::uint32_t collection,
                                 std::uint32_t task) = 0;
};

class ICameraQuakeTerminateSpawnRuntime : public virtual ICameraSequenceSpawnRuntime {
public:
    virtual void AddTerminateTask(std::uint32_t context,
                                  std::uint32_t task,
                                  std::uint32_t timing_address) = 0;
    virtual void InsertOwnerTask(std::uint32_t collection,
                                 std::uint32_t task) = 0;
};

class CameraQuakeTerminateTaskFactory final {
public:
    static constexpr std::uint32_t kDescriptorVtable = 0x8487e4u;
    // IDA 0x595090/0x5950c0.
    [[nodiscard]] static std::uint32_t ConstructDescriptor(
        std::uint32_t descriptor, ICameraSequenceTaskRuntime& runtime);
    [[nodiscard]] static std::uint32_t Create(
        ICameraSequenceSpawnRuntime& runtime);
};

class CameraQuakeTerminateTask final {
public:
    // IDA 0x594d70..0x594e10.
    [[nodiscard]] static std::uint32_t Construct(
        std::uint32_t object, ICameraSequenceTaskRuntime& runtime);
    [[nodiscard]] static std::uint32_t Destruct(
        std::uint32_t object, ICameraSequenceTaskRuntime& runtime);
    [[nodiscard]] static std::uint32_t Delete(
        std::uint32_t object, std::uint8_t flags,
        ICameraSequenceTaskRuntime& runtime);
    [[nodiscard]] static std::uint8_t Enter(ICameraQuakeRuntime& runtime);
    [[nodiscard]] static bool ValidateOwner(
        std::uint32_t object, ICameraSequenceTaskRuntime& runtime);
    [[nodiscard]] static std::uint8_t SpawnFollowup(
        std::uint32_t object, ICameraQuakeTerminateSpawnRuntime& runtime);
};

class CameraQuakeTaskFactory final {
public:
    static constexpr std::uint32_t kDescriptorVtable = 0x8487bcu;
    // IDA 0x594ca0.
    [[nodiscard]] static std::uint32_t ConstructDescriptor(
        std::uint32_t descriptor, ICameraSequenceTaskRuntime& runtime);
    // IDA 0x594ce0.
    [[nodiscard]] static std::uint32_t Create(
        ICameraSequenceSpawnRuntime& runtime);
};

class GomTaskOwnerContext final {
public:
    // IDA 0x594900.
    [[nodiscard]] static std::uint32_t Initialize(
        std::uint32_t object, std::uint32_t owner,
        std::uint32_t context, ICameraSequenceTaskRuntime& runtime);
    // IDA 0x5950a0.
    [[nodiscard]] static std::uint32_t InitializePlain(
        std::uint32_t object, std::uint32_t owner,
        std::uint32_t context, ICameraSequenceTaskRuntime& runtime);
};

class CameraQuakeTask final {
public:
    // IDA 0x594950..0x594a00.
    [[nodiscard]] static std::uint32_t Construct(
        std::uint32_t object, ICameraSequenceTaskRuntime& runtime);
    [[nodiscard]] static std::uint32_t Destruct(
        std::uint32_t object, ICameraSequenceTaskRuntime& runtime);
    [[nodiscard]] static std::uint8_t Enter(
        std::uint32_t object, ICameraQuakeRuntime& runtime);
    [[nodiscard]] static bool ValidateOwner(
        std::uint32_t object, ICameraSequenceTaskRuntime& runtime);
    // IDA 0x594a10.
    [[nodiscard]] static std::uint8_t SpawnFollowup(
        std::uint32_t object, ICameraQuakeSpawnRuntime& runtime);
    // IDA 0x594cb0.
    [[nodiscard]] static std::uint32_t Initialize(
        std::uint32_t object, std::uint32_t owner,
        std::uint32_t time, std::uint32_t amplitude,
        std::uint32_t frequency, std::uint32_t context,
        ICameraSequenceTaskRuntime& runtime);
};

class CameraActiveObject final {
public:
    // IDA 0x5945f0.
    [[nodiscard]] static constexpr std::uint32_t TaskCollection(
        std::uint32_t object) noexcept { return object + 0x10u; }
};

class CameraAutoSeekTask final {
public:
    // IDA 0x594630/0x594650/0x594670.
    [[nodiscard]] static std::uint32_t Construct(
        std::uint32_t object, ICameraSequenceTaskRuntime& runtime);
    [[nodiscard]] static std::uint32_t Destruct(
        std::uint32_t object, ICameraSequenceTaskRuntime& runtime);
    [[nodiscard]] static std::uint32_t Delete(
        std::uint32_t object, std::uint8_t flags,
        ICameraSequenceTaskRuntime& runtime);
    // IDA 0x594680/0x5946d0.
    [[nodiscard]] static std::uint8_t Enter(
        std::uint32_t object, ICameraSequencePlaybackRuntime& runtime);
    [[nodiscard]] static std::uint8_t SpawnFollowup(
        std::uint32_t object, ICameraSequenceSpawnRuntime& runtime);
};

class CameraSequenceTaskFactory final {
public:
    static constexpr std::uint32_t kDescriptorVtable = 0x848770u;
    // IDA 0x5941d0.
    [[nodiscard]] static std::uint32_t ConstructDescriptor(
        std::uint32_t descriptor, ICameraSequenceTaskRuntime& runtime);
};

class CameraSequenceTask final {
public:
    // IDA 0x593fc0/0x594010.
    [[nodiscard]] static std::uint32_t Construct(
        std::uint32_t object, ICameraSequenceTaskRuntime& runtime);
    [[nodiscard]] static std::uint32_t Destruct(
        std::uint32_t object, ICameraSequenceTaskRuntime& runtime);
    // IDA 0x594060/0x5941e0.
    [[nodiscard]] static std::uint8_t Enter(
        std::uint32_t object, ICameraSequencePlaybackRuntime& runtime);
    [[nodiscard]] static std::uint8_t Update(
        std::uint32_t object, ICameraSequencePlaybackRuntime& runtime);
    // IDA 0x594270.
    [[nodiscard]] static std::uint8_t SpawnFollowup(
        std::uint32_t object, ICameraSequenceSpawnRuntime& runtime);
    // IDA 0x5944a0.
    [[nodiscard]] static std::uint32_t Initialize(
        std::uint32_t object, std::uint32_t owner,
        std::uint32_t filename, std::uint32_t autoseek,
        std::uint32_t context, ICameraSequenceInitializeRuntime& runtime);
};

}  // namespace pal4
