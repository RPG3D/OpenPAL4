#pragma once

#include <array>
#include <cstdint>
#include <vector>

namespace pal4 {

class IMinimapGridRuntime {
public:
    virtual ~IMinimapGridRuntime() = default;
    [[nodiscard]] virtual std::uint32_t DestroyPluginObject(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t ReadDword(
        std::uint32_t address) = 0;
};

class IMinimapGridEffectsRuntime : public IMinimapGridRuntime {
public:
    [[nodiscard]] virtual std::uint8_t ReadByte(
        std::uint32_t address) = 0;
    virtual void WriteByte(std::uint32_t address,
                           std::uint8_t value) = 0;
    [[nodiscard]] virtual std::int32_t ConvertOpacity(
        double value) = 0;
};

class MinimapGridState final {
public:
    [[nodiscard]] std::uint32_t Cleanup(IMinimapGridRuntime& runtime);
    [[nodiscard]] std::uint32_t CleanupThunk(
        IMinimapGridRuntime& runtime) {
        return Cleanup(runtime);
    }
    [[nodiscard]] std::uint32_t Initialize(
        std::uint32_t grid_address, std::uint32_t flags,
        const std::array<std::uint8_t, 24>& data,
        IMinimapGridRuntime& runtime);
    [[nodiscard]] std::uint32_t PixelBuffer(
        IMinimapGridRuntime& runtime) const;
    [[nodiscard]] std::uint32_t Stride(
        IMinimapGridRuntime& runtime) const;
    [[nodiscard]] std::uint32_t ReadField12(
        IMinimapGridRuntime& runtime) const;
    [[nodiscard]] std::uint32_t Width(
        IMinimapGridRuntime& runtime) const;
    [[nodiscard]] std::uint32_t Height(
        IMinimapGridRuntime& runtime) const;
    [[nodiscard]] std::uint8_t ApplyRadius(
        std::int32_t x, std::int32_t y,
        IMinimapGridEffectsRuntime& runtime) const;
    [[nodiscard]] std::uint8_t EncodeOpaqueMask(
        std::vector<std::uint32_t>& words,
        IMinimapGridEffectsRuntime& runtime) const;
    [[nodiscard]] std::uint8_t ApplyOpaqueMask(
        const std::vector<std::uint32_t>& words,
        IMinimapGridEffectsRuntime& runtime) const;
    [[nodiscard]] const std::array<float, 3>& WorldToGridCoordinates(
        std::array<float, 2>& output,
        const std::array<float, 3>& position) const;

    [[nodiscard]] std::uint32_t GridAddress() const { return grid_address_; }
    [[nodiscard]] std::uint32_t Flags() const { return flags_; }
    [[nodiscard]] const std::array<std::uint8_t, 24>& Data() const {
        return data_;
    }
    void SetState(std::uint32_t grid_address, std::uint32_t flags) {
        grid_address_ = grid_address;
        flags_ = flags;
    }

private:
    [[nodiscard]] std::uint32_t ReadGridField(
        std::uint32_t offset, IMinimapGridRuntime& runtime) const;

    std::uint32_t grid_address_ = 0u;
    std::uint32_t flags_ = 0u;
    std::array<std::uint8_t, 24> data_{};
};

}  // namespace pal4
