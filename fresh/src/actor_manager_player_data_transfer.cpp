#include "pal4/actor_manager_player_data_transfer.h"

#include <initializer_list>

namespace pal4 {

namespace {

constexpr std::uint32_t kEmptyText = 0x841964u;

void AssignLegacyFinalText(
    const std::uint32_t destination, const std::uint32_t source,
    IActorManagerPlayerDataTransferRuntime& runtime) {
    const auto length = runtime.ReadWord(source + 8u);
    if (destination == source) {
        runtime.EraseText(destination, length, 0xffffffffu);
        runtime.EraseText(destination, 0u, 0u);
        return;
    }

    std::uint32_t source_pointer = 0u;
    if (length) {
        source_pointer = runtime.ReadWord(source + 4u);
        if (!source_pointer) source_pointer = kEmptyText;
    }
    if (length && runtime.ReadByte(source_pointer - 1u) < 0xfeu) {
        const auto old_pointer = runtime.ReadWord(destination + 4u);
        if (old_pointer) {
            const auto old_header = old_pointer - 1u;
            const auto references = runtime.ReadByte(old_header);
            if (references == 0u || references == 0xffu)
                (void)runtime.Free(old_header);
            else
                runtime.WriteByte(old_header,
                                  static_cast<std::uint8_t>(references - 1u));
        }
        for (const auto offset : {4u, 8u, 12u})
            runtime.WriteWord(destination + offset, 0u);
        runtime.WriteWord(destination + 4u, source_pointer);
        runtime.WriteWord(destination + 8u, length);
        runtime.WriteWord(destination + 12u,
                          runtime.ReadWord(source + 12u));
        const auto header = source_pointer - 1u;
        runtime.WriteByte(header,
                          static_cast<std::uint8_t>(
                              runtime.ReadByte(header) + 1u));
        return;
    }

    if (runtime.AllocateText(destination, length, 1u)) {
        source_pointer = runtime.ReadWord(source + 4u);
        if (!source_pointer) source_pointer = kEmptyText;
        const auto buffer = runtime.ReadWord(destination + 4u);
        runtime.CopyBytes(buffer, source_pointer, length);
        runtime.SetTextSize(destination, length);
    }
}

}  // namespace

void IActorManagerPlayerDataTransferRuntime::AssignFinalText(
    std::uint32_t destination, std::uint32_t source) {
    AssignLegacyFinalText(destination, source, *this);
}

std::uint32_t ActorManagerPlayerDataTransfer::CopyFromSource(
    const std::uint32_t actor, const std::uint32_t source,
    IActorManagerPlayerDataTransferRuntime& runtime) {
    const auto source_actor = source + 1620u;
    runtime.CopyBytes(actor + 4u, source_actor + 4u, 0x150u);
    runtime.CopyBytes(actor + 340u, source_actor + 340u, 0x150u);
    runtime.CopyProperty2(actor + 676u, source_actor + 676u);
    runtime.CopyProperty(actor + 692u, source_actor + 692u);
    for (std::uint32_t offset = 708u; offset < 716u; offset += 4u)
        runtime.WriteWord(actor + offset,
                          runtime.ReadWord(source_actor + offset));

    runtime.CopyBytes(actor + 896u, source + 2336u, 24u);
    for (std::uint32_t i = 0u; i < 5u; ++i)
        runtime.CopyArray(actor + 920u + 16u * i,
                          source + 2360u + 16u * i);
    for (std::uint32_t i = 0u; i < 5u; ++i)
        runtime.CopyProperty(actor + 1000u + 16u * i,
                             source + 2440u + 16u * i);
    runtime.CopyArray(actor + 1080u, source + 2520u);
    runtime.CopyArray(actor + 1096u, source + 2536u);

    for (std::uint32_t i = 0u; i < 4u; ++i)
        runtime.AssignText(actor + 1112u + 16u * i,
                           source + 2552u + 16u * i);
    for (std::uint32_t offset = 0u; offset < 16u; offset += 4u)
        runtime.WriteWord(actor + 1176u + offset,
                          runtime.ReadWord(source + 2616u + offset));
    for (std::uint32_t offset = 0u; offset < 16u; offset += 4u)
        runtime.WriteWord(actor + 1192u + offset,
                          runtime.ReadWord(source + 2632u + offset));
    for (std::uint32_t i = 0u; i < 5u; ++i)
        runtime.AssignText(actor + 1208u + 16u * i,
                           source + 2648u + 16u * i);

    runtime.ActorSlot16(actor, 0u);
    runtime.WriteWord(actor + 336u, 0u);

    return runtime.FinishCopyFromSource(actor, source);
}

std::uint32_t IActorManagerPlayerDataTransferRuntime::FinishCopyFromSource(
    const std::uint32_t actor, const std::uint32_t source) {
    auto& runtime = *this;
    const auto temporary = runtime.TemporaryTextRecord();
    runtime.WriteByte(temporary, runtime.ReadByte(source + 2584u));
    runtime.SetErrorFlag(temporary, 0u);
    runtime.AssignText(temporary, source + 2584u);
    runtime.SetProperty278(actor, 2u, temporary);

    const auto text = runtime.ReadWord(temporary + 4u);
    if (!text) return 0u;
    const auto header = text - 1u;
    const auto references = runtime.ReadByte(header);
    if (references == 0u || references == 0xffu)
        return runtime.Free(header);
    runtime.WriteByte(header, static_cast<std::uint8_t>(references - 1u));
    return header;
}

std::uint32_t ActorManagerPlayerDataTransfer::CopyToPlayer(
    const std::uint32_t actor, const std::uint32_t player,
    IActorManagerPlayerDataTransferRuntime& runtime) {
    const auto destination = player + 1620u;
    const auto saved_state = runtime.ActorSlot20(destination);
    runtime.CopyBytes(destination + 4u, actor + 4u, 0x150u);
    runtime.CopyBytes(destination + 340u, actor + 340u, 0x150u);
    runtime.CopyProperty2(destination + 676u, actor + 676u);
    runtime.CopyProperty(destination + 692u, actor + 692u);
    for (std::uint32_t offset = 708u; offset < 716u; offset += 4u)
        runtime.WriteWord(destination + offset,
                          runtime.ReadWord(actor + offset));

    runtime.CopyBytes(player + 2336u, actor + 896u, 24u);
    for (std::uint32_t i = 0u; i < 5u; ++i)
        runtime.CopyArray(player + 2360u + 16u * i,
                          actor + 920u + 16u * i);
    for (std::uint32_t i = 0u; i < 5u; ++i)
        runtime.CopyProperty(player + 2440u + 16u * i,
                             actor + 1000u + 16u * i);
    runtime.CopyArray(player + 2520u, actor + 1080u);
    runtime.CopyArray(player + 2536u, actor + 1096u);

    for (std::uint32_t i = 0u; i < 4u; ++i)
        runtime.AssignText(player + 2552u + 16u * i,
                           actor + 1112u + 16u * i);
    for (std::uint32_t offset = 0u; offset < 16u; offset += 4u)
        runtime.WriteWord(player + 2616u + offset,
                          runtime.ReadWord(actor + 1176u + offset));
    for (std::uint32_t offset = 0u; offset < 16u; offset += 4u)
        runtime.WriteWord(player + 2632u + offset,
                          runtime.ReadWord(actor + 1192u + offset));
    for (std::uint32_t i = 0u; i < 5u; ++i)
        runtime.AssignFinalText(player + 2648u + 16u * i,
                                actor + 1208u + 16u * i);

    runtime.CleanupObjectLists(destination);
    runtime.ProcessCollections(player);
    runtime.ResetObjectProperties(destination);
    runtime.ActorSlot276(destination);
    if (!runtime.ActorSlot52(destination, 7u))
        runtime.ActorSlot68(destination, 7u, 1u);
    return runtime.ActorSlot24(destination, saved_state);
}

}  // namespace pal4
