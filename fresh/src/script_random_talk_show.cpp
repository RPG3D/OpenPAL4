#include "pal4/script_random_talk.h"
#include <stdexcept>

namespace pal4 {
std::uint8_t ScriptRandomTalk::Show(
    IScriptRandomTalkPoolView& pool, IScriptRandomTalkService& service) {
    auto* handler = service.AllocateHandler(kHandlerSize);
    if (handler != nullptr) {
        service.ConstructBase(*handler, 1u,
                               service.ScriptExecutor() + kQueueOffset);
        handler->vtable_address = kVtable;
        service.Flush(*handler);
    }
    service.AddHandler(service.ScriptExecutor() + kQueueOffset, handler);

    const auto count = pool.Count();
    if (count == 0u)
        throw std::domain_error("giRandTalk divides by zero for an empty pool");
    const auto selection = service.Random() % static_cast<std::int32_t>(count);
    const auto manager = service.UiFrameManager();
    const auto dialog = service.CreateScenarioDialog(manager);
    const auto visible = service.IsVisible(dialog);
    if (visible != 0u) return visible;
    const auto record = pool.Record(static_cast<std::uint32_t>(selection));
    return service.SetTextRecord(dialog, record, kDisplayValue);
}

} // namespace pal4
