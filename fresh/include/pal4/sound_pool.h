#pragma once

#include "pal4/audio_file.h"

#include <cstddef>
#include <cstdint>

namespace pal4 {

struct SoundPoolState final {
    std::uint8_t unknown0[4]{};
    std::uint32_t tree_root = 0u;
    std::uint8_t unknown8[4]{};
    std::uint32_t tree_count = 0u;
    std::uint8_t control_flag = 0u;
    std::uint8_t unknown17[3]{};
};
static_assert(sizeof(SoundPoolState) == 20u);
static_assert(offsetof(SoundPoolState, tree_root) == 4u);
static_assert(offsetof(SoundPoolState, tree_count) == 12u);
static_assert(offsetof(SoundPoolState, control_flag) == 16u);

struct SoundPoolTreeGlobals final {
    std::uint32_t sentinel = 0u;
    std::uint32_t users = 0u;
};

struct SoundPoolEntryState final {
    std::uint32_t crc = 0u;
    std::uint8_t owns_file = 0u;
    std::uint8_t unknown5[3]{};
    std::uint32_t file = 0u;
};
static_assert(sizeof(SoundPoolEntryState) == 12u);
static_assert(offsetof(SoundPoolEntryState, file) == 8u);

class ISoundPoolObjectRuntime {
public:
    virtual ~ISoundPoolObjectRuntime() = default;
    [[nodiscard]] virtual SoundPoolTreeGlobals& TreeGlobals() = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    [[nodiscard]] virtual std::uint32_t Free(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address, std::uint32_t offset) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t offset,
                           std::uint32_t value) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint32_t offset,
                           std::uint8_t value) = 0;
    virtual void FreeNodesRecursive(std::uint32_t node) = 0;
    virtual void CleanupNodeValue(std::uint32_t value_address) = 0;
    [[nodiscard]] virtual std::uint32_t NextNode(std::uint32_t node) = 0;
    virtual void RemoveNode(std::uint32_t pool, std::uint32_t node) = 0;
    virtual void FirstElement(std::uint32_t pool) = 0;
};

using SoundPoolFactory = std::uint32_t (*)(ISoundPoolObjectRuntime&);
using SoundPoolDeleter = std::uint32_t (*)(std::uint32_t,
                                           ISoundPoolObjectRuntime&);

class ISoundPoolSingletonRuntime {
public:
    virtual ~ISoundPoolSingletonRuntime() = default;
    virtual void GetRegistryInstance() = 0;
    [[nodiscard]] virtual std::uint32_t Register(
        const char* name, std::uint32_t identifier,
        SoundPoolFactory factory, SoundPoolDeleter deleter) = 0;
};

class ISoundPoolEntryRuntime : public IAudioFileDestroyRuntime {
public:
    [[nodiscard]] virtual AudioFileState& FileAt(std::uint32_t address) = 0;
};

class ISoundPoolAddRuntime : public ISoundPoolEntryRuntime {
public:
    [[nodiscard]] virtual std::uint32_t Root(std::uint32_t pool) = 0;
    [[nodiscard]] virtual std::uint32_t FindNode(
        std::uint32_t pool, std::uint32_t crc) = 0;
    [[nodiscard]] virtual std::uint32_t NodeFile(std::uint32_t node) = 0;
    [[nodiscard]] virtual std::uint32_t AllocateFile(
        std::uint32_t bytes) = 0;
    [[nodiscard]] virtual bool ControlEnabled(std::uint32_t pool) = 0;
    virtual void InsertSoundObject(std::uint32_t pool, std::uint32_t crc,
                                   std::uint32_t file) = 0;
    virtual void DestroyTemporary(std::uint32_t file,
                                  std::uint32_t flags) = 0;
    virtual void IncrementReference(std::uint32_t file) = 0;
};

class ISoundPoolReleaseRuntime {
public:
    virtual ~ISoundPoolReleaseRuntime() = default;
    [[nodiscard]] virtual std::uint32_t Root(std::uint32_t pool) = 0;
    [[nodiscard]] virtual std::uint32_t FirstNode(std::uint32_t pool) = 0;
    [[nodiscard]] virtual std::uint32_t Count(std::uint32_t pool) = 0;
    [[nodiscard]] virtual std::uint32_t FindNode(
        std::uint32_t pool, std::uint32_t crc) = 0;
    [[nodiscard]] virtual std::uint32_t NodeCrc(std::uint32_t node) = 0;
    [[nodiscard]] virtual std::uint32_t NodeFile(std::uint32_t node) = 0;
    [[nodiscard]] virtual AudioFileState& FileAt(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t NextNode(std::uint32_t node) = 0;
    [[nodiscard]] virtual std::uint32_t RemoveNode(
        std::uint32_t pool, std::uint32_t node) = 0;
};

class SoundPoolEntry final {
public:
    // IDA 0x65a8d0: conditionally cleanup and free an owned audio file.
    [[nodiscard]] static std::uint8_t Cleanup(
        const SoundPoolEntryState& entry,
        ISoundPoolEntryRuntime& runtime);
};

struct SoundPoolSingletonState final {
    std::uint8_t guard = 0u;
    std::uint32_t instance = 0u;
};

class SoundPool final {
public:
    static constexpr std::uint32_t kObjectSize = 20u;
    static constexpr std::uint32_t kTreeNodeSize = 28u;
    // IDA 0x65a4a0: register auSoundPool under singleton id 1005 once.
    [[nodiscard]] static std::uint32_t GetInstance(
        SoundPoolSingletonState& state,
        ISoundPoolSingletonRuntime& runtime);
    // IDA 0x65a4e0: allocate pool object, shared sentinel and tree root.
    [[nodiscard]] static std::uint32_t Factory(
        ISoundPoolObjectRuntime& runtime);
    // IDA 0x65a5d0: destroy and free a non-null singleton instance.
    [[nodiscard]] static std::uint32_t DestroyInstance(
        std::uint32_t address, ISoundPoolObjectRuntime& runtime);
    // IDA 0x65a5f0: release pool tree and shared sentinel ownership.
    [[nodiscard]] static std::uint32_t Destroy(
        std::uint32_t address, ISoundPoolObjectRuntime& runtime);
    // IDA 0x65b5b0: second emitted entry for the same tree cleanup sequence.
    [[nodiscard]] static std::uint32_t CleanupTree(
        std::uint32_t address, ISoundPoolObjectRuntime& runtime);
    // IDA 0x65a6b0: clear tree values and nodes while retaining the root.
    [[nodiscard]] static std::uint32_t ClearElements(
        std::uint32_t address, ISoundPoolObjectRuntime& runtime);
    // IDA 0x65a750: retain an existing file or load and optionally index one.
    [[nodiscard]] static std::uint32_t AddAudioFile(
        std::uint32_t pool, const char* path,
        ISoundPoolAddRuntime& runtime);
    // IDA 0x65a900: release a CRC-keyed file and maybe evict one node.
    [[nodiscard]] static std::uint32_t ReleaseAudioFile(
        std::uint32_t pool, std::uint32_t crc,
        ISoundPoolReleaseRuntime& runtime);
    // IDA 0x65a980: select and remove one expired file node.
    [[nodiscard]] static std::uint32_t CleanupExpired(
        std::uint32_t pool, ISoundPoolReleaseRuntime& runtime);
    // IDA 0x65a9f0: update control byte and clear on disable.
    [[nodiscard]] static std::uint8_t SetControlFlag(
        std::uint32_t pool, std::uint8_t enabled,
        ISoundPoolObjectRuntime& runtime);
};

}  // namespace pal4
