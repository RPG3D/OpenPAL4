#pragma once

#include <cstdint>

namespace pal4 {

class IDialogEventHandlerMemory {
public:
    virtual ~IDialogEventHandlerMemory() = default;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
};

class DialogEventHandler final {
public:
    static constexpr std::uint32_t kMessageBoxVtable = 0x843ff0u;
    static constexpr std::uint32_t kYesNoVtable = 0x84403cu;

    [[nodiscard]] static std::uint32_t Construct(
        std::uint32_t memory, std::uint32_t callback,
        std::uint32_t argument, std::uint32_t owner,
        IDialogEventHandlerMemory& target);
    [[nodiscard]] static std::uint32_t ConstructAlt(
        std::uint32_t memory, std::uint32_t callback,
        std::uint32_t argument, std::uint32_t owner,
        IDialogEventHandlerMemory& target);
};

}  // namespace pal4
