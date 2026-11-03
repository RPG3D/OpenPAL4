#pragma once

#include <cstdint>

namespace pal4 {

class IShopUiRegistrationRuntime {
public:
    virtual ~IShopUiRegistrationRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    [[nodiscard]] virtual std::uint32_t FrameManager() = 0;
    virtual void InsertOrReplace(std::uint32_t tree,
                                 std::uint32_t key,
                                 std::uint32_t callback) = 0;
    virtual void ReleaseCallback(std::uint32_t callback,
                                 std::uint32_t flags) = 0;
};

class ShopUiRegistration final {
public:
    static constexpr std::uint32_t kNameSource = 0x8b098cu;
    static constexpr std::uint32_t kShowSelectorEvent = 0x8b0980u;
    static constexpr std::uint32_t kSetShopIdEvent = 0x8b0984u;
    static constexpr std::uint32_t kSetTradeTaskEvent = 0x8b0988u;
    static constexpr std::uint32_t kShowSelectorStorage = 0x8e7996u;
    static constexpr std::uint32_t kSetShopIdStorage = 0x8e7995u;
    static constexpr std::uint32_t kSetTradeTaskStorage = 0x8e7994u;
    static constexpr std::uint32_t kNameCallbackVtable = 0x843494u;
    static constexpr std::uint32_t kTextCallbackVtable = 0x8435dcu;
    static constexpr std::uint32_t kSetTextFunction = 0x486890u;
    static constexpr std::uint32_t kSetTradeTaskFunction = 0x486980u;

    [[nodiscard]] static std::uint32_t GetFrameworkHandler(
        IShopUiRegistrationRuntime& runtime);
    [[nodiscard]] static std::uint32_t RegisterFramework(
        IShopUiRegistrationRuntime& runtime);
    [[nodiscard]] static std::uint32_t GetTextSetterOne(
        IShopUiRegistrationRuntime& runtime);
    [[nodiscard]] static std::uint32_t RegisterTextSetterOne(
        IShopUiRegistrationRuntime& runtime);
    [[nodiscard]] static std::uint32_t GetTextSetterTwo(
        IShopUiRegistrationRuntime& runtime);
    [[nodiscard]] static std::uint32_t RegisterTextSetterTwo(
        IShopUiRegistrationRuntime& runtime);
};

}  // namespace pal4
