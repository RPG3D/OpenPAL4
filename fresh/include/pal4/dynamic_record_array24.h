#pragma once

#include <cstdint>
#include <unordered_map>
#include <vector>

namespace pal4 {

class IDynamicRecordArray24Memory {
public:
    virtual ~IDynamicRecordArray24Memory() = default;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    virtual void Free(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t ReadDword(std::uint32_t address) const = 0;
    virtual void WriteDword(std::uint32_t address, std::uint32_t value) = 0;
};

class DynamicRecordArray24 final {
public:
    static constexpr std::uint32_t kStride = 24u;

    // The return follows the observed EAX value; reallocation and count zero return 0.
    [[nodiscard]] static std::uint32_t Insert(
        std::uint32_t array, std::uint32_t position,
        std::uint32_t count, std::uint32_t source,
        IDynamicRecordArray24Memory& memory);
};

class DynamicRecordArray24MemoryStub final : public IDynamicRecordArray24Memory {
public:
    [[nodiscard]] std::uint32_t Allocate(std::uint32_t bytes) override;
    void Free(std::uint32_t address) override;
    [[nodiscard]] std::uint32_t ReadDword(std::uint32_t address) const override;
    void WriteDword(std::uint32_t address, std::uint32_t value) override;
    [[nodiscard]] const std::vector<std::uint32_t>& FreedBlocks() const {
        return freed_blocks_;
    }

private:
    std::unordered_map<std::uint32_t, std::uint32_t> words_;
    std::vector<std::uint32_t> freed_blocks_;
    std::uint32_t next_address_ = 0x10000u;
};

}  // namespace pal4
