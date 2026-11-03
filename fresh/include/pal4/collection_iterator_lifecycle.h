#pragma once

#include <cstdint>

namespace pal4 {

struct CollectionIteratorState {
    std::uint32_t vtable{};
    std::uint32_t current{};
    std::uint32_t owner{};
};
static_assert(sizeof(CollectionIteratorState) == 12u);

class ICollectionIteratorLifecycleRuntime {
public:
    virtual ~ICollectionIteratorLifecycleRuntime() = default;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    virtual void Free(std::uint32_t address) = 0;
};

class CollectionIteratorLifecycleRuntimeStub final
    : public ICollectionIteratorLifecycleRuntime {
public:
    std::uint32_t Allocate(std::uint32_t bytes) override;
    std::uint32_t ReadWord(std::uint32_t address) override;
    void WriteWord(std::uint32_t address, std::uint32_t value) override;
    void Free(std::uint32_t address) override;
};

class CollectionIteratorLifecycle final {
public:
    // IDA 0x5eea10 / 0x5eea70.
    [[nodiscard]] static std::uint32_t& CreateFromThird(
        std::uint32_t group, std::uint32_t& output,
        ICollectionIteratorLifecycleRuntime& runtime);
    [[nodiscard]] static std::uint32_t& CreateFromFirst(
        std::uint32_t group, std::uint32_t& output,
        ICollectionIteratorLifecycleRuntime& runtime);
    // IDA 0x5eead0 / 0x5eeb40.
    static void InitFirstBase(std::uint32_t iterator,
                              ICollectionIteratorLifecycleRuntime& runtime);
    static void InitSecondBase(std::uint32_t iterator,
                               ICollectionIteratorLifecycleRuntime& runtime);
    // IDA 0x5eeae0 / 0x5eeb50.
    [[nodiscard]] static std::uint32_t CloneFirst(
        const CollectionIteratorState& source,
        ICollectionIteratorLifecycleRuntime& runtime);
    [[nodiscard]] static std::uint32_t CloneSecond(
        const CollectionIteratorState& source,
        ICollectionIteratorLifecycleRuntime& runtime);
    // IDA 0x5eeb10 / 0x5eeb20 / 0x5eeb30.
    [[nodiscard]] static std::uint32_t GetValue(
        std::uint32_t iterator, ICollectionIteratorLifecycleRuntime& runtime);
    [[nodiscard]] static std::uint32_t NextFromFirst(
        std::uint32_t iterator, ICollectionIteratorLifecycleRuntime& runtime);
    [[nodiscard]] static std::uint32_t NextFromSecond(
        std::uint32_t iterator, ICollectionIteratorLifecycleRuntime& runtime);
    // IDA 0x5eeb80 / 0x5eeba0 / 0x5eebc0 / 0x5eebe0.
    [[nodiscard]] static std::uint32_t DestroySecond(
        std::uint32_t iterator, std::uint32_t flag,
        ICollectionIteratorLifecycleRuntime& runtime);
    [[nodiscard]] static std::uint32_t DestroyFirst(
        std::uint32_t iterator, std::uint32_t flag,
        ICollectionIteratorLifecycleRuntime& runtime);
    [[nodiscard]] static std::uint32_t CleanupFirst(
        std::uint32_t iterator, std::uint32_t flag,
        ICollectionIteratorLifecycleRuntime& runtime);
    [[nodiscard]] static std::uint32_t CleanupSecond(
        std::uint32_t iterator, std::uint32_t flag,
        ICollectionIteratorLifecycleRuntime& runtime);
};

}  // namespace pal4
