#pragma once

#include <cstdint>

namespace pal4 {

class ISoundEventTreeRuntime {
public:
    virtual ~ISoundEventTreeRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    [[nodiscard]] virtual std::uint32_t Free(std::uint32_t address) = 0;
    [[nodiscard]] virtual bool Less(std::uint32_t left,
                                     std::uint32_t right) = 0;
    virtual void CopyPayload(std::uint32_t destination,
                             std::uint32_t source) = 0;
};

class SoundEventTree final {
public:
    static constexpr std::uint32_t kNilAddress = 0x8edfc8u;
    static constexpr std::uint32_t kNilReferencesAddress = 0x8edfc4u;
    static constexpr std::uint32_t kNodeBytes = 184u;

    // IDA 0x4c3bd0 and 0x4c4010.
    [[nodiscard]] static std::uint32_t Construct(
        std::uint32_t tree, ISoundEventTreeRuntime& runtime);
    [[nodiscard]] static std::uint32_t AllocateNode(
        std::uint32_t parent, std::uint32_t colour,
        ISoundEventTreeRuntime& runtime);
    // IDA 0x4c3930 and 0x4c3c60.
    [[nodiscard]] static std::uint32_t InsertOrGet(
        std::uint32_t tree, std::uint32_t output,
        std::uint32_t key, ISoundEventTreeRuntime& runtime);
    [[nodiscard]] static std::uint32_t InsertNode(
        std::uint32_t tree, std::uint32_t output,
        std::uint32_t child, std::uint32_t parent,
        std::uint32_t key, ISoundEventTreeRuntime& runtime);
    // IDA 0x4c3f00, 0x4c3f50, 0x4c3fb0 and 0x4c4040.
    [[nodiscard]] static std::uint32_t LowerBound(
        std::uint32_t tree, std::uint32_t key,
        ISoundEventTreeRuntime& runtime);
    [[nodiscard]] static std::uint32_t RotateLeft(
        std::uint32_t tree, std::uint32_t node,
        ISoundEventTreeRuntime& runtime);
    [[nodiscard]] static std::uint32_t RotateRight(
        std::uint32_t tree, std::uint32_t node,
        ISoundEventTreeRuntime& runtime);
    [[nodiscard]] static std::uint32_t Previous(
        std::uint32_t cursor, ISoundEventTreeRuntime& runtime);

private:
    [[nodiscard]] static std::uint32_t InsertCore(
        std::uint32_t tree, std::uint32_t child,
        std::uint32_t parent, std::uint32_t key,
        ISoundEventTreeRuntime& runtime);
};

}  // namespace pal4
