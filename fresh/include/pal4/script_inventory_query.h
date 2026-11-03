#pragma once

#include <cstdint>

namespace pal4 {

class IScriptInventoryQueryRuntime {
public:
    virtual ~IScriptInventoryQueryRuntime() = default;
    [[nodiscard]] virtual std::uint32_t Inventory() = 0;
    [[nodiscard]] virtual std::uint32_t FindElement(
        std::uint32_t inventory, std::int32_t id,
        std::uint32_t category) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
};

class ScriptInventoryQuery final {
public:
    // IDA 0x5d6cf0, giCheckPackProperty.
    [[nodiscard]] static bool HasAtLeast(std::int32_t item_id,
                                         std::int32_t required,
                                         IScriptInventoryQueryRuntime& runtime);
    // IDA 0x5d78a0, giGetPropertyNumb.
    [[nodiscard]] static std::int32_t Count(
        std::int32_t item_id, IScriptInventoryQueryRuntime& runtime);
    // IDA 0x5d7fc0, giGetMoney.
    [[nodiscard]] static std::int32_t Money(
        IScriptInventoryQueryRuntime& runtime);
};

}  // namespace pal4
