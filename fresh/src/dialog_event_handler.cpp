#include "pal4/dialog_event_handler.h"

namespace pal4 {
namespace {

std::uint32_t ConstructWithVtable(
    const std::uint32_t memory, const std::uint32_t callback,
    const std::uint32_t argument, const std::uint32_t owner,
    const std::uint32_t vtable, IDialogEventHandlerMemory& target) {
    target.WriteWord(memory + 8u, callback);
    target.WriteWord(memory + 12u, argument);
    target.WriteWord(memory + 16u, owner);
    target.WriteWord(memory, vtable);
    return memory;
}

}  // namespace

std::uint32_t DialogEventHandler::Construct(
    const std::uint32_t memory, const std::uint32_t callback,
    const std::uint32_t argument, const std::uint32_t owner,
    IDialogEventHandlerMemory& target) {
    return ConstructWithVtable(memory, callback, argument, owner,
                               kMessageBoxVtable, target);
}

std::uint32_t DialogEventHandler::ConstructAlt(
    const std::uint32_t memory, const std::uint32_t callback,
    const std::uint32_t argument, const std::uint32_t owner,
    IDialogEventHandlerMemory& target) {
    return ConstructWithVtable(memory, callback, argument, owner,
                               kYesNoVtable, target);
}

}  // namespace pal4
