#pragma once

#include <cstdint>
#include <new>
#include <string>
#include <string_view>

namespace pal4 {

class IActionTaskRuntime {
public:
    virtual ~IActionTaskRuntime() = default;
    virtual void BaseConstruct(std::uint32_t object) = 0;
    [[nodiscard]] virtual std::uint32_t BaseDestruct(
        std::uint32_t object) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    virtual void InitializeEmptyString(std::uint32_t record) = 0;
    virtual void InitializeStringFromCString(std::uint32_t record,
                                              std::uint32_t source) = 0;
    virtual void CleanupString(std::uint32_t record) = 0;
    [[nodiscard]] virtual std::string CurrentAnimationName(
        std::uint32_t animation) = 0;
    [[nodiscard]] virtual std::int32_t CompareCaseInsensitive(
        std::uint32_t action_record, std::string_view current_name) = 0;
    virtual void SetAnimation(std::uint32_t animation,
                              std::uint32_t action_name) = 0;
    [[nodiscard]] virtual std::uint32_t ClearAnimationFlag(
        std::uint32_t animation, std::uint32_t flag) = 0;
    virtual void SetAnimationFlag(std::uint32_t animation,
                                  std::uint32_t flag) = 0;
    [[nodiscard]] virtual bool AnimationFlag(std::uint32_t animation) = 0;
    [[nodiscard]] virtual float AnimationLength(
        std::uint32_t animation) = 0;
    virtual void SetAnimationTime(std::uint32_t animation, float time) = 0;
    [[nodiscard]] virtual std::string ReadStringRecord(
        std::uint32_t record) = 0;
    [[nodiscard]] virtual bool AppendSoundSuffix(
        std::string& path, std::string_view suffix) = 0;
    virtual void PlayMusic(std::string_view path, std::uint32_t mode) = 0;
};

struct ActionTaskTiming {
    std::uint32_t first;
    std::uint32_t second;
    std::uint32_t third;
};

class IActionTaskSpawnRuntime : public IActionTaskRuntime {
public:
    [[nodiscard]] virtual std::uint32_t AllocateMemory(
        std::uint32_t bytes) = 0;
    virtual void FreeMemory(std::uint32_t address) = 0;
    virtual void AddTask(std::uint32_t context, std::uint32_t task,
                         std::uint32_t timing_address) = 0;
    virtual void AddHelperTask(std::uint32_t context,
                               std::uint32_t helper,
                               ActionTaskTiming timing) = 0;
    virtual void InsertCopiedTask(std::uint32_t collection,
                                  std::uint32_t task) = 0;
    virtual void NotifyPreviousTask(std::uint32_t task) = 0;
    [[nodiscard]] virtual std::uint32_t ConstructHelperTask(
        std::uint32_t address) = 0;
    virtual void SetHelperOwner(std::uint32_t helper,
                                std::uint32_t owner) = 0;
    virtual void MarkProcessed(std::uint32_t context) = 0;
};

class ActionTaskFactory final {
public:
    // IDA 0x593930/0x593940/0x593a40/0x593aa0.
    [[nodiscard]] static std::uint32_t ConstructActionDescriptor(
        std::uint32_t descriptor, IActionTaskRuntime& runtime);
    [[nodiscard]] static std::uint32_t ConstructHelperDescriptor(
        std::uint32_t descriptor, IActionTaskRuntime& runtime);
    [[nodiscard]] static std::uint32_t CreateAction(
        IActionTaskSpawnRuntime& runtime);
    template <typename Runtime>
    [[nodiscard]] static std::uint32_t CreateHelper(Runtime& runtime) {
        const auto memory = runtime.AllocateMemory(0x40u);
        if (memory == 0u) return 0u;
        try {
            return runtime.ConstructHelperTask(memory);
        } catch (...) {
            runtime.FreeMemory(memory);
            throw;
        }
    }
};

class ActionTaskObject final {
public:
    // IDA 0x593260/0x5932d0/0x593320.
    [[nodiscard]] static std::uint32_t Construct(
        std::uint32_t object, IActionTaskRuntime& runtime);
    [[nodiscard]] static std::uint32_t DestructBase(
        std::uint32_t object, IActionTaskRuntime& runtime);
    [[nodiscard]] static std::uint32_t Destruct(
        std::uint32_t object, IActionTaskRuntime& runtime);
    // IDA 0x593380/0x593560.
    [[nodiscard]] static std::uint8_t Enter(
        std::uint32_t object, IActionTaskRuntime& runtime);
    [[nodiscard]] static std::uint8_t Update(
        std::uint32_t object, std::int32_t ignored_delta,
        IActionTaskRuntime& runtime);
    // IDA 0x5936a0/0x593950.
    [[nodiscard]] static std::uint8_t SpawnFollowup(
        std::uint32_t object, IActionTaskSpawnRuntime& runtime);
    [[nodiscard]] static std::uint32_t Initialize(
        std::uint32_t object, std::uint32_t owner,
        std::uint32_t repeats, std::uint32_t name,
        std::uint32_t context, IActionTaskRuntime& runtime);
};

}  // namespace pal4
