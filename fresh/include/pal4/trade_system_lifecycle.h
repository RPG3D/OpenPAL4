#pragma once

#include <cstdint>

namespace pal4 {

struct TradeSystemSingletonState {
    std::uint8_t initialization_bits = 0u;
    std::uint32_t instance = 0u;
};

class ITradeSystemSingletonRuntime {
public:
    virtual ~ITradeSystemSingletonRuntime() = default;
    [[nodiscard]] virtual std::uint32_t GetSingletonManager() = 0;
    [[nodiscard]] virtual std::uint32_t Register(
        std::uint32_t manager, const char* name, std::uint32_t id,
        std::uint32_t factory, std::uint32_t destructor) = 0;
};

class TradeSystemSingleton final {
public:
    static constexpr std::uint32_t kGuardAddress = 0x8efaacu;
    static constexpr std::uint32_t kInstanceAddress = 0x8efaa8u;
    static constexpr std::uint32_t kFactoryAddress = 0x50f970u;
    static constexpr std::uint32_t kDestructorAddress = 0x428c00u;

    explicit TradeSystemSingleton(TradeSystemSingletonState& state)
        : state_(state) {}

    // IDA 0x50f930.
    [[nodiscard]] std::uint32_t GetInstance(
        ITradeSystemSingletonRuntime& runtime);

private:
    TradeSystemSingletonState& state_;
};

class ITradeSystemLifecycleRuntime {
public:
    virtual ~ITradeSystemLifecycleRuntime() = default;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    virtual void Clear(std::uint32_t address, std::uint32_t bytes) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
};

class TradeSystemLifecycle final {
public:
    static constexpr std::uint32_t kObjectBytes = 0x54u;

    // IDA 0x50f970 and 0x50f9d0.
    [[nodiscard]] static std::uint32_t CreateInstance(
        ITradeSystemLifecycleRuntime& runtime);
    [[nodiscard]] static std::uint32_t Construct(
        std::uint32_t object, ITradeSystemLifecycleRuntime& runtime);
};

}  // namespace pal4
