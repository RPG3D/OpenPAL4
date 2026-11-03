#include "pal4/timer_frame.h"
#include "pal4/timer_frame_buttons.h"

namespace pal4 {
namespace {

template <typename InitializeButtons>
std::uint32_t ConstructCore(const std::uint32_t object,
                            const std::uint32_t name,
                            ITimerFrameRuntime& runtime,
                            InitializeButtons initialize_buttons) {
    runtime.LoadLayout(object, name, 0u, 0u);
    try {
        runtime.ConstructMember(object + 0x28u);
        try {
            runtime.WriteWord(object + 0x28u,
                              TimerFrame::kMemberBaseVtable);
            for (std::uint32_t offset = 0x40u; offset <= 0x50u;
                 offset += 8u) {
                runtime.ConstructVector2(object + offset);
            }
            runtime.WriteWord(object, TimerFrame::kObjectVtable);
            runtime.WriteWord(object + 0x28u,
                              TimerFrame::kMemberVtable);
            initialize_buttons();
            runtime.WriteWord(object + 0x70u, 0u);
        } catch (...) {
            runtime.CleanupMember(object + 0x28u);
            throw;
        }
    } catch (...) {
        (void)runtime.DestroyDialog(object);
        throw;
    }
    return object;
}

}  // namespace

std::uint32_t TimerFrame::GetScriptName(ITimerFrameRuntime& runtime) {
    return InitializeScriptName(runtime);
}

std::uint32_t TimerFrame::InitializeScriptName(
    ITimerFrameRuntime& runtime) {
    const auto name = runtime.ReadWord(kScriptNameSource);
    runtime.WriteWord(kScriptNameStorage, name);
    return name;
}

std::uint32_t TimerFrame::Create(const std::uint32_t name,
                                 ITimerFrameRuntime& runtime) {
    const auto object = runtime.Allocate(kSize);
    if (object == 0u) {
        return 0u;
    }
    try {
        return Construct(object, name, runtime);
    } catch (...) {
        runtime.Free(object);
        throw;
    }
}

std::uint32_t TimerFrame::Create(
    const std::uint32_t name, ITimerFrameRuntime& runtime,
    ITimerFrameButtonsRuntime& buttons_runtime) {
    const auto object = runtime.Allocate(kSize);
    if (object == 0u) {
        return 0u;
    }
    try {
        return Construct(object, name, runtime, buttons_runtime);
    } catch (...) {
        runtime.Free(object);
        throw;
    }
}

std::uint32_t TimerFrame::GetFactory(ITimerFrameRuntime& runtime) {
    return Factory(runtime);
}

std::uint32_t TimerFrame::Factory(ITimerFrameRuntime& runtime) {
    const auto manager = runtime.FrameManager();
    const auto script_name = runtime.ReadWord(kScriptNameStorage);
    return runtime.ProcessEvent(manager, script_name, kCreateFunction);
}

std::uint32_t TimerFrame::Construct(
    const std::uint32_t object, const std::uint32_t name,
    ITimerFrameRuntime& runtime) {
    return ConstructCore(object, name, runtime,
                         [&]() { runtime.InitializeButtons(object); });
}

std::uint32_t TimerFrame::Construct(
    const std::uint32_t object, const std::uint32_t name,
    ITimerFrameRuntime& runtime,
    ITimerFrameButtonsRuntime& buttons_runtime) {
    return ConstructCore(object, name, runtime, [&]() {
        (void)TimerFrameButtons::Initialize(object, buttons_runtime);
    });
}

std::uint32_t TimerFrame::Destroy(
    const std::uint32_t object, ITimerFrameRuntime& runtime) {
    runtime.Free(runtime.ReadWord(object + 0x30u));
    runtime.WriteWord(object + 0x30u, 0u);
    runtime.WriteWord(object + 0x34u, 0u);
    runtime.WriteWord(object + 0x38u, 0u);
    return runtime.DestroyDialog(object);
}

}  // namespace pal4
