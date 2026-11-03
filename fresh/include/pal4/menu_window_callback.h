#pragma once

#include <cstdint>

namespace pal4 {

class IMenuWindowCallbackRuntime {
public:
    virtual ~IMenuWindowCallbackRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(
        std::uint32_t bytes) = 0;
};

class MenuWindowCallback final {
public:
    static constexpr std::uint32_t kBytes = 24u;
    static constexpr std::uint32_t kBaseVtable = 0x843500u;
    static constexpr std::uint32_t kActionVtable = 0x844294u;
    static constexpr std::uint32_t kButtonVtable = 0x84424cu;

    // IDA 0x4b5200.
    [[nodiscard]] static std::uint32_t ConstructAction(
        std::uint32_t memory, std::uint32_t owner,
        std::uint32_t argument, std::uint32_t function,
        IMenuWindowCallbackRuntime& runtime);
    // IDA 0x4b5220.
    [[nodiscard]] static std::uint32_t CloneAction(
        std::uint32_t source, IMenuWindowCallbackRuntime& runtime);
    // IDA 0x4b5260.
    [[nodiscard]] static std::uint32_t ConstructButton(
        std::uint32_t memory, std::uint32_t owner,
        std::uint32_t argument, std::uint32_t function,
        IMenuWindowCallbackRuntime& runtime);
};

}  // namespace pal4
