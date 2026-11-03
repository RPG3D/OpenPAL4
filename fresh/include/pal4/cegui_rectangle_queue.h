#pragma once

#include "pal4/cegui_packed_color.h"
#include "pal4/cegui_renderer.h"

#include <array>
#include <cstdint>
#include <map>

namespace pal4 {

using CeguiRectWords = std::array<std::uint32_t, 4>;
using CeguiColourWords = std::array<float, 6>;

class CeguiRectangleRecord final {
public:
    static constexpr std::uint32_t kSize = 60u;
    [[nodiscard]] std::uint32_t ReadDword(std::uint32_t offset) const noexcept;
    void WriteDword(std::uint32_t offset, std::uint32_t value) noexcept;
    [[nodiscard]] const std::array<std::uint8_t, kSize>& Bytes() const noexcept {
        return bytes_;
    }

private:
    std::array<std::uint8_t, kSize> bytes_{};
};

class ICeguiRectangleQueueRuntime : public CeguiPackedColor::IFloatToInt {
public:
    virtual ~ICeguiRectangleQueueRuntime() = default;
    [[nodiscard]] virtual std::uint32_t Forward(
        CeguiRendererState& state, const CeguiRectWords& rectangle,
        std::uint32_t unused, std::uint32_t depth,
        std::uint32_t texture, const CeguiRectWords& uv,
        const std::array<CeguiColourWords, 4>& colours,
        std::uint32_t colour_source_address) = 0;
    virtual void AssignRect(CeguiRectWords& destination,
                            const CeguiRectWords& source) = 0;
    virtual void CopyColour(CeguiColourWords& destination,
                            const CeguiColourWords& source) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    virtual void CopyRecord(std::uint32_t destination,
                            std::uint32_t source) = 0;
    virtual void FillRecord(std::uint32_t destination,
                            const CeguiRectangleRecord& record) = 0;
    virtual void CopyStructuredRange(
        std::uint32_t begin, std::uint32_t end,
        const CeguiRectangleRecord& record) = 0;
    virtual void RangeNoOp(std::uint32_t begin, std::uint32_t end) = 0;
    virtual void Free(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t EffectCount(
        const CeguiRendererState& state) = 0;
};

class CeguiRectangleQueueStub : public ICeguiRectangleQueueRuntime {
public:
    [[nodiscard]] std::uint32_t Forward(
        CeguiRendererState& state, const CeguiRectWords& rectangle,
        std::uint32_t unused, std::uint32_t depth,
        std::uint32_t texture, const CeguiRectWords& uv,
        const std::array<CeguiColourWords, 4>& colours,
        std::uint32_t colour_source_address) override;
    void AssignRect(CeguiRectWords& destination,
                    const CeguiRectWords& source) override;
    void CopyColour(CeguiColourWords& destination,
                    const CeguiColourWords& source) override;
    [[nodiscard]] std::int32_t Convert(double value) override;
    [[nodiscard]] std::uint32_t Allocate(std::uint32_t bytes) override;
    void CopyRecord(std::uint32_t destination,
                    std::uint32_t source) override;
    void FillRecord(std::uint32_t destination,
                    const CeguiRectangleRecord& record) override;
    void CopyStructuredRange(
        std::uint32_t begin, std::uint32_t end,
        const CeguiRectangleRecord& record) override;
    void RangeNoOp(std::uint32_t begin, std::uint32_t end) override;
    void Free(std::uint32_t address) override;
    [[nodiscard]] std::uint32_t EffectCount(
        const CeguiRendererState& state) override;
    [[nodiscard]] const CeguiRectangleRecord& Record(
        std::uint32_t address) const;

private:
    std::uint32_t next_address_ = 0x10000u;
    std::map<std::uint32_t, CeguiRectangleRecord> records_;
};

class CeguiRectangleQueue final {
public:
    static void Clear(CeguiRendererState& state) noexcept;
    [[nodiscard]] static std::uint32_t Draw(
        CeguiRendererState& state, const CeguiRectWords& rectangle,
        std::uint32_t unused, std::uint32_t depth,
        std::uint32_t texture, const CeguiRectWords& uv,
        const std::array<CeguiColourWords, 4>& colours,
        std::uint32_t colour_source_address,
        ICeguiRectangleQueueRuntime& runtime);
};

}  // namespace pal4
