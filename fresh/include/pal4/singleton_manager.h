#pragma once

#include <cstdint>
#include <string>
#include <string_view>

#include "pal4/string_buffer.h"

namespace pal4 {

struct SingletonManagerState {
    std::uint32_t first_word = 0u;
    std::uint32_t begin = 0u;
    std::uint32_t end = 0u;
    std::uint32_t capacity = 0u;
};

class ISingletonManagerRuntime {
public:
    virtual ~ISingletonManagerRuntime() = default;
    virtual void DestroyAll() = 0;
    [[nodiscard]] virtual std::uint32_t ReadDword(std::uint32_t address) = 0;
    virtual void WriteDword(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t ResolveReferenceByte(
        std::uint32_t element, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    virtual void DisposeElement(std::uint32_t element,
                                std::uint32_t value_minus_one,
                                std::uint32_t field_plus_two) = 0;
    [[nodiscard]] virtual std::uint32_t Free(std::uint32_t address) = 0;
};

class ISingletonDestroyRuntime {
public:
    virtual ~ISingletonDestroyRuntime() = default;
    virtual void SortByDescendingId(std::uint32_t begin,
                                    std::uint32_t end) = 0;
    [[nodiscard]] virtual std::uint32_t ReadDword(
        std::uint32_t address) = 0;
    virtual void LogDestroy(std::uint32_t singleton_id,
                            std::uint32_t name_address,
                            const char* source_file,
                            const char* format, std::uint32_t line) = 0;
    virtual void InvokeDestroy(std::uint32_t callback,
                               std::uint32_t argument) = 0;
    [[nodiscard]] virtual std::uint32_t ReleaseTextRecord(
        std::uint32_t element) = 0;
};

struct SingletonRegistrationEntry {
    std::uint32_t status = 0u;
    std::uint32_t text = 0u;
    std::uint32_t text_length = 0u;
    std::uint32_t text_capacity = 0u;
    std::uint32_t singleton_id = 0u;
    std::uint32_t instance = 0u;
    std::uint32_t type_info = 0u;
};

static_assert(sizeof(SingletonRegistrationEntry) == 28u);

class ISingletonRegistrationRuntime : public IStringCompareRangeRuntime {
public:
    virtual ~ISingletonRegistrationRuntime() = default;
    [[nodiscard]] virtual std::string FormatTypeLabel(
        const char* type_name, std::uint32_t factory,
        std::uint32_t type_info) = 0;
    virtual void SetErrorFlag(SingletonRegistrationEntry& entry,
                              std::uint8_t flag) = 0;
    [[nodiscard]] virtual bool AllocateText(SingletonRegistrationEntry& entry,
                                             std::uint32_t length,
                                             std::uint8_t mode) = 0;
    virtual void CopyText(std::uint32_t destination,
                          std::string_view text) = 0;
    virtual void SetTextSize(SingletonRegistrationEntry& entry,
                             std::uint32_t length) = 0;
    [[nodiscard]] virtual std::uint32_t ReadDword(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t Create(std::uint32_t factory) = 0;
    virtual void Insert(std::uint32_t end, std::uint32_t count,
                        const SingletonRegistrationEntry& entry) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(
        std::uint32_t address) override = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    virtual void FreeText(std::uint32_t address) = 0;
};

class ISingletonCollectionCleanupRuntime {
public:
    virtual ~ISingletonCollectionCleanupRuntime() = default;
    virtual void ReleaseTextRecord(std::uint32_t element) = 0;
    virtual void Free(std::uint32_t address) = 0;
};

class SingletonManager final {
public:
    static constexpr std::uint32_t kEmptyLogMessageAddress = 0x841964u;

    explicit SingletonManager(SingletonManagerState& state) : state_(state) {}

    [[nodiscard]] std::uint32_t Initialize(std::uint32_t object_address);
    [[nodiscard]] std::uint32_t Cleanup(ISingletonManagerRuntime& runtime);
    [[nodiscard]] static std::uint32_t CompareBySingletonId(
        std::uint32_t left, std::uint32_t right,
        ISingletonManagerRuntime& runtime);
    [[nodiscard]] std::uint32_t DestroyAll(
        std::uint32_t object_address,
        ISingletonDestroyRuntime& runtime);
    [[nodiscard]] std::uint32_t Register(
        const char* type_name, std::uint32_t singleton_id,
        std::uint32_t factory, std::uint32_t type_info,
        ISingletonRegistrationRuntime& runtime);
    [[nodiscard]] std::uint32_t CleanupCollection(
        ISingletonCollectionCleanupRuntime& runtime);

private:
    SingletonManagerState& state_;
};

struct SingletonManagerGlobalState {
    std::uint8_t initialization_bits = 0u;
    std::uint8_t owns_instance = 0u;
    std::uint32_t instance = 0u;
};

class ISingletonManagerAccessRuntime {
public:
    virtual ~ISingletonManagerAccessRuntime() = default;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    [[nodiscard]] virtual std::uint32_t Initialize(std::uint32_t address) = 0;
    virtual void RegisterExit(std::uint32_t callback_address) = 0;
    virtual void Cleanup(std::uint32_t address) = 0;
    virtual void Free(std::uint32_t address) = 0;
};

class SingletonManagerAccess final {
public:
    static constexpr std::uint32_t kGuardAddress = 0x8e08c4u;
    static constexpr std::uint32_t kOwnedAddress = 0x8e08c8u;
    static constexpr std::uint32_t kInstanceAddress = 0x8e08ccu;
    static constexpr std::uint32_t kExitCallbackAddress = 0x405500u;

    explicit SingletonManagerAccess(SingletonManagerGlobalState& state)
        : state_(state) {}

    [[nodiscard]] std::uint32_t GetInstance(
        ISingletonManagerAccessRuntime& runtime);
    void ExitCleanup(ISingletonManagerAccessRuntime& runtime);

private:
    SingletonManagerGlobalState& state_;
};

}  // namespace pal4
