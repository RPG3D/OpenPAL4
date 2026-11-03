#pragma once

#include <cstdint>

namespace pal4 {

class ITradeWindowBootstrapRuntime {
public:
    virtual ~ITradeWindowBootstrapRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    virtual void Free(std::uint32_t storage) = 0;
    [[nodiscard]] virtual std::uint32_t ConstructWindow(
        std::uint32_t storage, std::uint32_t name) = 0;
    [[nodiscard]] virtual std::uint32_t FrameManager() = 0;
    [[nodiscard]] virtual std::uint32_t ProcessFrameEvent(
        std::uint32_t manager, std::uint32_t name,
        std::uint32_t factory) = 0;
    virtual void InsertOrReplace(std::uint32_t tree, std::uint32_t key,
                                 std::uint32_t callback) = 0;
    virtual void ReleaseCallback(std::uint32_t callback,
                                 std::uint32_t flags) = 0;
};

class TradeWindowBootstrap final {
public:
    static constexpr std::uint32_t kNameSource = 0x8b0a54u;
    static constexpr std::uint32_t kCurrentName = 0x8e79a4u;
    static constexpr std::uint32_t kFactoryAddress = 0x48a840u;
    static constexpr std::uint32_t kShowEvent = 0x8b0a44u;
    static constexpr std::uint32_t kShowRegistration = 0x8e79a0u;
    static constexpr std::uint32_t kShowCallbackVtable = 0x843494u;

    // IDA 0x48a820-0x48a8e0.
    [[nodiscard]] static std::uint32_t GetNameThunk(
        ITradeWindowBootstrapRuntime& runtime);
    [[nodiscard]] static std::uint32_t GetName(
        ITradeWindowBootstrapRuntime& runtime);
    [[nodiscard]] static std::uint32_t Create(
        std::uint32_t name, ITradeWindowBootstrapRuntime& runtime);
    [[nodiscard]] static std::uint32_t RegisterFactoryThunk(
        ITradeWindowBootstrapRuntime& runtime);
    [[nodiscard]] static std::uint32_t RegisterFactory(
        ITradeWindowBootstrapRuntime& runtime);
    [[nodiscard]] static std::uint32_t RegisterShowEventThunk(
        ITradeWindowBootstrapRuntime& runtime);
    [[nodiscard]] static std::uint32_t RegisterShowEvent(
        ITradeWindowBootstrapRuntime& runtime);
};

}  // namespace pal4
