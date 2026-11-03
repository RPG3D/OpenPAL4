#pragma once

#include <cstdint>
#include <string_view>

namespace pal4 {

class ICombatConsoleFactoryRuntime {
public:
    virtual ~ICombatConsoleFactoryRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    [[nodiscard]] virtual std::uint32_t ConstructConsole(
        std::uint32_t storage, std::string_view name) = 0;
    [[nodiscard]] virtual std::uint32_t FrameManager() = 0;
    [[nodiscard]] virtual std::uint32_t InsertFactory(
        std::uint32_t factory_tree, std::uint32_t create_callback) = 0;
    virtual void InsertEvent(std::uint32_t event_tree,
                             std::uint32_t event_key,
                             std::uint32_t& callback_record) = 0;
    virtual void DeleteCallback(std::uint32_t callback_record,
                                 std::uint8_t flags) = 0;
};

class CombatConsoleFactory final {
public:
    static constexpr std::uint32_t kFactorySource = 0x8b6910u;
    static constexpr std::uint32_t kFactoryCache = 0x8f95a0u;
    static constexpr std::uint32_t kCreateCallback = 0x54b3a0u;
    static constexpr std::uint32_t kEventKey = 0x8b68e8u;
    static constexpr std::uint32_t kEventCallbackVtable = 0x843494u;
    static constexpr std::uint32_t kEventSentinel = 0x8f959cu;

    // IDA CombatConsoleFactory_GetInstanceThunk at 0x54b380.
    [[nodiscard]] static std::uint32_t GetInstanceThunk(
        ICombatConsoleFactoryRuntime& runtime);
    // IDA CombatConsoleFactory_GetInstance at 0x54b390.
    [[nodiscard]] static std::uint32_t GetInstance(
        ICombatConsoleFactoryRuntime& runtime);
    // IDA CombatConsoleFactory_Create at 0x54b3a0.
    [[nodiscard]] static std::uint32_t Create(
        std::string_view name, ICombatConsoleFactoryRuntime& runtime);
    // IDA CombatConsoleFactory_RegisterThunk at 0x54b400.
    [[nodiscard]] static std::uint32_t RegisterThunk(
        ICombatConsoleFactoryRuntime& runtime);
    // IDA CombatConsoleFactory_Register at 0x54b410.
    [[nodiscard]] static std::uint32_t Register(
        ICombatConsoleFactoryRuntime& runtime);
    // IDA CombatConsoleFactory_RegisterEventThunk at 0x54b430.
    [[nodiscard]] static std::uint32_t RegisterEventThunk(
        ICombatConsoleFactoryRuntime& runtime);
    // IDA CombatConsoleFactory_RegisterEvent at 0x54b440.
    [[nodiscard]] static std::uint32_t RegisterEvent(
        ICombatConsoleFactoryRuntime& runtime);
};

}  // namespace pal4
