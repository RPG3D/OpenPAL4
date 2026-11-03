#pragma once

#include <cstdint>
#include <string_view>

namespace pal4 {

class IGameDataAiConfigRuntime {
public:
    virtual ~IGameDataAiConfigRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t GetPropertyString(
        std::uint32_t owner, std::string_view name) = 0;
    [[nodiscard]] virtual std::uint32_t FindEntry(
        std::uint32_t property, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t RegisterFactory(
        std::uint32_t owner, std::string_view name,
        std::uint32_t factory_address) = 0;
    [[nodiscard]] virtual std::uint32_t LookupTable(
        std::uint32_t owner, std::string_view name) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    virtual void LogError(std::string_view message) = 0;
};

class GameDataAiConfig final {
public:
    // IDA 0x4cbec0: resolve AiCommandType then search its entry.
    [[nodiscard]] static std::uint32_t FindCommandType(
        std::uint32_t object, std::uint32_t value,
        IGameDataAiConfigRuntime& runtime);
    // IDA 0x4cbee0: return the third DWORD.
    [[nodiscard]] static std::uint32_t MemberAt8(
        std::uint32_t object, IGameDataAiConfigRuntime& runtime);
    // IDA 0x4cbef0: register AiConfig and AiProperty factories in order.
    [[nodiscard]] static std::uint8_t RegisterFactories(
        std::uint32_t object, IGameDataAiConfigRuntime& runtime);
    // IDA 0x4cbf20: resolve both required tables or log and throw.
    [[nodiscard]] static std::uint8_t InitializeFromDatabase(
        std::uint32_t object, IGameDataAiConfigRuntime& runtime);
};

}  // namespace pal4
