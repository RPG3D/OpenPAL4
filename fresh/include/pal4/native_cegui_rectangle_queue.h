#pragma once

#include "pal4/cegui_rectangle_queue.h"

namespace pal4 {

class NativeCeguiRectangleQueueRuntime final : public ICeguiRectangleQueueRuntime {
public:
    explicit NativeCeguiRectangleQueueRuntime(bool wide=false, bool native=false):wide_(wide),native_(native) {}
    [[nodiscard]] std::uint32_t Forward(
        CeguiRendererState& state, const CeguiRectWords& rectangle,
        std::uint32_t mode, std::uint32_t depth, std::uint32_t texture,
        const CeguiRectWords& uv,
        const std::array<CeguiColourWords, 4>& colours,
        std::uint32_t colour_source_address) override;
    void AssignRect(CeguiRectWords& destination,
                    const CeguiRectWords& source) override;
    void CopyColour(CeguiColourWords& destination,
                    const CeguiColourWords& source) override;
    [[nodiscard]] std::int32_t Convert(double value) override;
    [[nodiscard]] std::uint32_t Allocate(std::uint32_t bytes) override;
    void CopyRecord(std::uint32_t destination, std::uint32_t source) override;
    void FillRecord(std::uint32_t destination,
                    const CeguiRectangleRecord& record) override;
    void CopyStructuredRange(std::uint32_t begin, std::uint32_t end,
                             const CeguiRectangleRecord& record) override;
    void RangeNoOp(std::uint32_t begin, std::uint32_t end) override;
    void Free(std::uint32_t address) override;
    [[nodiscard]] std::uint32_t EffectCount(
        const CeguiRendererState& state) override;
private:
    bool wide_;
    bool native_;
};

}  // namespace pal4
