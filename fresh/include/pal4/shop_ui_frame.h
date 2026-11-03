#pragma once

#include <cstdint>

namespace pal4 {

class IShopUiFrameRuntime {
public:
    virtual ~IShopUiFrameRuntime() = default;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    virtual void LoadLayout(std::uint32_t frame, std::uint32_t name,
                            std::uint32_t left, std::uint32_t top) = 0;
    virtual void ConstructMouse(std::uint32_t mouse) = 0;
    virtual void ConstructVector2(std::uint32_t vector) = 0;
    virtual void InitializeControls(std::uint32_t frame) = 0;
    virtual void CleanupBase(std::uint32_t frame) = 0;
    virtual void CleanupMouse(std::uint32_t mouse) = 0;
    virtual void CleanupText(std::uint32_t text) = 0;
    virtual void Free(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t DestroyBase(std::uint32_t frame) = 0;
    [[nodiscard]] virtual std::uint32_t CallVirtual(
        std::uint32_t function, std::uint32_t frame) = 0;
};

class ShopUiFrame final {
public:
    static constexpr std::uint32_t kFrameVtable = 0x843d04u;
    static constexpr std::uint32_t kMouseInitialVtable = 0x843628u;
    static constexpr std::uint32_t kMouseFinalVtable = 0x843cdcu;

    [[nodiscard]] static std::uint32_t Construct(
        std::uint32_t frame, std::uint32_t name,
        IShopUiFrameRuntime& runtime);
    [[nodiscard]] static std::uint32_t ForwardVirtualSlotThree(
        std::uint32_t frame, IShopUiFrameRuntime& runtime);
    [[nodiscard]] static std::uint32_t Destroy(
        std::uint32_t frame, IShopUiFrameRuntime& runtime);
};

}  // namespace pal4
