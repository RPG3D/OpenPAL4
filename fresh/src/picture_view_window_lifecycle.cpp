#include "pal4/picture_view_window_lifecycle.h"

#include <new>

namespace pal4 {
namespace {

void FreeRecordList(PictureViewWindowState& state,
                    IPictureViewWindowLifecycleRuntime& runtime) {
    runtime.FreeMemory(state.Read(0x54u));
    state.Write(0x54u, 0u);
    state.Write(0x58u, 0u);
    state.Write(0x5cu, 0u);
}

void FreeMember(PictureViewWindowState& state,
                IPictureViewWindowLifecycleRuntime& runtime) {
    runtime.FreeMemory(state.Read(0x30u));
    state.Write(0x30u, 0u);
    state.Write(0x34u, 0u);
    state.Write(0x38u, 0u);
}

}  // namespace

bool PictureViewWindowRecords::Eligible(std::uint32_t record_id) noexcept {
    return static_cast<std::int32_t>(record_id) >= 100;
}

std::uint32_t PictureViewWindowRecords::CopyWords(
    std::uint32_t begin, std::uint32_t end, std::uint32_t destination,
    IPictureViewWindowLifecycleRuntime& runtime) {
    while (begin != end) {
        if (destination != 0u)
            runtime.WriteWord(destination, runtime.ReadWord(begin));
        begin += 4u;
        destination += 4u;
    }
    return destination;
}

void PictureViewWindowRecords::Append(
    PictureViewWindowState& state, std::uint32_t record_address,
    IPictureViewWindowLifecycleRuntime& runtime) {
    const auto begin = state.Read(0x54u);
    const auto end = state.Read(0x58u);
    const auto capacity_end = state.Read(0x5cu);
    if (end != 0u && capacity_end - end >= 4u) {
        runtime.WriteWord(end, record_address);
        state.Write(0x58u, end + 4u);
        return;
    }

    const auto size = begin == 0u ? 0u : (end - begin) / 4u;
    const auto increment = begin != 0u && size > 1u ? size : 1u;
    const auto capacity = size + increment;
    const auto replacement = runtime.AllocateMemory(capacity * 4u);
    if (replacement == 0u) throw std::bad_alloc();
    for (std::uint32_t index = 0u; index < size; ++index)
        runtime.WriteWord(replacement + index * 4u,
                          runtime.ReadWord(begin + index * 4u));
    runtime.WriteWord(replacement + size * 4u, record_address);
    runtime.FreeMemory(begin);
    state.Write(0x54u, replacement);
    state.Write(0x58u, replacement + (size + 1u) * 4u);
    state.Write(0x5cu, replacement + capacity * 4u);
}

PictureViewWindowState& PictureViewWindowLifecycle::Construct(
    PictureViewWindowState& state, std::uint32_t object_address,
    std::uint32_t name, IPictureViewWindowLifecycleRuntime& runtime) {
    runtime.LoadLayout(state, name, 0u, 0u);
    state.Write(0x28u, kInitialMemberVtable);
    state.bytes[0x2cu] = static_cast<std::uint8_t>(
        (object_address + 0x28u) >> 8u);
    state.Write(0x30u, 0u);
    state.Write(0x34u, 0u);
    state.Write(0x38u, 0u);
    state.Write(0x3cu, 0u);
    state.bytes[0x50u] = static_cast<std::uint8_t>(name);
    state.Write(0x54u, 0u);
    state.Write(0x58u, 0u);
    state.Write(0x5cu, 0u);
    state.Write(0u, kObjectVtable);
    state.Write(0x28u, kMemberVtable);
    try {
        runtime.InitializeElements(state);
        const auto range = runtime.PictureRecords();
        if (range.begin != 0u) {
            const auto count = (range.end - range.begin) / 0x34u;
            for (std::uint32_t index = 0u; index < count; ++index) {
                const auto address = range.begin + index * 0x34u;
                if (PictureViewWindowRecords::Eligible(
                        runtime.ReadWord(address)))
                    PictureViewWindowRecords::Append(state, address, runtime);
            }
        }
        state.Write(0x60u, 0u);
    } catch (...) {
        FreeRecordList(state, runtime);
        FreeMember(state, runtime);
        (void)runtime.DestroyDialog(state);
        throw;
    }
    return state;
}

std::uint32_t PictureViewWindowLifecycle::Cleanup(
    PictureViewWindowState& state,
    IPictureViewWindowLifecycleRuntime& runtime) {
    FreeRecordList(state, runtime);
    FreeMember(state, runtime);
    return runtime.DestroyDialog(state);
}

}  // namespace pal4
