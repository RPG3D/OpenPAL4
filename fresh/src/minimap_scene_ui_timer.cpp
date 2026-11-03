#include "pal4/minimap_scene_ui_timer.h"

#include <limits>
#include <stdexcept>
#include <string>

namespace pal4 {
namespace {

std::uint32_t ReleaseIncomingText(
    const StringBufferState& text,
    ITimerQueueEntryRuntime& runtime) {
    if (text.data == 0u) {
        return 0u;
    }
    const auto count = runtime.ReadReferenceCount(text.data);
    const auto header = text.data - 1u;
    if (count == 0u || count == 0xffu) {
        return runtime.FreeName(header);
    }
    runtime.WriteReferenceCount(text.data,
                                static_cast<std::uint8_t>(count - 1u));
    return header;
}

}  // namespace

void MinimapSceneUiTimer::StartCountdown(MinimapSceneUiState& state,
    std::uint32_t owner, std::uint32_t period, IMinimapCountdownStartRuntime& runtime) {
    state.WriteWord(0xf0u, period);
    runtime.RemoveCountdown(owner);
    runtime.ScheduleCountdown(owner);
    runtime.AssignCallback(owner);
    runtime.ExecuteCountdown(owner);
}

void MinimapSceneUiTimer::ClearCountdown(MinimapSceneUiState& state,
    std::uint32_t owner, IMinimapCountdownClearRuntime& runtime) {
    runtime.RemoveCountdown(owner);
    state.WriteWord(0xf0u, 0u);
    runtime.ClearCallbackAndText(owner);
}

std::uint32_t MinimapSceneUiTimer::Initialize(
    MinimapSceneUiState& state, const std::uint32_t owner,
    const std::uint32_t period, const StringBufferState& text,
    IMinimapSceneUiTimerRuntime& runtime) {
    class StartRuntime final : public IMinimapCountdownStartRuntime {
    public:
        StartRuntime(IMinimapSceneUiTimerRuntime& r, std::uint32_t p,
            const StringBufferState& t) : runtime(r), period(p), text(t) {}
        void RemoveCountdown(std::uint32_t owner) override {
            const auto queue=runtime.ReadWord(runtime.FrameManager()+0x1a0u);
            runtime.RemoveTimer(queue,owner+0xd0u);
        }
        void ScheduleCountdown(std::uint32_t owner) override {
            const auto queue=runtime.ReadWord(runtime.FrameManager()+0x1a0u);
            const auto callback=runtime.Allocate(0x10u);
            if(callback!=0u){
                runtime.WriteCallbackWord(callback,kCallbackVtable);
                runtime.WriteCallbackWord(callback+4u,owner);
                runtime.WriteCallbackWord(callback+8u,kExecuteFunction);
                runtime.WriteCallbackWord(callback+0xcu,0u);
            }
            TimerQueueEntryState entry{};
            runtime.CopyFloorAttribute(entry,owner+0xd0u);
            entry.initial_period_bits=0x3f800000u;
            entry.current_period_bits=0x3f800000u;
            entry.owns_callback=callback!=0u;
            entry.callback=callback;
            try {
                runtime.InsertTimer(queue,period,runtime.ReadWord(queue+4u),entry);
            } catch (...) {
                (void)TimerQueueEntry::Destroy(entry,runtime.EntryRuntime());
                throw;
            }
            (void)TimerQueueEntry::Destroy(entry,runtime.EntryRuntime());
        }
        void AssignCallback(std::uint32_t owner) override {runtime.AssignText(owner+0xe0u,text);}
        void ExecuteCountdown(std::uint32_t owner) override {runtime.ExecuteTimerScript(owner);}
    private:
        IMinimapSceneUiTimerRuntime& runtime;
        std::uint32_t period;
        const StringBufferState& text;
    } adapter(runtime,period,text);
    try {
        StartCountdown(state,owner,period,adapter);
    } catch (...) {
        (void)ReleaseIncomingText(text,runtime.EntryRuntime());
        throw;
    }
    return ReleaseIncomingText(text,runtime.EntryRuntime());
}

void MinimapSceneUiTimer::UpdateText(
    MinimapSceneUiState& state, const std::uint32_t owner,
    IMinimapSceneUiTimerRuntime& runtime) {
    class ClearRuntime final : public IMinimapCountdownClearRuntime {
    public:
        ClearRuntime(MinimapSceneUiState& s, IMinimapSceneUiTimerRuntime& r)
            : state(s), runtime(r) {}
        void RemoveCountdown(std::uint32_t owner) override {
            const auto queue=runtime.ReadWord(runtime.FrameManager()+0x1a0u);
            runtime.RemoveTimer(queue,owner+0xd0u);
        }
        void ClearCallbackAndText(std::uint32_t owner) override {
            const auto value = runtime.EmptyErrorText();
            if (value.size() > std::numeric_limits<std::uint32_t>::max() - 2u) {
                throw std::length_error("timer error text too long");
            }
            const auto length = static_cast<std::uint32_t>(value.size());
            const auto data = state.ReadWord(0xe4u);
            bool needs_copy = length != 0u;
            if (data != 0u) {
                const auto count = runtime.EntryRuntime().ReadReferenceCount(data);
                if (count != 0u && count != 0xffu) {
                    if (length == 0u) {
                        runtime.EntryRuntime().WriteReferenceCount(
                            data, static_cast<std::uint8_t>(count - 1u));
                        runtime.SetTextErrorFlag(state, owner + 0xe0u, 0u);
                        needs_copy = false;
                    } else {
                        runtime.ReallocateText(state, owner + 0xe0u, length);
                    }
                } else if (length == 0u) {
                    runtime.SetTextErrorFlag(state, owner + 0xe0u, 1u);
                } else if (state.ReadWord(0xecu) > 0x1fu ||
                           state.ReadWord(0xecu) < length) {
                    runtime.SetTextErrorFlag(state, owner + 0xe0u, 1u);
                    runtime.ReallocateText(state, owner + 0xe0u, length);
                }
            } else if (length == 0u) {
                runtime.SetTextErrorFlag(state, owner + 0xe0u, 1u);
            } else if (state.ReadWord(0xecu) > 0x1fu ||
                       state.ReadWord(0xecu) < length) {
                runtime.SetTextErrorFlag(state, owner + 0xe0u, 1u);
                runtime.ReallocateText(state, owner + 0xe0u, length);
            }
        
            if (needs_copy) {
                const auto destination = state.ReadWord(0xe4u);
                runtime.CopyText(destination, value);
                state.WriteWord(0xe8u, length);
                runtime.WriteByte(destination + length, 0u);
            }
        
            const auto displayed = runtime.EmptyErrorText();
            std::u32string widened;
            widened.reserve(displayed.size());
            for (const auto character : displayed) {
                widened.push_back(static_cast<unsigned char>(character));
            }
            runtime.SetWindowText(state.ReadWord(0xbcu), widened);
        }
    private:
        MinimapSceneUiState& state;
        IMinimapSceneUiTimerRuntime& runtime;
    } adapter(state,runtime);
    ClearCountdown(state,owner,adapter);
}

}  // namespace pal4
