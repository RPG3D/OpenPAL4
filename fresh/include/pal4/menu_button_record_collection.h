#pragma once

#include <cstdint>

namespace pal4 {

class IMenuButtonRecordRuntime {
public:
    virtual ~IMenuButtonRecordRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(
        std::uint32_t bytes) = 0;
    virtual void CopyConstruct(std::uint32_t destination,
                               std::uint32_t source) = 0;
    virtual void Assign(std::uint32_t destination,
                        std::uint32_t source) = 0;
    virtual void Cleanup(std::uint32_t record) = 0;
    virtual void Free(std::uint32_t pointer) = 0;
};

class MenuButtonRecordCollection final {
public:
    static constexpr std::uint32_t kRecordBytes = 164u;

    // IDA 0x4b4d20.
    [[nodiscard]] static std::uint32_t Clear(
        std::uint32_t collection, IMenuButtonRecordRuntime& runtime);
    // IDA 0x4b4d60. Its incidental EAX value is not a collection result.
    static void Insert(std::uint32_t collection,
                       std::uint32_t insertion,
                       std::uint32_t count,
                       std::uint32_t source,
                       IMenuButtonRecordRuntime& runtime);
};

}  // namespace pal4
