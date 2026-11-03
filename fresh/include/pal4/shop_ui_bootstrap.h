#pragma once

#include <cstdint>

namespace pal4 {

class IShopUiBootstrapRuntime {
public:
    virtual ~IShopUiBootstrapRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    [[nodiscard]] virtual std::uint32_t ConstructShop(
        std::uint32_t storage, std::uint32_t name) = 0;
    virtual void Free(std::uint32_t storage) = 0;
    [[nodiscard]] virtual std::uint32_t FrameManager() = 0;
    [[nodiscard]] virtual std::uint32_t ProcessFrameEvent(
        std::uint32_t manager, std::uint32_t name,
        std::uint32_t factory) = 0;
};

class ShopUiBootstrap final {
public:
    static constexpr std::uint32_t kNameSource = 0x8b098cu;
    static constexpr std::uint32_t kCurrentName = 0x8e7998u;
    static constexpr std::uint32_t kFactoryAddress = 0x4855c0u;

    [[nodiscard]] static std::uint32_t GetFrameType(
        IShopUiBootstrapRuntime& runtime);
    [[nodiscard]] static std::uint32_t InitializeFrameType(
        IShopUiBootstrapRuntime& runtime);
    [[nodiscard]] static std::uint32_t Create(
        std::uint32_t name, IShopUiBootstrapRuntime& runtime);
    [[nodiscard]] static std::uint32_t GetFrame(
        IShopUiBootstrapRuntime& runtime);
    [[nodiscard]] static std::uint32_t RegisterFrame(
        IShopUiBootstrapRuntime& runtime);
};

}  // namespace pal4
