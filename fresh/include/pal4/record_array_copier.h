#pragma once

#include <cstdint>

namespace pal4 {

struct RecordArrayState {
    std::uint32_t marker{};
    std::uint32_t begin{};
    std::uint32_t end{};
    std::uint32_t capacity_end{};
};
static_assert(sizeof(RecordArrayState) == 0x10u);

class IRecordArrayCopyRuntime {
public:
    virtual ~IRecordArrayCopyRuntime() = default;
    [[nodiscard]] virtual std::uint32_t Allocate(
        std::uint32_t bytes) = 0;
    [[nodiscard]] virtual std::uint32_t ReadDword(
        std::uint32_t address) = 0;
    virtual void WriteDword(std::uint32_t address,
                            std::uint32_t value) = 0;
};

class RecordArrayCopyRuntimeStub : public IRecordArrayCopyRuntime {
public:
    std::uint32_t Allocate(std::uint32_t) override;
    std::uint32_t ReadDword(std::uint32_t) override;
    void WriteDword(std::uint32_t, std::uint32_t) override;
};

class RecordArrayCopier final {
public:
    // IDA 0x440200.
    [[nodiscard]] static RecordArrayState& CopyTriples(
        RecordArrayState& destination, const RecordArrayState& source,
        IRecordArrayCopyRuntime& runtime);
    // IDA 0x440290.
    [[nodiscard]] static RecordArrayState& CopyPairs(
        RecordArrayState& destination, const RecordArrayState& source,
        IRecordArrayCopyRuntime& runtime);
};

}  // namespace pal4
