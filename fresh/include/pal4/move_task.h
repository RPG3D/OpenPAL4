#pragma once

#include <cstdint>
#include <string_view>

namespace pal4 {

struct MoveTaskVector {
    float x;
    float y;
    float z;
};

struct MoveTaskEffectHandle {
    std::uint32_t first;
    std::uint32_t second;
};

class IMoveTaskRuntime {
public:
    virtual ~IMoveTaskRuntime() = default;
    virtual void BaseConstruct(std::uint32_t object) = 0;
    virtual void BaseDestruct(std::uint32_t object) = 0;
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
    [[nodiscard]] virtual std::uint32_t BuildSoundPath(
        std::uint32_t name_record, std::string_view suffix) = 0;
    virtual void ReleaseSoundPath(std::uint32_t path) = 0;
    [[nodiscard]] virtual std::uint32_t EmptyStringAddress() = 0;
    [[nodiscard]] virtual std::uint32_t PlayMusic(
        std::uint32_t path, std::uint32_t parameter) = 0;
    [[nodiscard]] virtual MoveTaskEffectHandle CreateEffect(
        std::uint32_t name, std::uint32_t position,
        std::uint32_t parameter, bool anchored) = 0;
    virtual void ReleaseTemporaryEffect(MoveTaskEffectHandle handle) = 0;
    virtual void SetEffectVisibility(
        std::uint32_t handle, std::uint32_t visibility) = 0;
    [[nodiscard]] virtual float InterpolateFloat(
        float start, float end, float fraction,
        std::uint32_t mode) = 0;
    virtual void SetOwnerPosition(std::uint32_t owner,
                                  const MoveTaskVector& position,
                                  std::uint32_t mode) = 0;
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
    [[nodiscard]] virtual std::uint8_t AudioIsPlaying(
        std::uint32_t handle) = 0;
    [[nodiscard]] virtual std::uint8_t AudioStopSound(
        std::uint32_t handle) = 0;
    [[nodiscard]] virtual std::uint8_t AudioPauseSound(
        std::uint32_t handle) = 0;
};

class IMoveTaskSpawnRuntime : public virtual IMoveTaskRuntime {
public:
    [[nodiscard]] virtual std::uint32_t AllocateMemory(
        std::uint32_t bytes) = 0;
    virtual void AddReverseTask(std::uint32_t context,
                                std::uint32_t task,
                                std::uint32_t timing_address) = 0;
    virtual void InsertOwnerTask(std::uint32_t collection,
                                 std::uint32_t task) = 0;
    virtual void NotifyPreviousTask(std::uint32_t previous) = 0;
    [[nodiscard]] virtual std::uint32_t ConstructHelperTask(
        std::uint32_t memory) = 0;
    virtual void AddHelperTask(std::uint32_t context,
                               std::uint32_t helper,
                               std::uint32_t first_timing,
                               std::uint32_t second_timing,
                               std::uint32_t third_timing) = 0;
    virtual void SetHelperOwner(std::uint32_t helper,
                                std::uint32_t owner) = 0;
    virtual void MarkProcessed(std::uint32_t context) = 0;
};

class MoveTaskFactory final {
public:
    static constexpr std::uint32_t kDescriptorVtable = 0x84887cu;
    // IDA 0x5967d0/0x5969c0.
    [[nodiscard]] static std::uint32_t ConstructDescriptor(
        std::uint32_t descriptor, IMoveTaskRuntime& runtime);
    [[nodiscard]] static std::uint32_t Create(
        IMoveTaskSpawnRuntime& runtime);
};

class MoveTask final {
public:
    static constexpr std::uint32_t kVtable = 0x848858u;
    // IDA 0x596080/0x596140.
    [[nodiscard]] static std::uint32_t Construct(
        std::uint32_t object, IMoveTaskRuntime& runtime);
    static void Destruct(std::uint32_t object,
                         IMoveTaskRuntime& runtime);
    // IDA 0x596250/0x596430.
    [[nodiscard]] static std::uint8_t Enter(
        std::uint32_t object, IMoveTaskRuntime& runtime);
    [[nodiscard]] static std::uint8_t Update(
        std::uint32_t object, float delta_seconds,
        IMoveTaskRuntime& runtime);
    // IDA 0x596500/0x5967e0/0x596800/0x596820.
    [[nodiscard]] static std::uint8_t SpawnReverseAndFollowup(
        std::uint32_t object, IMoveTaskSpawnRuntime& runtime);
    [[nodiscard]] static std::uint8_t StopSoundIfPlaying(
        std::uint32_t object, IMoveTaskRuntime& runtime);
    [[nodiscard]] static std::uint8_t PauseSoundIfNotPlaying(
        std::uint32_t object, IMoveTaskRuntime& runtime);
    [[nodiscard]] static std::uint32_t Initialize(
        std::uint32_t object, std::uint32_t owner,
        std::uint32_t duration_bits, std::uint32_t target_vector,
        std::uint32_t sound_name, std::uint32_t sound_time,
        std::uint32_t effect_name, std::uint32_t effect_time,
        std::uint32_t context, IMoveTaskRuntime& runtime);
};

}  // namespace pal4
