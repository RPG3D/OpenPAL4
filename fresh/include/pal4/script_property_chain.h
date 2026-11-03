#pragma once

#include <cstdint>

namespace pal4 {

class IScriptPropertyChainRuntime {
public:
    virtual ~IScriptPropertyChainRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(
        std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    virtual void WriteByte(std::uint32_t address,
                           std::uint8_t value) = 0;
    [[nodiscard]] virtual std::uint32_t CallLink(
        std::uint32_t link, std::uint32_t virtual_offset,
        std::int32_t first, std::int32_t second) = 0;
    [[nodiscard]] virtual std::int32_t CallValue(
        std::uint32_t object, std::uint32_t virtual_offset,
        std::int32_t argument) = 0;
};

class ScriptPropertyChain final {
public:
    // IDA 0x56e2a0-0x56e380.
    [[nodiscard]] static std::uint8_t ReturnZero();
    [[nodiscard]] static std::int32_t CountTwo();
    [[nodiscard]] static std::int32_t CountEight();
    [[nodiscard]] static std::uint8_t DestroyBase12(
        std::uint32_t object, IScriptPropertyChainRuntime& runtime);
    [[nodiscard]] static std::uint8_t AllowsAtFour(
        std::uint32_t object, std::int32_t argument,
        IScriptPropertyChainRuntime& runtime);
    [[nodiscard]] static std::uint8_t AllowsAtEight(
        std::uint32_t object, std::int32_t first,
        std::int32_t second, IScriptPropertyChainRuntime& runtime);
    [[nodiscard]] static std::uint8_t AllowsAtTwelve(
        std::uint32_t object, std::int32_t first,
        std::int32_t second, IScriptPropertyChainRuntime& runtime);
    [[nodiscard]] static std::int32_t BroadcastValue(
        std::uint32_t object, std::int32_t argument,
        IScriptPropertyChainRuntime& runtime);
    static void UpdateFromCurrent(
        std::uint32_t object, std::int32_t argument,
        IScriptPropertyChainRuntime& runtime);

    // IDA 0x56e3a0/0x56e440/0x56e480.
    [[nodiscard]] static std::uint8_t AssignLinkFromSlot(
        std::uint32_t object, std::uint32_t source_slot,
        IScriptPropertyChainRuntime& runtime);
    static void PropagateByValue(
        std::uint32_t object, std::int32_t argument,
        std::int32_t value, IScriptPropertyChainRuntime& runtime);
    [[nodiscard]] static std::uint32_t DetachMatchingNext(
        std::uint32_t object, std::int32_t argument,
        IScriptPropertyChainRuntime& runtime);
};

}  // namespace pal4
