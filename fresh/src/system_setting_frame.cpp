#include "pal4/system_setting_frame.h"
#include "pal4/system_setting_interface.h"

namespace pal4 {
namespace {

void ClearTriplet(const std::uint32_t address,
                  ISystemSettingFrameRuntime& runtime) {
    runtime.Free(runtime.ReadWord(address));
    runtime.WriteWord(address, 0u);
    runtime.WriteWord(address + 4u, 0u);
    runtime.WriteWord(address + 8u, 0u);
}

void ClearRecords(const std::uint32_t frame,
                  ISystemSettingFrameRuntime& runtime) {
    constexpr struct Group {
        std::uint32_t offset;
        std::uint32_t count;
    } groups[] = {{0xf0u, 3u}, {0x12cu, 2u},
                  {0x154u, 3u}, {0x190u, 2u}};
    for (const auto& group : groups) {
        for (std::uint32_t index = 0u; index < group.count; ++index) {
            for (std::uint32_t word = 0u; word < 5u; ++word) {
                runtime.WriteWord(frame + group.offset + index * 0x14u +
                                      word * 4u, 0u);
            }
        }
    }
}

template <typename Setup>
std::uint32_t ConstructCore(const std::uint32_t frame,
                            const std::uint32_t name,
                            ISystemSettingFrameRuntime& runtime,
                            Setup setup) {
    runtime.LoadLayout(frame, name, 0u, 0u);
    try {
        runtime.ConstructMember(frame + 0x28u);
        bool field_constructed = false;
        try {
            runtime.WriteWord(frame + 0x28u, SystemSettingFrame::kBaseMemberVtable);
            runtime.WriteWord(frame + 0x4cu, 0u);
            runtime.WriteByte(frame + 0x48u,
                              static_cast<std::uint8_t>(name));
            runtime.WriteWord(frame + 0x50u, 0u);
            runtime.WriteWord(frame + 0x54u, 0u);
            runtime.WriteWord(frame + 0x40u, 0x843be8u);
            field_constructed = true;
            runtime.WriteWord(frame + 0x60u, 0x843bd0u);
            ClearRecords(frame, runtime);
            runtime.WriteWord(frame, SystemSettingFrame::kObjectVtable);
            runtime.WriteWord(frame + 0x28u, SystemSettingFrame::kMemberVtable);
            setup();
        } catch (...) {
            if (field_constructed) {
                ClearTriplet(frame + 0x4cu, runtime);
            }
            runtime.CleanupMember(frame + 0x28u);
            throw;
        }
    } catch (...) {
        (void)runtime.DestroyDialog(frame);
        throw;
    }
    return frame;
}

}  // namespace

std::uint32_t SystemSettingFrame::GetName(
    ISystemSettingFrameRuntime& runtime) {
    return InitializeName(runtime);
}

std::uint32_t SystemSettingFrame::InitializeName(
    ISystemSettingFrameRuntime& runtime) {
    const auto name = runtime.ReadWord(kNameSource);
    runtime.WriteWord(kNameStorage, name);
    return name;
}

std::uint32_t SystemSettingFrame::Create(
    const std::uint32_t name, ISystemSettingFrameRuntime& runtime) {
    const auto frame = runtime.Allocate(kSize);
    if (frame == 0u) {
        return 0u;
    }
    try {
        return Construct(frame, name, runtime);
    } catch (...) {
        runtime.Free(frame);
        throw;
    }
}

std::uint32_t SystemSettingFrame::Create(
    const std::uint32_t name, ISystemSettingFrameRuntime& runtime,
    ISystemSettingInterfaceRuntime& interface_runtime) {
    const auto frame = runtime.Allocate(kSize);
    if (frame == 0u) {
        return 0u;
    }
    try {
        return Construct(frame, name, runtime, interface_runtime);
    } catch (...) {
        runtime.Free(frame);
        throw;
    }
}

std::uint32_t SystemSettingFrame::GetFactory(
    ISystemSettingFrameRuntime& runtime) {
    return Factory(runtime);
}

std::uint32_t SystemSettingFrame::Factory(
    ISystemSettingFrameRuntime& runtime) {
    const auto manager = runtime.FrameManager();
    return runtime.ProcessEvent(manager,
                                runtime.ReadWord(kNameStorage),
                                kCreateFunction);
}

std::uint32_t SystemSettingFrame::Construct(
    const std::uint32_t frame, const std::uint32_t name,
    ISystemSettingFrameRuntime& runtime) {
    return ConstructCore(frame, name, runtime,
                         [&]() { runtime.SetupInterface(frame); });
}

std::uint32_t SystemSettingFrame::Construct(
    const std::uint32_t frame, const std::uint32_t name,
    ISystemSettingFrameRuntime& runtime,
    ISystemSettingInterfaceRuntime& interface_runtime) {
    return ConstructCore(frame, name, runtime, [&]() {
        (void)SystemSettingInterface::Initialize(frame, interface_runtime);
    });
}

std::uint32_t SystemSettingFrame::Destroy(
    const std::uint32_t frame, ISystemSettingFrameRuntime& runtime) {
    ClearTriplet(frame + 0x4cu, runtime);
    ClearTriplet(frame + 0x30u, runtime);
    return runtime.DestroyDialog(frame);
}

}  // namespace pal4
