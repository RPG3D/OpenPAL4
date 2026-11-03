#pragma once

#include <cstdint>
#include <span>
#include <string_view>

namespace pal4 {

class ICombatScriptEventHandlerRuntime {
public:
    virtual ~ICombatScriptEventHandlerRuntime() = default;
    virtual void RemoveAll(std::uint32_t owner) = 0;
    [[nodiscard]] virtual std::uint32_t GetRouter() = 0;
    [[nodiscard]] virtual std::uint32_t Register(
        std::uint32_t router, std::uint32_t mode,
        std::uint32_t descriptor, std::uint32_t owner,
        std::uint32_t callback) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
};

class CombatScriptEventHandlers final {
public:
    struct Binding {std::uint32_t mode,descriptor,callback;std::string_view name;};
    [[nodiscard]] static std::span<const Binding> Bindings();
    static constexpr std::uint32_t kHandlerCount = 17u;

    // IDA 0x574160. Returns the last registration handle.
    [[nodiscard]] static std::uint32_t Initialize(
        std::uint32_t owner, ICombatScriptEventHandlerRuntime& runtime);
};

}  // namespace pal4
