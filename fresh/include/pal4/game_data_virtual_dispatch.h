#pragma once

#include <cstdint>

namespace pal4 {

class IGameDataVirtualDispatchRuntime {
public:
    virtual ~IGameDataVirtualDispatchRuntime() = default;
    [[nodiscard]] virtual std::uint32_t Invoke(
        std::uint32_t target, std::uint32_t slot_offset,
        std::uint32_t argument) = 0;
};

class GameDataVirtualDispatch final {
public:
    // IDA 0x4c8930/40/50/60: forward this through four target vtable slots.
    [[nodiscard]] static std::uint32_t Slot20(
        std::uint32_t argument, std::uint32_t target,
        IGameDataVirtualDispatchRuntime& runtime);
    [[nodiscard]] static std::uint32_t Slot44(
        std::uint32_t argument, std::uint32_t target,
        IGameDataVirtualDispatchRuntime& runtime);
    [[nodiscard]] static std::uint32_t Slot0(
        std::uint32_t argument, std::uint32_t target,
        IGameDataVirtualDispatchRuntime& runtime);
    [[nodiscard]] static std::uint32_t Slot24(
        std::uint32_t argument, std::uint32_t target,
        IGameDataVirtualDispatchRuntime& runtime);
    // IDA 0x4c8970: MSVC type_info::raw_name offset.
    [[nodiscard]] static std::uint32_t RawName(std::uint32_t type_info);
};

}  // namespace pal4
