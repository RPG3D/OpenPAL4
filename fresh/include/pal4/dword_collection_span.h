#pragma once

#include <cstddef>
#include <cstdint>

namespace pal4 {

struct DwordCollectionSpanState {
    std::uint32_t unknown_00{};
    std::uint32_t begin{};
    std::uint32_t end{};
    std::uint32_t capacity{};
};

static_assert(offsetof(DwordCollectionSpanState, begin) == 4u);
static_assert(offsetof(DwordCollectionSpanState, end) == 8u);

class IDwordCollectionSpanRuntime {
public:
    virtual ~IDwordCollectionSpanRuntime() = default;
    virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
};

class DwordCollectionSpanRuntimeStub : public IDwordCollectionSpanRuntime {
public:
    std::uint32_t ReadWord(std::uint32_t) override { return 0; }
    void WriteWord(std::uint32_t, std::uint32_t) override {}
};

class DwordCollectionSpan final {
public:
    static std::int32_t Size(const DwordCollectionSpanState& state) noexcept;
    static std::uint32_t CopyTo(DwordCollectionSpanState& state,
                                std::uint32_t destination,
                                std::uint32_t source,
                                IDwordCollectionSpanRuntime& runtime);
};

}  // namespace pal4
