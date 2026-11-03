#include "pal4/game_data_virtual_dispatch.h"

namespace pal4 {

std::uint32_t GameDataVirtualDispatch::Slot20(
    std::uint32_t argument, std::uint32_t target,
    IGameDataVirtualDispatchRuntime& runtime) {
    return runtime.Invoke(target, 20, argument);
}

std::uint32_t GameDataVirtualDispatch::Slot44(
    std::uint32_t argument, std::uint32_t target,
    IGameDataVirtualDispatchRuntime& runtime) {
    return runtime.Invoke(target, 44, argument);
}

std::uint32_t GameDataVirtualDispatch::Slot0(
    std::uint32_t argument, std::uint32_t target,
    IGameDataVirtualDispatchRuntime& runtime) {
    return runtime.Invoke(target, 0, argument);
}

std::uint32_t GameDataVirtualDispatch::Slot24(
    std::uint32_t argument, std::uint32_t target,
    IGameDataVirtualDispatchRuntime& runtime) {
    return runtime.Invoke(target, 24, argument);
}

std::uint32_t GameDataVirtualDispatch::RawName(std::uint32_t type_info) {
    return type_info + 8;
}

}  // namespace pal4
