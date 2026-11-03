#pragma once

#include "pal4/pal_movement_secondary_child.h"

#include <cstdint>

namespace pal4 {

class IPalMovementSecondaryCollectionRuntime {
public:
    virtual ~IPalMovementSecondaryCollectionRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t GetClump(std::uint32_t element) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    virtual void Free(std::uint32_t address) = 0;
    virtual void Leave(std::uint32_t element) = 0;
};

class PalMovementSecondaryCollectionRuntimeStub final
    : public IPalMovementSecondaryCollectionRuntime {
public:
    std::uint32_t ReadWord(std::uint32_t address) override;
    void WriteWord(std::uint32_t address, std::uint32_t value) override;
    std::uint32_t GetClump(std::uint32_t element) override;
    std::uint32_t Allocate(std::uint32_t bytes) override;
    void Free(std::uint32_t address) override;
    void Leave(std::uint32_t element) override;
};

}  // namespace pal4
