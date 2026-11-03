#pragma once

#include <cstdint>
#include <cstddef>
#include <string_view>

namespace pal4 {

enum class PalPlayerAttributeKind { Player, Basic };

struct PalPlayerAttributeContext {
    std::uint32_t vtable = 0x845560u;
    std::uint32_t reserved = 0u;
    std::uint32_t object = 0u;
};

static_assert(sizeof(PalPlayerAttributeContext) == 12u);
static_assert(offsetof(PalPlayerAttributeContext, object) == 8u);

class IPalConsolePlayerAttributeRuntime {
public:
    virtual ~IPalConsolePlayerAttributeRuntime() = default;
    [[nodiscard]] virtual std::uint32_t PlayerManager() = 0;
    [[nodiscard]] virtual std::int32_t PlayerId(
        std::uint32_t manager, std::string_view name) = 0;
    [[nodiscard]] virtual std::uint32_t PlayerByIndex(
        std::uint32_t manager, std::int32_t id) = 0;
    [[nodiscard]] virtual std::uint32_t AttributeCollection(
        PalPlayerAttributeKind kind) = 0;
    [[nodiscard]] virtual std::uint32_t FindAttribute(
        std::uint32_t collection, std::string_view name) = 0;
    virtual void SetAttribute(const PalPlayerAttributeContext& context,
                              std::uint32_t attribute,
                              std::string_view value) = 0;
    [[nodiscard]] virtual std::string_view GetAttribute(
        const PalPlayerAttributeContext& context,
        std::uint32_t attribute) = 0;
    [[nodiscard]] virtual std::string_view PlayerName(
        std::uint32_t player) = 0;
    virtual void InitializeFormatBuffer(std::string_view name) = 0;
    [[nodiscard]] virtual std::int32_t CleanupFormatBuffer() = 0;
    [[nodiscard]] virtual std::uint32_t Console() = 0;
    [[nodiscard]] virtual std::int32_t AddOutputLine(
        std::uint32_t console, std::string_view text) = 0;
};

class PalConsolePlayerAttributeCommands final {
public:
    // IDA 0x5be470 / 0x5be650 / 0x5be870 / 0x5bea50.
    [[nodiscard]] static std::int32_t GetPlayer(
        const char* command, IPalConsolePlayerAttributeRuntime& runtime);
    [[nodiscard]] static std::int32_t SetPlayer(
        const char* command, IPalConsolePlayerAttributeRuntime& runtime);
    [[nodiscard]] static std::int32_t GetBasic(
        const char* command, IPalConsolePlayerAttributeRuntime& runtime);
    [[nodiscard]] static std::int32_t SetBasic(
        const char* command, IPalConsolePlayerAttributeRuntime& runtime);
};

}  // namespace pal4
