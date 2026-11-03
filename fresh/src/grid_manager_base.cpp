#include "pal4/grid_manager.h"
#include "pal4/global_render_state.h"

#include <array>
#include <cstdint>

namespace pal4 {
namespace {

class BasisRuntime final : public IGlobalRenderStateRuntime {
public:
    BasisRuntime(GridManagerState& state, IGridManagerRuntime& runtime)
        : state_(state), runtime_(runtime) {}

    std::array<std::uint32_t, 3u> TransformDirection(
        const GlobalRenderState&,
        const std::array<std::uint32_t, 3u>& source) override {
        return runtime_.TransformBasis(state_, source);
    }

    std::uint32_t CommitDirection(
        GlobalRenderState&, const std::array<float, 3u>&) override {
        return 0u;
    }

private:
    GridManagerState& state_;
    IGridManagerRuntime& runtime_;
};

void InitializeBasis(GridManagerState& state, IGridManagerRuntime& runtime) {
    GlobalRenderState basis;
    for (std::uint32_t index = 0u; index < 13u; ++index)
        basis.WriteDword(4u * index, state.base_words[index]);
    BasisRuntime adapter(state, runtime);
    (void)basis.InitializeDefaultBasis(adapter);
    for (std::uint32_t index = 0u; index < 13u; ++index)
        state.base_words[index] = basis.ReadDword(4u * index);
}

void CopyZero(GridManagerState& state, const std::uint32_t first_word,
              const std::array<std::uint32_t, 3u>& zero) {
    for (std::uint32_t axis = 0u; axis < 3u; ++axis)
        state.base_words[first_word + axis] = zero[axis];
}

}  // namespace

GridManagerState& GridManager::ConstructBase(
    GridManagerState& state, IGridManagerRuntime& runtime) {
    for (std::uint32_t index = 1u; index <= 12u; ++index)
        state.base_words[index] = 0u;
    state.base_words[0] = 0x84b780u;
    InitializeBasis(state, runtime);
    for (std::uint32_t index = 15u; index <= 20u; ++index)
        state.base_words[index] = 0u;
    state.base_words[13] = 0u;
    state.base_words[14] = 0u;
    state.base_words[21] &= 0xffffff00u;
    CopyZero(state, 29u, {0u, 0u, 0u});
    CopyZero(state, 32u, {0u, 0u, 0u});
    CopyZero(state, 35u, {0u, 0u, 0u});
    CopyZero(state, 39u, {0u, 0u, 0u});
    state.base_words[0] = 0x84b6e8u;
    InitializeBasis(state, runtime);
    state.base_words[13] = 0u;
    state.base_words[14] = 0u;
    state.base_words[21] &= 0xffffff00u;
    runtime.SetParameter(state, 84u, 0x3f800000u);
    runtime.SetParameter(state, 104u, 0u);
    runtime.SetParameter(state, 92u, 0x3f000000u);
    runtime.SetParameter(state, 116u, 0x3dcccccdU);
    runtime.SetParameter(state, 124u, 0x3f800000u);
    const auto zero = runtime.ZeroVector();
    CopyZero(state, 35u, zero);
    CopyZero(state, 29u, zero);
    CopyZero(state, 32u, zero);
    state.base_words[28] = 0u;
    state.base_words[38] = 0u;
    CopyZero(state, 39u, zero);
    state.base_words[22] = runtime.NextIdentifier();
    return state;
}

void GridManager::SetBaseVtable(GridManagerState& state) {
    state.base_words[0] = 0x84b6e8u;
}

}  // namespace pal4
