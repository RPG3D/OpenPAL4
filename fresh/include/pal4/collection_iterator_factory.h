#pragma once

#include <cstdint>

namespace pal4 {

struct CollectionIteratorFactoryState {
    std::uint32_t vtable{};
    std::uint32_t source{};
    std::uint32_t created{};
};
static_assert(sizeof(CollectionIteratorFactoryState) == 12u);

class ICollectionIteratorFactoryRuntime {
public:
    virtual ~ICollectionIteratorFactoryRuntime() = default;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
};

class CollectionIteratorFactoryRuntimeStub final
    : public ICollectionIteratorFactoryRuntime {
public:
    std::uint32_t Allocate(std::uint32_t bytes) override;
    std::uint32_t ReadWord(std::uint32_t address) override;
    void WriteWord(std::uint32_t address, std::uint32_t value) override;
};

class CollectionIteratorFactory final {
public:
    // IDA 0x5ee4f0..0x5ee680.
    [[nodiscard]] static std::uint32_t Create1(
        CollectionIteratorFactoryState& owner, std::uint32_t argument,
        ICollectionIteratorFactoryRuntime& runtime);
    [[nodiscard]] static std::uint32_t Create2(
        CollectionIteratorFactoryState& owner, std::uint32_t argument,
        ICollectionIteratorFactoryRuntime& runtime);
    [[nodiscard]] static std::uint32_t Create3(
        CollectionIteratorFactoryState& owner, std::uint32_t argument,
        ICollectionIteratorFactoryRuntime& runtime);
    [[nodiscard]] static std::uint32_t Create4(
        CollectionIteratorFactoryState& owner, std::uint32_t argument,
        ICollectionIteratorFactoryRuntime& runtime);
    [[nodiscard]] static std::uint32_t Create5(
        CollectionIteratorFactoryState& owner, std::uint32_t argument,
        ICollectionIteratorFactoryRuntime& runtime);
    [[nodiscard]] static std::uint32_t Create6(
        CollectionIteratorFactoryState& owner, std::uint32_t source_range,
        ICollectionIteratorFactoryRuntime& runtime);

private:
    [[nodiscard]] static std::uint32_t CreateBasic(
        CollectionIteratorFactoryState& owner, std::uint32_t argument,
        std::uint32_t vtable, ICollectionIteratorFactoryRuntime& runtime);
};

}  // namespace pal4
