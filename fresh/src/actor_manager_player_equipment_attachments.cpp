#include "pal4/actor_manager_player_equipment_attachments.h"

namespace pal4 {

namespace {

using Attachments = ActorManagerPlayerEquipmentAttachments;

std::uint32_t TextOrEmpty(const std::uint32_t address,
                          IActorManagerPlayerEquipmentRuntime& runtime) {
    const auto text = runtime.ReadWord(address);
    return text ? text : Attachments::kEmptyText;
}

ActorManagerAttachmentRequest FromEquipment(
    const std::uint32_t record, const std::uint32_t primary,
    const std::uint32_t slot,
    IActorManagerPlayerEquipmentRuntime& runtime) {
    return {{primary, TextOrEmpty(record + 236u, runtime),
             TextOrEmpty(record + 204u, runtime),
             TextOrEmpty(record + 252u, runtime),
             TextOrEmpty(record + 316u, runtime)},
            slot, 1u};
}

ActorManagerAttachmentRequest StaticRequest(
    const std::uint32_t primary, const std::uint32_t secondary,
    const std::uint32_t slot) {
    return {{primary, secondary, Attachments::kEmptyError,
             Attachments::kEmptyError, Attachments::kEmptyError},
            slot, 1u};
}

ActorManagerAttachmentRequest SingleName(
    const std::uint32_t name, const std::uint32_t slot) {
    return {{name, Attachments::kEmptyError, Attachments::kEmptyError,
             Attachments::kEmptyError, Attachments::kEmptyError},
            slot, 1u};
}

ActorManagerAttachmentRequest Variant(
    const std::uint32_t record, const std::uint32_t size_offset,
    const std::uint32_t text_offset, const std::uint32_t slot,
    IActorManagerPlayerEquipmentRuntime& runtime) {
    const auto use_variant = runtime.ReadWord(record + size_offset) != 0u;
    const auto name = TextOrEmpty(record + (use_variant ? text_offset : 220u),
                                  runtime);
    return FromEquipment(record, name, slot, runtime);
}

}  // namespace

std::uint32_t ActorManagerPlayerEquipmentAttachments::Refresh(
    const std::uint32_t actor, const std::uint32_t animation,
    IActorManagerPlayerEquipmentRuntime& runtime) {
    const auto inventory_id = runtime.ReadWord(actor + 896u);
    const auto inventory = runtime.InventorySubsystem();
    const auto equipment = runtime.FindEquipment(inventory + 124u,
                                                  inventory_id, 7u);
    const auto slot = runtime.ReadWord(actor + 1192u);
    if (slot > 3u) return slot;

    if (slot == 0u) {
        const auto differs = runtime.TextDiffers(animation + 496u,
                                                  actor + 1144u);
        if (equipment && !differs) {
            const auto name = TextOrEmpty(equipment + 220u, runtime);
            return runtime.LoadLeft(
                animation,
                FromEquipment(equipment, name,
                              runtime.ReadWord(actor + 1192u), runtime));
        }
        if (!equipment && !differs)
            return runtime.LoadLeft(
                animation, SingleName(kWta,
                                      runtime.ReadWord(actor + 1192u)));
        const auto early = static_cast<std::int32_t>(
            runtime.ReadWord(kVersion)) < 120300;
        return runtime.LoadLeft(
            animation, StaticRequest(early ? kJq02a : kJqja,
                                     early ? kJq02 : kJqj1,
                                     runtime.ReadWord(actor + 1192u)));
    }

    if (equipment) {
        switch (slot) {
        case 1u:
            static_cast<void>(runtime.LoadLeft(
                animation, Variant(equipment, 288u, 284u, 1u, runtime)));
            return runtime.LoadRight(
                animation,
                Variant(equipment, 304u, 300u,
                        runtime.ReadWord(actor + 1192u), runtime));
        case 2u:
            return runtime.LoadLeft(
                animation,
                FromEquipment(equipment,
                              TextOrEmpty(equipment + 220u, runtime), 2u,
                              runtime));
        case 3u:
            return runtime.LoadRight(
                animation,
                FromEquipment(equipment,
                              TextOrEmpty(equipment + 220u, runtime), 3u,
                              runtime));
        default:
            return slot;
        }
    }

    switch (slot) {
    case 1u:
        static_cast<void>(runtime.LoadRight(animation,
                                            SingleName(kWl01, 1u)));
        return runtime.LoadLeft(
            animation, SingleName(kWl01,
                                  runtime.ReadWord(actor + 1192u)));
    case 2u:
        return runtime.LoadLeft(animation, SingleName(kWm01, 2u));
    case 3u:
        return runtime.LoadRight(animation, SingleName(kWta, 3u));
    default:
        return slot;
    }
}

}  // namespace pal4
