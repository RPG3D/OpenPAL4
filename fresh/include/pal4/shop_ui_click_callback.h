#pragma once

#include <cstdint>

namespace pal4 {

class IShopUiClickCallbackRuntime {
public:
    virtual ~IShopUiClickCallbackRuntime() = default;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
};

class ShopUiClickCallback final {
public:
    static constexpr std::uint32_t kVtable = 0x843d1cu;

    // IDA 0x486dd0.
    [[nodiscard]] static std::uint32_t Construct(
        std::uint32_t storage, std::uint32_t function,
        std::uint32_t adjustment, std::uint32_t frame,
        IShopUiClickCallbackRuntime& runtime);
};

}  // namespace pal4
