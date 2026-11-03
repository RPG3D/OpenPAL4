#pragma once

#include "pal4/string_buffer.h"

#include <array>
#include <cstdint>

namespace pal4 {

struct ScriptEventSystemState {
    std::array<std::array<std::uint32_t, 4>, 3> collections{};
    std::uint8_t setup_guard{};
    std::uint8_t population_guard{};
    std::uint8_t error_guard{};
    std::uint8_t error_first_byte{};
    StringBufferState error_text{};
};

struct ScriptEventObjectImage {
    std::array<std::uint8_t, 36> bytes{};
    std::array<bool, 36> written{};
    std::uint32_t base_vtable{};
    std::uint32_t final_vtable{};

    [[nodiscard]] std::uint32_t Dword(std::uint32_t offset) const;
};

struct ScriptIndexedAdapterImage {
    std::uint32_t vtable{};
    std::uint32_t callback{};
    std::uint32_t index{};
};
static_assert(sizeof(ScriptIndexedAdapterImage) == 12u);

class IScriptEventSystemRuntime {
public:
    virtual ~IScriptEventSystemRuntime() = default;
    virtual void RegisterAtExit(std::uint32_t callback) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    virtual void Free(std::uint32_t address) = 0;
    virtual void StoreObject(std::uint32_t address,
                             const ScriptEventObjectImage& image) = 0;
    virtual void AssignName(std::uint32_t object, std::uint32_t source,
                            bool error_string) = 0;
    virtual void InitializeErrorString(StringBufferState& text,
                                       const char* message,
                                       std::uint8_t stack_byte) = 0;
    virtual void StoreAdapter(std::uint32_t address,
                              const ScriptIndexedAdapterImage& image) = 0;
    virtual void AssignAdapter(std::uint32_t object,
                               std::uint32_t member_offset,
                               std::uint32_t adapter, bool& owned) = 0;
    virtual void ReleaseTemporaryAdapter(std::uint32_t adapter,
                                         bool owned) = 0;
    virtual void PushBack(std::uint32_t object) = 0;
    virtual void Sort() = 0;
    virtual void CleanupCollection(std::uint32_t address) = 0;
};

class ScriptEventSystemRuntimeStub final : public IScriptEventSystemRuntime {
public:
    void RegisterAtExit(std::uint32_t callback) override;
    std::uint32_t Allocate(std::uint32_t bytes) override;
    void Free(std::uint32_t address) override;
    void StoreObject(std::uint32_t address,
                     const ScriptEventObjectImage& image) override;
    void AssignName(std::uint32_t object, std::uint32_t source,
                    bool error_string) override;
    void InitializeErrorString(StringBufferState& text,
                               const char* message,
                               std::uint8_t stack_byte) override;
    void StoreAdapter(std::uint32_t address,
                      const ScriptIndexedAdapterImage& image) override;
    void AssignAdapter(std::uint32_t object, std::uint32_t member_offset,
                       std::uint32_t adapter, bool& owned) override;
    void ReleaseTemporaryAdapter(std::uint32_t adapter,
                                 bool owned) override;
    void PushBack(std::uint32_t object) override;
    void Sort() override;
    void CleanupCollection(std::uint32_t address) override;
};

class IScriptEventErrorCleanupRuntime {
public:
    virtual ~IScriptEventErrorCleanupRuntime() = default;
    [[nodiscard]] virtual std::uint8_t ReadByte(
        std::uint32_t address) = 0;
    virtual void WriteByte(std::uint32_t address,
                           std::uint8_t value) = 0;
    virtual void Free(std::uint32_t address) = 0;
};

class ScriptEventErrorCleanupRuntimeStub final
    : public IScriptEventErrorCleanupRuntime {
public:
    std::uint8_t ReadByte(std::uint32_t address) override;
    void WriteByte(std::uint32_t address,
                   std::uint8_t value) override;
    void Free(std::uint32_t address) override;
};

class IScriptIndexedAdapterLifecycleRuntime {
public:
    virtual ~IScriptIndexedAdapterLifecycleRuntime() = default;
    virtual void Free(std::uint32_t address,
                      const ScriptIndexedAdapterImage& image) = 0;
};

class ScriptIndexedAdapterLifecycleRuntimeStub final
    : public IScriptIndexedAdapterLifecycleRuntime {
public:
    void Free(std::uint32_t address,
              const ScriptIndexedAdapterImage& image) override;
};

class ScriptIndexedAdapterLifecycle final {
public:
    static constexpr std::uint32_t kReadBaseVtable = 0x842becu;
    static constexpr std::uint32_t kWriteBaseVtable = 0x842bf4u;

    static void ResetReadBase(ScriptIndexedAdapterImage& image);
    static void ResetWriteBase(ScriptIndexedAdapterImage& image);
    [[nodiscard]] static std::uint32_t DeleteRead(
        ScriptIndexedAdapterImage& image, std::uint32_t object_address,
        std::uint8_t flags, IScriptIndexedAdapterLifecycleRuntime& runtime);
    [[nodiscard]] static std::uint32_t DeleteWrite(
        ScriptIndexedAdapterImage& image, std::uint32_t object_address,
        std::uint8_t flags, IScriptIndexedAdapterLifecycleRuntime& runtime);
    [[nodiscard]] static std::uint32_t DeleteReadBase(
        ScriptIndexedAdapterImage& image, std::uint32_t object_address,
        std::uint8_t flags, IScriptIndexedAdapterLifecycleRuntime& runtime);
    [[nodiscard]] static std::uint32_t DeleteWriteBase(
        ScriptIndexedAdapterImage& image, std::uint32_t object_address,
        std::uint8_t flags, IScriptIndexedAdapterLifecycleRuntime& runtime);
};

class ScriptEventSystem final {
public:
    static constexpr std::uint32_t kCollectionAddress = 0x8e4224u;
    static constexpr std::uint32_t kDescriptorBegin = 0x8ee578u;
    static constexpr std::uint32_t kDescriptorEnd = 0x8ee5d8u;
    static constexpr std::uint32_t kCollectionCleanup = 0x42ec40u;
    static constexpr std::uint32_t kErrorStringCleanup = 0x42f0b0u;

    [[nodiscard]] static ScriptEventObjectImage MakeObject(
        std::uint8_t stack_byte);
    [[nodiscard]] static ScriptIndexedAdapterImage MakeAdapter(
        std::uint32_t callback, std::uint32_t index);
    [[nodiscard]] static std::uint32_t ConstructWriteAdapter(
        ScriptIndexedAdapterImage& image, std::uint32_t object_address,
        std::uint32_t callback, std::uint32_t index);
    [[nodiscard]] static std::uint32_t Initialize(
        ScriptEventSystemState& state, std::uint8_t stack_byte,
        IScriptEventSystemRuntime& runtime,
        std::uint32_t descriptor_begin = kDescriptorBegin,
        std::uint32_t descriptor_end = kDescriptorEnd);
    // IDA 0x57bf40.
    [[nodiscard]] static std::uint32_t GetEnumStringItem(
        std::uint32_t base, std::int32_t index, std::int32_t count,
        ScriptEventSystemState& state,
        IScriptEventSystemRuntime& runtime);
    static void Cleanup(IScriptEventSystemRuntime& runtime);
    static void CleanupErrorString(ScriptEventSystemState& state,
                                   IScriptEventErrorCleanupRuntime& runtime);
};

}  // namespace pal4
