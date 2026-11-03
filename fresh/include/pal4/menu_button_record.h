#pragma once

#include <cstdint>

namespace pal4 {

class IMenuButtonRecordDataRuntime {
public:
    virtual ~IMenuButtonRecordDataRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    virtual void GrowString(std::uint32_t string,
                            std::uint32_t length) = 0;
    virtual void AssignString(std::uint32_t destination,
                              std::uint32_t source,
                              std::uint32_t offset,
                              std::uint32_t length) = 0;
};

class MenuButtonRecord final {
public:
    static constexpr std::uint32_t kBytes = 164u;
    static constexpr std::uint32_t kInlineCapacity = 32u;
    static constexpr std::uint32_t kNpos = 0xffffffffu;

    // IDA 0x4b50d0. The return is EAX after copying, not the destination.
    [[nodiscard]] static std::uint32_t CopyConstruct(
        std::uint32_t destination, std::uint32_t source,
        IMenuButtonRecordDataRuntime& runtime);
    // IDA 0x4b51c0.
    [[nodiscard]] static std::uint32_t Assign(
        std::uint32_t destination, std::uint32_t source,
        IMenuButtonRecordDataRuntime& runtime);
};

}  // namespace pal4
