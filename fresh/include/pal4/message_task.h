#pragma once

#include <array>
#include <cstdint>
#include <string_view>

namespace pal4 {

struct MessageTaskState final {
    std::array<std::uint32_t, 23u> words{};
};
static_assert(sizeof(MessageTaskState) == 0x5cu);

class IMessageTaskRuntime {
public:
    virtual ~IMessageTaskRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ResolveFacet(
        std::uint32_t object, std::string_view class_name) = 0;
    [[nodiscard]] virtual std::int32_t ReadConfiguredId(
        std::uint32_t object, std::string_view key,
        std::int32_t default_value) = 0;
    virtual void Send(std::uint32_t facet, std::int32_t id,
                      std::uint32_t text_address) = 0;
};

struct MessageStringTemporary final {
    std::uint32_t data = 0u;
    std::uint32_t length = 0u;
    std::uint32_t capacity = 0u;
    bool aliases_destination = false;
};

class IMessageStringRuntime {
public:
    virtual ~IMessageStringRuntime() = default;
    [[nodiscard]] virtual MessageStringTemporary ReadString(
        std::uint32_t config, std::string_view key,
        std::uint32_t default_address) = 0;
    [[nodiscard]] virtual std::uint8_t RefCount(
        std::uint32_t data) = 0;
    virtual void SetRefCount(std::uint32_t data,
                             std::uint8_t value) = 0;
    [[nodiscard]] virtual std::uint32_t ResetDestination(
        MessageTaskState& state, std::uint32_t offset,
        std::uint32_t flag) = 0;
    virtual void EraseDestination(MessageTaskState& state,
                                  std::uint32_t start,
                                  std::uint32_t count) = 0;
    [[nodiscard]] virtual bool AllocateDestination(
        MessageTaskState& state, std::uint32_t length,
        std::uint32_t mode) = 0;
    virtual void ReallocateDestination(MessageTaskState& state,
                                       std::uint32_t length) = 0;
    virtual void CopyBytes(std::uint32_t destination,
                           std::uint32_t source,
                           std::uint32_t count) = 0;
    virtual void CopyLiteral(std::uint32_t destination,
                             std::string_view text) = 0;
    virtual void WriteNull(std::uint32_t address) = 0;
    virtual void Free(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::string_view ResetText() = 0;
};

class MessageTask final {
public:
    static constexpr std::uint32_t kEmptyTextAddress = 0x841964u;

    // IDA 0x61e700/0x61e770.
    [[nodiscard]] static std::uint8_t Start(
        MessageTaskState& state, IMessageTaskRuntime& runtime);
    [[nodiscard]] static std::uint8_t Reset(MessageTaskState& state);
    // IDA 0x61e780/0x61e8e0.
    [[nodiscard]] static std::uint8_t LoadString(
        MessageTaskState& state, std::uint32_t config,
        IMessageStringRuntime& runtime);
    [[nodiscard]] static std::uint8_t ResetString(
        MessageTaskState& state, IMessageStringRuntime& runtime);
    static constexpr std::uint32_t kEmptyConfigAddress = 0x8e425cu;
};

}  // namespace pal4
