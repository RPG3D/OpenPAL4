#pragma once

#include "pal4/cegui_renderer.h"

#include <cstdint>
#include <map>
#include <optional>

namespace pal4 {

class ICeguiRenderHelperRuntime {
public:
    virtual ~ICeguiRenderHelperRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadDword(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t SetRenderState(
        std::uint32_t state_id, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t DrawIndexed(
        std::uint32_t primitive_type, std::uint32_t vertices,
        std::uint32_t vertex_count, std::uint32_t indices,
        std::uint32_t index_count) = 0;
};

class CeguiRenderHelperStub : public ICeguiRenderHelperRuntime {
public:
    [[nodiscard]] std::uint32_t ReadDword(
        std::uint32_t address) override;
    [[nodiscard]] std::uint32_t SetRenderState(
        std::uint32_t state_id, std::uint32_t value) override;
    [[nodiscard]] std::uint32_t DrawIndexed(
        std::uint32_t primitive_type, std::uint32_t vertices,
        std::uint32_t vertex_count, std::uint32_t indices,
        std::uint32_t index_count) override;
    void WriteDword(std::uint32_t address, std::uint32_t value);

private:
    std::map<std::uint32_t, std::uint32_t> words_;
};

class CeguiRenderHelpers final {
public:
    [[nodiscard]] static std::uint32_t MinGameStateExtent(
        std::uint32_t game_state_base, ICeguiRenderHelperRuntime& runtime);
    [[nodiscard]] static std::uint32_t SetRenderStates(
        ICeguiRenderHelperRuntime& runtime);
    [[nodiscard]] static std::optional<std::uint32_t> FlushGeometry(
        CeguiRendererState& state, ICeguiRenderHelperRuntime& runtime);
};

}  // namespace pal4
