#include "pal4/pal_movement_adapter.h"

#include <bit>

namespace pal4 {

std::uint32_t PalMovementAdapterRuntimeStub::Allocate(std::uint32_t) {
    return 0u;
}
PalMovementAdapterState& PalMovementAdapterRuntimeStub::StateAt(std::uint32_t) {
    return empty_state_;
}
std::uint32_t PalMovementAdapterRuntimeStub::ConstructPrimary(std::uint32_t) {
    return 0u;
}
std::uint32_t PalMovementAdapterRuntimeStub::ConstructSecondary(std::uint32_t) {
    return 0u;
}
std::uint32_t PalMovementAdapterRuntimeStub::SingletonManager() {
    return 0u;
}
std::uint32_t PalMovementAdapterRuntimeStub::RegisterSingleton(
    std::uint32_t, std::string_view, std::uint32_t, std::uint32_t,
    std::uint32_t) { return 0u; }
void PalMovementAdapterRuntimeStub::NoopDestroy(std::uint32_t) {}
std::uint32_t PalMovementAdapterRuntimeStub::Free(std::uint32_t) {
    return 0u;
}
std::uint32_t PalMovementAdapterRuntimeStub::PreDestroy(
    std::uint32_t, PalMovementAdapterState&) { return 0u; }
std::uint32_t PalMovementAdapterRuntimeStub::DeleteChild(
    std::uint32_t, std::uint32_t) { return 0u; }
std::uint32_t PalMovementAdapterRuntimeStub::DispatchPrimary(
    std::uint32_t, const std::array<std::uint32_t, 7>&) { return 0u; }
std::uint32_t PalMovementAdapterRuntimeStub::ProbeChild(
    std::uint32_t, std::uint32_t, std::uint32_t, std::uint32_t,
    std::uint32_t, std::uint32_t, PalMovementProbeScratch&) { return 0u; }
std::uint32_t PalMovementAdapterRuntimeStub::CanSwitch(
    std::uint32_t, std::uint32_t, std::uint32_t,
    std::uint32_t, std::uint32_t) { return 0u; }
std::uint32_t PalMovementAdapterRuntimeStub::UpdateChild(
    std::uint32_t, std::uint32_t, std::uint32_t,
    std::uint32_t) { return 0u; }
void PalMovementAdapterRuntimeStub::DeactivateChild(std::uint32_t) {}
void PalMovementAdapterRuntimeStub::ActivateChild(std::uint32_t) {}
std::uint32_t PalMovementAdapterRuntimeStub::ProbeActive(
    std::uint32_t, std::uint32_t, std::uint32_t,
    const std::array<std::uint32_t, 4>&) { return 0u; }

std::uint32_t PalMovementAdapter::Initialize(
    PalMovementAdapterGlobalState& globals,
    IPalMovementAdapterRuntime& runtime) {
    if ((globals.initialized & 1u) != 0u) return globals.instance;
    globals.initialized |= 1u;
    const auto manager = runtime.SingletonManager();
    globals.instance = runtime.RegisterSingleton(
        manager, kSingletonName, kSingletonId, kFactoryAddress,
        kDestroyAddress);
    return globals.instance;
}

std::uint32_t PalMovementAdapter::Create(
    IPalMovementAdapterRuntime& runtime) {
    const auto address = runtime.Allocate(sizeof(PalMovementAdapterState));
    if (address == 0u) return 0u;
    static_cast<void>(Construct(runtime.StateAt(address), runtime));
    return address;
}

PalMovementAdapterState& PalMovementAdapter::Construct(
    PalMovementAdapterState& state,
    IPalMovementAdapterRuntime& runtime) {
    const auto primary = runtime.Allocate(0x74u);
    state.words[0] = primary != 0u ? runtime.ConstructPrimary(primary) : 0u;
    const auto secondary = runtime.Allocate(0x88u);
    state.words[1] = secondary != 0u ? runtime.ConstructSecondary(secondary) : 0u;
    state.words[3] = 0u;
    state.words[2] = state.words[0];
    state.words[4] = 0u;
    return state;
}

std::uint32_t PalMovementAdapter::Destroy(
    std::uint32_t address, std::uint32_t undefined_return,
    IPalMovementAdapterRuntime& runtime) {
    if (address == 0u) return undefined_return;
    runtime.NoopDestroy(address);
    return runtime.Free(address);
}

std::uint32_t PalMovementAdapter::Destruct(
    PalMovementAdapterState& state, std::uint32_t undefined_return,
    IPalMovementAdapterRuntime& runtime) {
    auto result = undefined_return;
    for (std::size_t index = 0u; index < 2u; ++index) {
        if (const auto child = state.words[index]; child != 0u) {
            result = runtime.PreDestroy(child, state);
            if (const auto current = state.words[index]; current != 0u)
                result = runtime.DeleteChild(current, 1u);
            state.words[index] = 0u;
        }
    }
    state.words[4] = 0u;
    return result;
}

std::uint32_t PalMovementAdapter::ForwardPrimary(
    const PalMovementAdapterState& state,
    std::array<std::uint32_t, 7> arguments,
    IPalMovementAdapterRuntime& runtime) {
    if (state.words[2] == 0u) return 0u;
    if (state.words[4] != 0u) arguments[1] = 0u;
    return runtime.DispatchPrimary(state.words[2], arguments);
}

std::uint32_t PalMovementAdapter::SelectAndUpdate(
    PalMovementAdapterState& state, std::uint32_t first,
    std::uint32_t second, std::uint32_t third,
    std::uint32_t fourth, IPalMovementAdapterRuntime& runtime) {
    auto best_score = std::bit_cast<float>(0xff7fffffu);
    std::uint32_t selected = 0u;
    std::uint32_t result = 0u;
    PalMovementProbeScratch scratch{};
    for (std::uint32_t index = 0u; index < 2u; ++index) {
        const auto child = state.words[index];
        const auto valid = runtime.ProbeChild(
            child, 1u, 1u, first, second, third, scratch);
        const auto score = valid != 0u ? scratch.score :
            std::bit_cast<float>(0xff7fffffu);
        if (best_score < score) {
            selected = state.words[index];
            state.words[3] = index;
            best_score = score;
            result = 1u;
        }
    }

    if (selected != 0u && selected != state.words[2]) {
        if (runtime.CanSwitch(selected, first, second, third, fourth) == 0u) {
            result = 0u;
        } else if (runtime.UpdateChild(selected, first, second, third) != 0u) {
            if (state.words[2] != 0u)
                runtime.DeactivateChild(state.words[2]);
            runtime.ActivateChild(selected);
            static_cast<void>(runtime.ProbeChild(
                selected, 1u, 1u, first, second, third, scratch));
            state.words[2] = selected;
        }
    }
    static_cast<void>(runtime.UpdateChild(
        state.words[2], first, second, third));
    return result;
}

bool PalMovementAdapter::CheckActive(
    const PalMovementAdapterState& state,
    const std::array<std::uint32_t, 4>& arguments,
    IPalMovementAdapterRuntime& runtime) {
    return state.words[2] != 0u &&
        runtime.ProbeActive(state.words[2], 0u, 0u, arguments) != 0u;
}

bool PalMovementAdapter::AnyChildMatches(
    const PalMovementAdapterState& state,
    const std::array<std::uint32_t, 4>& arguments,
    IPalMovementAdapterRuntime& runtime) {
    for (std::uint32_t index = 0u; index < 2u; ++index) {
        if (runtime.ProbeActive(
                state.words[index], 0u, 0u, arguments) != 0u)
            return true;
    }
    return false;
}

}  // namespace pal4
