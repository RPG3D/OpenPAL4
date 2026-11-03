#include "pal4/ui_object_event_dispatcher.h"

namespace pal4 {

bool UiObjectEventDispatcher::Handle(
    std::uint32_t subobject_address, std::uint32_t event_address,
    IUiObjectEventRuntime& runtime) {
    if (runtime.ReadEventCode(event_address + 0x10u) != 1u) {
        return runtime.ForwardKeyEvent(subobject_address, event_address);
    }

    UiEventArgsRecord args{};
    args.vtable = runtime.EventArgsVtable();
    runtime.DispatchAction(subobject_address - 0x28u, args);
    return true;
}

}  // namespace pal4
