#pragma once

#include <cstdint>
#include <functional>

namespace pal4 {

class IGameDataParameterRecordCopyRuntime {
public:
    virtual ~IGameDataParameterRecordCopyRuntime() = default;
    virtual void CopyBytes(std::uint32_t destination,
                           std::uint32_t source,
                           std::uint32_t bytes) = 0;
    virtual void AssignText(std::uint32_t destination,
                            std::uint32_t source) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
};

class GameDataParameterRecordCopy final {
public:
    // IDA 0x4e3df0: assign one template to every record in [begin, end).
    [[nodiscard]] static std::uint32_t AssignRange(
        std::uint32_t begin, std::uint32_t end, std::uint32_t source,
        IGameDataParameterRecordCopyRuntime& runtime);

    // IDA 0x4e4040: shift a source range backward into destination_end.
    [[nodiscard]] static std::uint32_t AssignBackward(
        std::uint32_t begin, std::uint32_t end,
        std::uint32_t destination_end,
        IGameDataParameterRecordCopyRuntime& runtime);

    // IDA 0x4e4280: copy-construct one record when destination is non-null.
    static void Construct(std::uint32_t destination, std::uint32_t source,
                          IGameDataParameterRecordCopyRuntime& runtime);

    // IDA 0x4e4510: assign one full record and return its destination.
    [[nodiscard]] static std::uint32_t AssignRecord(
        std::uint32_t destination, std::uint32_t source,
        IGameDataParameterRecordCopyRuntime& runtime);

    // IDA 0x4e47e0: retain the first record and each later non-equivalent one.
    [[nodiscard]] static std::uint32_t CopySelected(
        std::uint32_t begin, std::uint32_t end, std::uint32_t destination,
        const std::function<bool(std::uint32_t, std::uint32_t)>& equivalent,
        IGameDataParameterRecordCopyRuntime& runtime);

    static void Assign(std::uint32_t destination, std::uint32_t source,
                       IGameDataParameterRecordCopyRuntime& runtime);
};

}  // namespace pal4
