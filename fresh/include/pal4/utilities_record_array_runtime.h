#pragma once

#include <cstdint>

namespace pal4 {

class IUtilitiesRecordArrayRuntime {
public:
    virtual ~IUtilitiesRecordArrayRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    virtual void Free(std::uint32_t address) = 0;
    virtual void CopyConstruct(std::uint32_t destination,
                               std::uint32_t source,
                               std::uint32_t stride) = 0;
    virtual void Assign(std::uint32_t destination,
                        std::uint32_t source,
                        std::uint32_t stride) = 0;
    virtual void MoveBackward(std::uint32_t begin, std::uint32_t end,
                              std::uint32_t destination_end,
                              std::uint32_t stride) = 0;
    virtual void DestroyRange(std::uint32_t begin, std::uint32_t end,
                              std::uint32_t stride) = 0;
};

}  // namespace pal4
