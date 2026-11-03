#pragma once

#include <cstddef>
#include <cstdint>

namespace pal4 {

struct PalObjectManagerIterator final {
    std::uint32_t first = 0u;
    std::uint32_t second = 0u;
};

static_assert(sizeof(PalObjectManagerIterator) == 8);

class IPalObjectManagerTraversalRuntime {
public:
    virtual ~IPalObjectManagerTraversalRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t IsPropertyEmpty(
        std::uint32_t object) = 0;
    virtual void InitializeIterator(PalObjectManagerIterator& source,
                                    std::uint32_t collection) = 0;
    virtual void WrapIterator(PalObjectManagerIterator& destination,
                              PalObjectManagerIterator& source) = 0;
    [[nodiscard]] virtual std::uint32_t IsEnd(
        PalObjectManagerIterator& iterator) = 0;
    [[nodiscard]] virtual std::uint32_t GetInstance(
        PalObjectManagerIterator& iterator) = 0;
    virtual void Invoke(std::uint32_t target, std::uint32_t slot,
                        std::uint32_t argument) = 0;
    virtual void Remove(PalObjectManagerIterator& iterator,
                        std::uint32_t flag) = 0;
};

class PalObjectManagerTraversalRuntimeStub final
    : public IPalObjectManagerTraversalRuntime {
public:
    [[nodiscard]] std::uint32_t ReadWord(std::uint32_t) override { return 0u; }
    [[nodiscard]] std::uint32_t IsPropertyEmpty(std::uint32_t) override {
        return 1u;
    }
    void InitializeIterator(PalObjectManagerIterator&, std::uint32_t) override {}
    void WrapIterator(PalObjectManagerIterator&,
                      PalObjectManagerIterator&) override {}
    [[nodiscard]] std::uint32_t IsEnd(
        PalObjectManagerIterator&) override { return 1u; }
    [[nodiscard]] std::uint32_t GetInstance(
        PalObjectManagerIterator&) override { return 0u; }
    void Invoke(std::uint32_t, std::uint32_t, std::uint32_t) override {}
    void Remove(PalObjectManagerIterator&, std::uint32_t) override {}
};

class PalObjectManagerTraversal final {
public:
    // IDA 0x5fc6d0.
    [[nodiscard]] static std::uint32_t Process(
        std::uint32_t object, std::uint32_t argument,
        IPalObjectManagerTraversalRuntime& runtime);
    // IDA 0x5fc740; the third native argument is unused.
    [[nodiscard]] static std::uint32_t ProcessWhenPresent(
        std::uint32_t object, std::uint32_t argument,
        std::uint32_t unused, IPalObjectManagerTraversalRuntime& runtime);
};

}  // namespace pal4
