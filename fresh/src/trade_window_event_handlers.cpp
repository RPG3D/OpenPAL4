#include "pal4/trade_window_event_handlers.h"

#include <new>

namespace pal4 {
namespace {

void ReleaseName(const std::uint32_t data,
                 ITradeWindowEventRuntime& runtime) {
    if (data == 0u) return;
    const auto count = runtime.ReadByte(data - 1u);
    if (count == 0u || count == 0xffu) {
        runtime.Free(data - 1u);
    } else {
        runtime.WriteByte(data - 1u,
                          static_cast<std::uint8_t>(count - 1u));
    }
}

std::uint32_t MakeCallback(const std::uint32_t frame,
                           const std::uint32_t handler,
                           const std::uint32_t event,
                           const std::uint8_t flag,
                           ITradeWindowEventRuntime& runtime) {
    const auto callback = runtime.Allocate(0x14u);
    if (callback != 0u) {
        try {
            runtime.WriteWord(callback + 4u, frame);
            runtime.WriteWord(callback + 8u, handler);
            runtime.WriteWord(callback,
                              TradeWindowEventHandlers::kCallbackVtable);
            runtime.WriteWord(callback + 0xcu, event);
            runtime.WriteByte(callback + 0x10u, flag);
        } catch (...) {
            runtime.Free(callback);
            throw;
        }
    }
    return callback;
}

void CopyTimerData(TimerQueueEntryState& destination,
                   TimerQueueEntryState& source,
                   ITradeWindowEventRuntime& runtime) {
    destination.text_flag = source.text_flag;
    destination.name = {};
    runtime.AssignTimerName(destination, source);
    destination.initial_period_bits = source.initial_period_bits;
    destination.current_period_bits = source.current_period_bits;
    destination.owns_callback = source.owns_callback;
    destination.callback = source.callback;
    source.owns_callback = false;
}

void AddHandler(const std::uint32_t queue, const TradeEventName& name,
                const std::uint32_t delay_bits, const std::uint32_t frame,
                const std::uint32_t handler, const std::uint32_t event,
                const std::uint8_t flag, const bool timer,
                ITradeWindowEventRuntime& runtime) {
    if (handler == 0u) return;
    const auto callback = MakeCallback(frame, handler, event, flag, runtime);
    const auto count_address = queue + (timer ? 8u : 0x14u);
    TradeEventData event_source{};
    TimerQueueEntryState timer_source{};
    std::uint32_t node = 0u;
    std::uint32_t head = 0u;
    bool linked = false;
    bool moved = false;
    std::uint32_t old_first = 0u;
    std::uint32_t old_first_next = 0u;
    try {
        head = runtime.ReadWord(queue + (timer ? 4u : 0x10u));
        if (timer) {
            timer_source.text_flag = name.text_flag;
            runtime.AssignTimerName(timer_source,
                TimerQueueEntryState{name.text_flag, name.name});
            timer_source.initial_period_bits = delay_bits;
            timer_source.current_period_bits = delay_bits;
            timer_source.owns_callback = callback != 0u;
            timer_source.callback = callback;
        } else {
            event_source.text.text_flag = name.text_flag;
            runtime.AssignName(event_source.text, name);
            event_source.delay_bits = delay_bits;
            event_source.owns_callback = callback != 0u;
            event_source.callback = callback;
        }
        old_first = runtime.ReadWord(head + 4u);
        if (old_first != 0u) old_first_next = runtime.ReadWord(old_first);
        node = runtime.Allocate(timer ? 0x28u : 0x24u);
        if (node == 0u) throw std::bad_alloc();
        runtime.WriteWord(node, head);
        runtime.WriteWord(node + 4u,
                          old_first != 0u ? old_first : node);
        runtime.WriteWord(head + 4u, node);
        runtime.WriteWord(runtime.ReadWord(node + 4u), node);
        linked = true;
        if (timer) {
            CopyTimerData(runtime.TimerDataAt(node + 8u),
                          timer_source, runtime);
        } else {
            (void)TradeWindowEventHandlers::CopyEventData(
                &runtime.EventDataAt(node + 8u), event_source, runtime);
        }
        moved = true;
        runtime.WriteWord(count_address,
                          runtime.ReadWord(count_address) + 1u);
    } catch (...) {
        if (linked) {
            runtime.WriteWord(head + 4u, old_first);
            if (old_first != 0u)
                runtime.WriteWord(old_first, old_first_next);
        }
        if (moved) {
            if (timer) {
                auto& data = runtime.TimerDataAt(node + 8u);
                if (data.owns_callback && data.callback != 0u)
                    runtime.ReleaseCallback(data.callback, 1u);
                ReleaseName(data.name.data, runtime);
            } else {
                (void)TradeWindowEventHandlers::CleanupEventData(
                    runtime.EventDataAt(node + 8u), runtime);
            }
        }
        if (node != 0u) runtime.Free(node);
        if (timer) {
            if (timer_source.owns_callback && callback != 0u)
                runtime.ReleaseCallback(callback, 1u);
            ReleaseName(timer_source.name.data, runtime);
        } else {
            if (event_source.owns_callback && callback != 0u)
                runtime.ReleaseCallback(callback, 1u);
            ReleaseName(event_source.text.name.data, runtime);
        }
        if (event_source.callback == 0u &&
            timer_source.callback == 0u && callback != 0u)
            runtime.ReleaseCallback(callback, 1u);
        throw;
    }
    if (timer) {
        if (timer_source.owns_callback && callback != 0u)
            runtime.ReleaseCallback(callback, 1u);
        ReleaseName(timer_source.name.data, runtime);
    } else {
        if (event_source.owns_callback && callback != 0u)
            runtime.ReleaseCallback(callback, 1u);
        ReleaseName(event_source.text.name.data, runtime);
    }
}

std::uint32_t CloneHandler(const std::uint32_t source,
                           const std::uint32_t base_vtable,
                           const std::uint32_t vtable,
                           ITradeWindowEventRuntime& runtime) {
    const auto result = runtime.Allocate(0xcu);
    if (result == 0u) return 0u;
    try {
        runtime.WriteWord(result, base_vtable);
        runtime.WriteWord(result + 4u, runtime.ReadWord(source + 4u));
        runtime.WriteWord(result + 8u, runtime.ReadWord(source + 8u));
        runtime.WriteWord(result, vtable);
    } catch (...) {
        runtime.Free(result);
        throw;
    }
    return result;
}

}  // namespace

void TradeWindowEventHandlers::AddEventHandler(
    const std::uint32_t queue, const TradeEventName& name,
    const std::uint32_t delay_bits, const std::uint32_t frame,
    const std::uint32_t handler, const std::uint32_t event,
    const std::uint8_t flag, ITradeWindowEventRuntime& runtime) {
    AddHandler(queue, name, delay_bits, frame, handler, event, flag,
               false, runtime);
}

void TradeWindowEventHandlers::AddTimerHandler(
    const std::uint32_t queue, const TradeEventName& name,
    const std::uint32_t delay_bits, const std::uint32_t frame,
    const std::uint32_t handler, const std::uint32_t event,
    const std::uint8_t flag, ITradeWindowEventRuntime& runtime) {
    AddHandler(queue, name, delay_bits, frame, handler, event, flag,
               true, runtime);
}

std::uint32_t TradeWindowEventHandlers::CopyEventData(
    TradeEventData* destination, TradeEventData& source,
    ITradeWindowEventRuntime& runtime) {
    if (destination == nullptr) return 0u;
    destination->text.text_flag = source.text.text_flag;
    destination->text.name = {};
    runtime.AssignName(destination->text, source.text);
    destination->delay_bits = source.delay_bits;
    destination->owns_callback = source.owns_callback;
    destination->callback = source.callback;
    source.owns_callback = false;
    return destination->callback;
}

std::uint32_t TradeWindowEventHandlers::CleanupEventData(
    TradeEventData& data, ITradeWindowEventRuntime& runtime) {
    if (data.owns_callback && data.callback != 0u)
        runtime.ReleaseCallback(data.callback, 1u);
    const auto name = data.text.name.data;
    std::uint32_t result = 0u;
    if (name != 0u) {
        const auto count = runtime.ReadByte(name - 1u);
        result = name - 1u;
        if (count == 0u || count == 0xffu) {
            runtime.Free(result);
            result = 0u;
        } else {
            runtime.WriteByte(result,
                              static_cast<std::uint8_t>(count - 1u));
        }
    }
    data.text.name = {};
    return result;
}

std::uint32_t TradeWindowEventHandlers::RemoveEventNode(
    const std::uint32_t queue, const std::uint32_t output,
    const std::uint32_t node, ITradeWindowEventRuntime& runtime) {
    const auto next = runtime.ReadWord(node);
    const auto previous = runtime.ReadWord(node + 4u);
    runtime.WriteWord(previous, next);
    runtime.WriteWord(next + 4u, previous);
    (void)CleanupEventData(runtime.EventDataAt(node + 8u), runtime);
    runtime.Free(node);
    runtime.WriteWord(queue + 8u, runtime.ReadWord(queue + 8u) - 1u);
    runtime.WriteWord(output, next);
    return output;
}

std::uint32_t TradeWindowEventHandlers::AllocateEventHandlerOne(
    const std::uint32_t source, ITradeWindowEventRuntime& runtime) {
    return CloneHandler(source, 0x843500u, kHandlerOneVtable, runtime);
}

std::uint32_t TradeWindowEventHandlers::AllocateEventHandlerTwo(
    const std::uint32_t source, ITradeWindowEventRuntime& runtime) {
    return CloneHandler(source, 0x8435d0u, kHandlerTwoVtable, runtime);
}

}  // namespace pal4
