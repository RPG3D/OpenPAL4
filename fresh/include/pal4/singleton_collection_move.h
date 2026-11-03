#pragma once

#include <cstdint>

namespace pal4 {

class ISingletonCollectionMoveRuntime {
public:
    virtual ~ISingletonCollectionMoveRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadDword(std::uint32_t address) = 0;
    virtual void WriteDword(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    virtual void Copy(std::uint32_t destination, std::uint32_t source,
                      std::uint32_t size) = 0;
    virtual void Free(std::uint32_t address) = 0;
    virtual void ThrowRange() = 0;
    virtual void DetachAndReset(std::uint32_t element) = 0;
    [[nodiscard]] virtual bool AllocateText(std::uint32_t element,
                                             std::uint32_t length,
                                             std::uint8_t mode) = 0;
    virtual void SetTextSize(std::uint32_t element,
                             std::uint32_t length) = 0;
};

class SingletonCollectionMove final {
public:
    static constexpr std::uint32_t kElementBytes = 28u;
    static constexpr std::uint32_t kEmptyTextAddress = 0x841964u;

    [[nodiscard]] static std::uint32_t Backward(
        std::uint32_t begin, std::uint32_t end,
        std::uint32_t destination_end,
        ISingletonCollectionMoveRuntime& runtime);
    [[nodiscard]] static std::uint32_t CopyElement(
        std::uint32_t destination, std::uint32_t source,
        ISingletonCollectionMoveRuntime& runtime);
    [[nodiscard]] static std::uint32_t CopyConstruct(
        std::uint32_t destination, std::uint32_t source,
        ISingletonCollectionMoveRuntime& runtime);
};

}  // namespace pal4
