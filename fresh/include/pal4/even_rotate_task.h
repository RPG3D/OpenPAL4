#pragma once

#include <cstdint>
#include <string_view>

namespace pal4 {

class IEvenRotateTaskRuntime {
public:
    virtual ~IEvenRotateTaskRuntime() = default;
    virtual void BaseConstruct(std::uint32_t object) = 0;
    [[nodiscard]] virtual std::uint32_t BaseDestruct(
        std::uint32_t object) = 0;
    [[nodiscard]] virtual std::uint8_t TransientInitialByte() = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    virtual void WriteByte(std::uint32_t address,
                           std::uint8_t value) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    virtual void FreeMemory(std::uint32_t address) = 0;
    virtual void ConstructNameNode(std::uint32_t address) = 0;
    virtual void ReleaseText(std::uint32_t record) = 0;
    [[nodiscard]] virtual bool AudioIsActive(
        std::uint32_t handle) = 0;
    virtual void AudioPlaySound(std::uint32_t handle) = 0;
    [[nodiscard]] virtual bool EffectNeedsUpdate(
        std::uint32_t handle) = 0;
    virtual void EffectUpdate(std::uint32_t handle) = 0;
    virtual void EffectReset(std::uint32_t handle) = 0;
    [[nodiscard]] virtual std::uint32_t CStringLength(
        std::uint32_t source) = 0;
    [[noreturn]] virtual void ThrowLength() = 0;
    [[nodiscard]] virtual std::uint32_t SetErrorFlag(
        std::uint32_t record, std::uint8_t flag) = 0;
    virtual void ReallocateText(std::uint32_t record,
                                std::uint32_t length) = 0;
    virtual void CopyBytes(std::uint32_t destination,
                           std::uint32_t source,
                           std::uint32_t length) = 0;
};

struct EvenRotateEffectHandle {
    std::uint32_t first;
    std::uint32_t second;
};

struct EvenRotateVector {
    float x;
    float y;
    float z;
};

class IEvenRotatePlaybackRuntime : public IEvenRotateTaskRuntime {
public:
    [[nodiscard]] virtual std::uint32_t BuildSoundPath(
        std::uint32_t name_record, std::string_view suffix) = 0;
    [[nodiscard]] virtual std::uint32_t EmptyStringAddress() = 0;
    [[nodiscard]] virtual std::uint32_t AudioManager() = 0;
    [[nodiscard]] virtual std::uint32_t PlayMusic(
        std::uint32_t manager, std::uint32_t path,
        std::uint32_t parameter) = 0;
    virtual void ReleaseSoundPath(std::uint32_t path) = 0;
    [[nodiscard]] virtual std::uint32_t EffectManager() = 0;
    [[nodiscard]] virtual EvenRotateEffectHandle CreateEffect(
        std::uint32_t manager, std::uint32_t name,
        std::uint32_t position, std::uint32_t parameter,
        bool anchored) = 0;
    virtual void ReleaseTemporaryEffect(EvenRotateEffectHandle handle) = 0;
    virtual void SetEffectVisibility(
        std::uint32_t handle, std::uint32_t visibility) = 0;
    virtual void RotateOwner(std::uint32_t owner,
                             const EvenRotateVector& delta,
                             std::uint32_t mode) = 0;
    [[nodiscard]] virtual std::uint8_t AudioIsPlaying(
        std::uint32_t handle) = 0;
    [[nodiscard]] virtual std::uint8_t AudioStopSound(
        std::uint32_t handle) = 0;
    [[nodiscard]] virtual std::uint8_t AudioPauseSound(
        std::uint32_t handle) = 0;
};

class IEvenRotateSpawnRuntime : public virtual IEvenRotatePlaybackRuntime {
public:
    [[nodiscard]] virtual std::uint32_t AllocateMemory(
        std::uint32_t bytes) = 0;
    virtual void AddReverseTask(std::uint32_t context,
                                std::uint32_t task,
                                std::uint32_t timing_address) = 0;
    virtual void InitializeReverseTask(
        std::uint32_t task, std::uint32_t owner,
        std::uint32_t speed_bits, std::uint32_t sound_name,
        std::uint32_t sound_time, std::uint32_t effect_name,
        std::uint32_t effect_time, std::uint32_t context) = 0;
    virtual void InsertOwnerTask(std::uint32_t collection,
                                 std::uint32_t task) = 0;
    virtual void NotifyPreviousTask(std::uint32_t task) = 0;
    [[nodiscard]] virtual std::uint32_t ConstructHelperTask(
        std::uint32_t address) = 0;
    virtual void AddHelperTask(std::uint32_t context,
                               std::uint32_t helper,
                               std::uint32_t first_timing,
                               std::uint32_t second_timing,
                               std::uint32_t third_timing) = 0;
    virtual void SetHelperOwner(std::uint32_t helper,
                                std::uint32_t owner) = 0;
    virtual void MarkProcessed(std::uint32_t context) = 0;
};

class EvenRotateTaskFactory final {
public:
    static constexpr std::uint32_t kDescriptorVtable = 0x84880cu;
    // IDA 0x5957b0.
    [[nodiscard]] static std::uint32_t ConstructDescriptor(
        std::uint32_t descriptor, IEvenRotateTaskRuntime& runtime);
    // IDA 0x595990.
    [[nodiscard]] static std::uint32_t Create(
        IEvenRotateSpawnRuntime& runtime);
};

class EvenRotateTask final {
public:
    // IDA 0x595150/0x595200.
    [[nodiscard]] static std::uint32_t Construct(
        std::uint32_t object, IEvenRotateTaskRuntime& runtime);
    [[nodiscard]] static std::uint32_t Destruct(
        std::uint32_t object, IEvenRotateTaskRuntime& runtime);
    // IDA 0x595300/0x5954b0.
    [[nodiscard]] static std::uint8_t Enter(
        std::uint32_t object, IEvenRotatePlaybackRuntime& runtime);
    [[nodiscard]] static std::uint8_t Update(
        std::uint32_t object, float delta_seconds,
        IEvenRotatePlaybackRuntime& runtime);
    // IDA 0x595500/0x5957c0/0x5957e0.
    [[nodiscard]] static std::uint8_t SpawnReverseAndFollowup(
        std::uint32_t object, IEvenRotateSpawnRuntime& runtime);
    [[nodiscard]] static std::uint8_t StopSoundIfPlaying(
        std::uint32_t object, IEvenRotatePlaybackRuntime& runtime);
    [[nodiscard]] static std::uint8_t PauseSoundIfNotPlaying(
        std::uint32_t object, IEvenRotatePlaybackRuntime& runtime);
    // IDA 0x595800. Returns the original raw EAX value.
    [[nodiscard]] static std::uint32_t Initialize(
        std::uint32_t object, std::uint32_t owner,
        std::uint32_t speed_bits, std::uint32_t sound_name,
        std::uint32_t sound_time, std::uint32_t effect_name,
        std::uint32_t effect_time, std::uint32_t context,
        IEvenRotateTaskRuntime& runtime);
};

}  // namespace pal4
