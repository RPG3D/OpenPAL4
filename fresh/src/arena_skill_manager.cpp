#include "pal4/arena_skill_manager.h"
#include "pal4/record_array12.h"

#include <bit>
#include <cmath>

namespace pal4 {
namespace {

class SkillRecordArrayRuntime final : public IRecordArray12Runtime {
public:
    explicit SkillRecordArrayRuntime(IArenaSkillRuntime& runtime)
        : runtime_(runtime) {}
    [[nodiscard]] std::uint32_t ReadWord(std::uint32_t address) override {
        return runtime_.ReadWord(address);
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        runtime_.WriteWord(address, value);
    }
    [[nodiscard]] std::uint32_t Allocate(std::uint32_t size) override {
        return runtime_.Allocate(size);
    }
    void Free(std::uint32_t pointer) override {
        (void)runtime_.Free(pointer);
    }

private:
    IArenaSkillRuntime& runtime_;
};

bool QueueFadeRecord(const std::uint32_t manager,
                     const std::uint32_t record,
                     IArenaSkillRuntime& runtime) {
    SkillRecordArrayRuntime adapter(runtime);
    return RecordArray12::InsertValue(
        manager + 32u, runtime.ReadWord(manager + 40u), 1u,
        {0x3f800000u, 0u, record}, adapter);
}

void ReleaseSkillRecord(const std::uint32_t record,
                        IArenaSkillRuntime& runtime) {
    const auto actor = runtime.ReadWord(record + 32u);
    if (actor != 0u) {
        (void)ArenaActorResource::Cleanup(actor, runtime);
        (void)runtime.SkillCall(ArenaSkillCall::DestroyActor, {actor, 1u});
    }
    if (runtime.SkillCall(ArenaSkillCall::NeedsContentUpdate,
                          {record + 36u}) != 0u)
        (void)runtime.SkillCall(ArenaSkillCall::UpdateContent,
                                {record + 36u});
    runtime.WriteWord(record + 28u, 0u);
    (void)ArenaSkillManager::ResetContent(record, runtime);
    (void)runtime.Free(record);
}

std::uint32_t UpdateMinimapIcon(const std::uint32_t object,
                                const bool enabled, const bool treasure,
                                IArenaSkillRuntime& runtime) {
    if (runtime.ReadWord(object + 440u) != 0u ||
        runtime.ReadWord(object + 432u) != (treasure ? 4u : 1u) ||
        (treasure && runtime.ReadWord(object + 388u) == 0u))
        return object;
    const auto icon = treasure ? 3u : 2u;
    const auto name = runtime.ReadWord(object + 4u);
    if (enabled &&
        runtime.SkillCall(ArenaSkillCall::IsObjectVisible, {object}) != 0u &&
        (treasure || runtime.ReadWord(object + 388u) != 0u)) {
        const auto minimap = runtime.SkillCall(
            ArenaSkillCall::GetMinimapManager, {});
        const auto coordinates = runtime.SkillCall(
            ArenaSkillCall::ConvertGridCoordinates,
            {minimap, object + 172u});
        const auto x = static_cast<std::int32_t>(std::bit_cast<float>(
            runtime.ReadWord(coordinates)));
        const auto y = static_cast<std::int32_t>(std::bit_cast<float>(
            runtime.ReadWord(coordinates + 4u)));
        const auto target = runtime.SkillCall(
            ArenaSkillCall::GetMinimapManager, {});
        (void)runtime.SkillCall(treasure ? ArenaSkillCall::AddTreasureIcon :
                                          ArenaSkillCall::AddOreIcon,
                                {target, name, icon,
                                 static_cast<std::uint32_t>(x),
                                 static_cast<std::uint32_t>(y)});
    } else {
        const auto minimap = runtime.SkillCall(
            ArenaSkillCall::GetMinimapManager, {});
        (void)runtime.SkillCall(ArenaSkillCall::RemoveMinimapIcon,
                                {minimap, name, icon});
    }
    return object;
}

}  // namespace

std::uint32_t IArenaSkillRuntime::CreateArenaActor() {
    const auto actor = Allocate(0x1ecu);
    return actor ? ArenaActorResource::Construct(actor, *this) : 0u;
}

std::uint32_t ArenaSkillManager::Register(
    std::uint8_t& guard, std::uint32_t& registered,
    IArenaSkillRuntime& runtime) {
    if ((guard & 1u) != 0u) return registered;
    guard |= 1u;
    registered = runtime.RegisterSingleton(kSingletonName, kSingletonId,
                                           0x520f70u, 0x520fd0u);
    return registered;
}

std::uint32_t ArenaSkillManager::Create(IArenaSkillRuntime& runtime) {
    const auto manager = runtime.Allocate(kObjectSize);
    return manager == 0u ? 0u : Initialize(manager, runtime);
}

std::uint32_t ArenaSkillManager::Destroy(
    const std::uint32_t manager, IArenaSkillRuntime& runtime) {
    if (manager == 0u) return 0u;
    (void)Cleanup(manager, runtime);
    return runtime.Free(manager);
}

std::uint32_t ArenaSkillManager::Initialize(
    const std::uint32_t manager, IArenaSkillRuntime& runtime) {
    const auto high_byte = static_cast<std::uint8_t>(manager >> 24u);
    for (const auto offset : {0u, 16u, 32u})
        runtime.WriteByte(manager + offset, high_byte);
    for (const auto offset : {4u, 8u, 12u, 20u, 24u, 28u,
                              36u, 40u, 44u, 48u, 52u})
        runtime.WriteWord(manager + offset, 0u);
    return manager;
}

std::uint32_t ArenaSkillManager::Cleanup(
    const std::uint32_t manager, IArenaSkillRuntime& runtime) {
    std::uint32_t result = 0u;
    for (const auto offset : {36u, 20u, 4u}) {
        result = runtime.Free(runtime.ReadWord(manager + offset));
        for (auto field = offset; field < offset + 12u; field += 4u)
            runtime.WriteWord(manager + field, 0u);
    }
    return result;
}

std::uint32_t ArenaSkillManager::UpdateObjectList(
    const std::uint32_t manager, const std::uint32_t pass,
    IArenaSkillRuntime& runtime) {
    for (auto cursor = runtime.ReadWord(manager + 4u);
         cursor != runtime.ReadWord(manager + 8u); cursor += 4u) {
        const auto entry = runtime.ReadWord(cursor);
        const auto actor = runtime.ReadWord(entry + 32u);
        if (actor != 0u)
            (void)ArenaActorResource::RenderAndAnimate(actor, pass, runtime);
    }
    return runtime.ReadWord(manager + 8u);
}

std::uint32_t ArenaSkillManager::UpdateTimedObjects(
    const std::uint32_t manager, const float delta,
    IArenaSkillRuntime& runtime) {
    auto cursor = runtime.ReadWord(manager + 4u);
    while (cursor != runtime.ReadWord(manager + 8u)) {
        const auto record = runtime.ReadWord(cursor);
        if (runtime.ReadWord(record + 32u) != 0u)
            (void)ArenaActorResource::Update(delta, runtime);
        const auto duration_bits = runtime.ReadWord(record + 20u);
        const auto duration = std::bit_cast<float>(duration_bits);
        if (duration < 0.0f) {
            cursor += 4u;
            continue;
        }
        const auto elapsed = std::bit_cast<float>(runtime.ReadWord(record + 24u));
        const auto advanced = static_cast<float>(static_cast<double>(delta) +
                                                 static_cast<double>(elapsed));
        runtime.WriteWord(record + 24u,
                          std::bit_cast<std::uint32_t>(advanced));
        if (runtime.ReadWord(record) != 0u && !(advanced >= duration)) {
            cursor += 4u;
            continue;
        }

        runtime.WriteWord(record + 24u, duration_bits);
        runtime.WriteWord(record + 28u, 0u);
        if (runtime.SkillCall(ArenaSkillCall::NeedsContentUpdate,
                              {record + 36u}) != 0u)
            (void)runtime.SkillCall(ArenaSkillCall::UpdateContent,
                                    {record + 36u});
        const auto type = runtime.ReadWord(record + 4u);
        if (type == 1u || type == 3u) {
            const auto objects = runtime.SkillCall(
                ArenaSkillCall::GetGameObjectManager, {});
            (void)runtime.SkillCall(
                type == 1u ? ArenaSkillCall::ProcessMinimapOne :
                             ArenaSkillCall::ProcessMinimapThree,
                {objects, 0u});
        }
        if (runtime.ReadWord(record + 32u) != 0u) {
            if (!QueueFadeRecord(manager, record, runtime)) {
                cursor += 4u;
                continue;
            }
        } else {
            (void)ResetContent(record, runtime);
            (void)runtime.Free(record);
        }
        const auto end = runtime.ReadWord(manager + 8u);
        for (auto next = cursor + 4u; next != end; next += 4u)
            runtime.WriteWord(next - 4u, runtime.ReadWord(next));
        runtime.WriteWord(manager + 8u, end - 4u);
    }
    return UpdateFadeState(manager, delta, runtime);
}

std::uint32_t ArenaSkillManager::ResetContent(
    const std::uint32_t record, IArenaSkillRuntime& runtime) {
    return runtime.SkillCall(ArenaSkillCall::ResetEffectHandle,
                             {record + 36u});
}

std::uint32_t ArenaSkillManager::UpdateFadeState(
    const std::uint32_t manager, const float delta,
    IArenaSkillRuntime& runtime) {
    auto cursor = runtime.ReadWord(manager + 36u);
    std::uint32_t result = runtime.ReadWord(manager + 40u);
    while (cursor != runtime.ReadWord(manager + 40u)) {
        const auto duration_bits = runtime.ReadWord(cursor);
        const auto duration = std::bit_cast<float>(duration_bits);
        const auto old_elapsed = std::bit_cast<float>(
            runtime.ReadWord(cursor + 4u));
        const auto advanced = static_cast<double>(delta) + old_elapsed;
        const bool complete = advanced >= duration;
        runtime.WriteWord(cursor + 4u,
                          complete ? duration_bits :
                          std::bit_cast<std::uint32_t>(
                              static_cast<float>(advanced)));
        const auto elapsed = std::bit_cast<float>(
            runtime.ReadWord(cursor + 4u));
        const auto fraction = elapsed / duration;
        const auto interpolated_bits = runtime.SkillCall(
            ArenaSkillCall::InterpolateAlpha,
            {0x3f800000u, 0u, std::bit_cast<std::uint32_t>(fraction), 0u});
        const auto interpolated = std::bit_cast<float>(interpolated_bits);
        const auto alpha = static_cast<std::uint8_t>(
            static_cast<std::int32_t>(interpolated * 255.0f + 0.5f));
        const auto color = static_cast<std::uint32_t>(alpha) * 0x01010101u;
        const auto record = runtime.ReadWord(cursor + 8u);
        const auto actor = runtime.ReadWord(record + 32u);
        const auto clump = runtime.ReadWord(actor + 20u);
        result = runtime.SkillCall(ArenaSkillCall::SetMaterialFlags,
                                   {clump, color});
        if (!complete) {
            cursor += 12u;
            continue;
        }
        (void)runtime.SkillCall(ArenaSkillCall::SetMaterialFlags,
                                {runtime.ReadWord(
                                     runtime.ReadWord(record + 32u) + 20u),
                                 color});
        const auto arena = runtime.SkillCall(
            ArenaSkillCall::GetGameArenaManager, {});
        (void)runtime.SkillCall(ArenaSkillCall::RemoveClump,
                                {arena, runtime.ReadWord(
                                    runtime.ReadWord(record + 32u) + 20u)});
        (void)ArenaActorResource::Cleanup(runtime.ReadWord(record + 32u),
                                          runtime);
        const auto active_actor = runtime.ReadWord(record + 32u);
        if (active_actor != 0u)
            (void)runtime.SkillCall(ArenaSkillCall::DestroyActor,
                                    {active_actor, 1u});
        if (record != 0u) {
            (void)ResetContent(record, runtime);
            (void)runtime.Free(record);
        }
        const auto end = runtime.ReadWord(manager + 40u);
        result = cursor + 12u;
        for (auto next = cursor + 12u; next != end; next += 12u) {
            for (const auto offset : {0u, 4u, 8u})
                runtime.WriteWord(next - 12u + offset,
                                  runtime.ReadWord(next + offset));
        }
        runtime.WriteWord(manager + 40u, end - 12u);
    }
    return result;
}

std::uint32_t ArenaSkillManager::ClearActiveObjects(
    const std::uint32_t manager, IArenaSkillRuntime& runtime) {
    for (auto cursor = runtime.ReadWord(manager + 4u);
         cursor != runtime.ReadWord(manager + 8u); cursor += 4u)
        ReleaseSkillRecord(runtime.ReadWord(cursor), runtime);
    runtime.WriteWord(manager + 8u, runtime.ReadWord(manager + 4u));

    for (auto cursor = runtime.ReadWord(manager + 20u);
         cursor != runtime.ReadWord(manager + 24u); cursor += 4u)
        ReleaseSkillRecord(runtime.ReadWord(cursor), runtime);
    runtime.WriteWord(manager + 24u, runtime.ReadWord(manager + 20u));

    const auto old_end = runtime.ReadWord(manager + 40u);
    for (auto cursor = runtime.ReadWord(manager + 36u);
         cursor != runtime.ReadWord(manager + 40u); cursor += 12u)
        ReleaseSkillRecord(runtime.ReadWord(cursor + 8u), runtime);
    runtime.WriteWord(manager + 40u, runtime.ReadWord(manager + 36u));
    runtime.WriteWord(manager + 48u, 0u);
    runtime.WriteWord(manager + 52u, 0u);
    return old_end;
}

std::uint32_t ArenaSkillManager::HandlePlayerAction(
    const std::uint32_t manager, const std::uint32_t player_index,
    IArenaSkillRuntime& runtime) {
    const auto players = runtime.SkillCall(ArenaSkillCall::GetPlayerManager, {});
    const auto player = runtime.SkillCall(ArenaSkillCall::GetPlayerByIndex,
                                          {players, player_index});
    if (player == 0u || player_index > 3u) return 0u;

    if (player_index == 1u || player_index == 3u) {
        const auto objects = runtime.SkillCall(
            ArenaSkillCall::GetGameObjectManager, {});
        (void)runtime.SkillCall(
            player_index == 1u ? ArenaSkillCall::ProcessMinimapOne :
                                 ArenaSkillCall::ProcessMinimapThree,
            {objects, 1u});
    }

    const auto record = runtime.Allocate(44u);
    if (record == 0u) return 0u;
    (void)runtime.SkillCall(ArenaSkillCall::InitializeRecordName,
                            {record + 36u});
    runtime.WriteWord(record, 1u);
    runtime.WriteWord(record + 4u, player_index);
    for (auto offset = 0u; offset < 12u; offset += 4u)
        runtime.WriteWord(record + 8u + offset,
                          runtime.ReadWord(player + 172u + offset));
    runtime.WriteWord(record + 20u, std::bit_cast<std::uint32_t>(10.0f));
    runtime.WriteWord(record + 24u, 0u);
    runtime.WriteWord(record + 28u, 0u);
    runtime.WriteWord(record + 32u, 0u);

    if (player_index == 0u) {
        const auto actor_storage = runtime.Allocate(0x1ecu);
        const auto actor = actor_storage == 0u ? 0u :
            ArenaActorResource::Construct(actor_storage, runtime);
        if (actor == 0u) {
            (void)ResetContent(record, runtime);
            (void)runtime.Free(record);
            return 0u;
        }
        runtime.WriteWord(record + 32u, actor);
        (void)runtime.SkillCall(ArenaSkillCall::LoadSnareResource,
                                {actor, runtime.ReadWord(manager + 48u)});
        (void)runtime.SkillCall(ArenaSkillCall::SetActorPosition,
                                {actor, record + 8u, 0u});
        (void)runtime.SkillCall(ArenaSkillCall::SetActorRotation,
                                {actor, 0u,
                                 runtime.ReadWord(player + 164u), 0u, 0u});
        (void)runtime.SkillCall(ArenaSkillCall::SetAnimationFlag,
                                {actor + 368u, 0u});
        (void)ResetContent(record, runtime);
        const auto arena = runtime.SkillCall(
            ArenaSkillCall::GetGameArenaManager, {});
        (void)runtime.SkillCall(ArenaSkillCall::AddClump,
                                {arena, runtime.ReadWord(actor + 20u)});
        runtime.WriteWord(manager + 48u,
                          runtime.ReadWord(manager + 48u) + 1u);
    } else if (player_index == 2u) {
        const auto effect = runtime.SkillCall(
            ArenaSkillCall::CreateEnthrallEffect,
            {record + 8u, 0xffffffffu, 0u});
        runtime.WriteWord(record + 36u, runtime.ReadWord(effect));
        runtime.WriteWord(record + 40u, runtime.ReadWord(effect + 4u));
        (void)runtime.SkillCall(ArenaSkillCall::ResetEffectHandle,
                                {effect});
    } else {
        (void)ResetContent(record, runtime);
    }

    (void)runtime.SkillCall(ArenaSkillCall::InsertActiveRecord,
                            {manager, runtime.ReadWord(manager + 8u),
                             1u, record});
    constexpr std::string_view sounds[] = {
        "WF032.wav", "WF033.wav", "WF034.wav", "WF035.wav"};
    runtime.PlayActionSound(sounds[player_index]);
    return 1u;
}

std::uint32_t ArenaSkillManager::ShowSignpost(
    const std::uint32_t manager, const std::uint32_t player_index,
    IArenaSkillRuntime& runtime) {
    if (static_cast<std::int32_t>(runtime.ReadWord(manager + 52u)) >= 10) {
        const auto begin = runtime.ReadWord(manager + 20u);
        const auto end = runtime.ReadWord(manager + 24u);
        if (begin != end) {
            const auto oldest = runtime.ReadWord(begin);
            runtime.WriteWord(oldest + 28u, 0u);
            if (runtime.SkillCall(ArenaSkillCall::NeedsContentUpdate,
                                  {oldest + 36u}) != 0u)
                (void)runtime.SkillCall(ArenaSkillCall::UpdateContent,
                                        {oldest + 36u});
            if (runtime.ReadWord(oldest + 32u) != 0u &&
                !QueueFadeRecord(manager, oldest, runtime))
                return 0u;
            for (auto cursor = begin + 4u; cursor != end; cursor += 4u)
                runtime.WriteWord(cursor - 4u, runtime.ReadWord(cursor));
            runtime.WriteWord(manager + 24u, end - 4u);
            runtime.WriteWord(manager + 52u,
                              runtime.ReadWord(manager + 52u) - 1u);
        }
    }

    const auto players = runtime.SkillCall(ArenaSkillCall::GetPlayerManager, {});
    const auto player = runtime.SkillCall(ArenaSkillCall::GetPlayerByIndex,
                                          {players, player_index});
    if (player == 0u) return 0u;
    const auto record = runtime.Allocate(44u);
    if (record == 0u) return 0u;
    // Native callbacks can throw. Keep the unqueued record safe to retire.
    runtime.WriteWord(record + 32u, 0u);
    runtime.WriteWord(record + 36u, 0u);
    runtime.WriteWord(record + 40u, 0u);
    try {
    (void)runtime.SkillCall(ArenaSkillCall::InitializeRecordName,
                            {record + 36u});
    runtime.WriteWord(record, 1u);
    runtime.WriteWord(record + 4u, 4u);
    for (auto offset = 0u; offset < 12u; offset += 4u)
        runtime.WriteWord(record + 8u + offset,
                          runtime.ReadWord(player + 172u + offset));
    runtime.WriteWord(record + 20u, std::bit_cast<std::uint32_t>(-1.0f));
    runtime.WriteWord(record + 24u, 0u);
    runtime.WriteWord(record + 28u, 0u);
    const auto effect = runtime.SkillCall(
        ArenaSkillCall::CreateSignpostEffect,
        {record + 8u, 0xffffffffu, 0u});
    runtime.WriteWord(record + 36u, runtime.ReadWord(effect));
    runtime.WriteWord(record + 40u, runtime.ReadWord(effect + 4u));
    (void)runtime.SkillCall(ArenaSkillCall::ResetEffectHandle, {effect});

    const auto actor = runtime.CreateArenaActor();
    if (actor == 0u) {
        (void)ResetContent(record, runtime);
        (void)runtime.Free(record);
        return 0u;
    }
    runtime.WriteWord(record + 32u, actor);
    (void)runtime.SkillCall(ArenaSkillCall::LoadSignpostResource,
                            {actor, runtime.ReadWord(manager + 52u) + 1u});
    (void)runtime.SkillCall(ArenaSkillCall::SetActorPosition,
                            {actor, record + 8u, 0u});
    (void)runtime.SkillCall(ArenaSkillCall::SetActorRotation,
                            {actor, 0u,
                             runtime.ReadWord(player + 164u), 0u, 0u});
    const auto arena = runtime.SkillCall(ArenaSkillCall::GetGameArenaManager,
                                         {});
    (void)runtime.SkillCall(ArenaSkillCall::AddClump,
                            {arena, runtime.ReadWord(actor + 20u)});

    auto begin = runtime.ReadWord(manager + 20u);
    auto end = runtime.ReadWord(manager + 24u);
    auto capacity = runtime.ReadWord(manager + 28u);
    if (end == capacity) {
        const auto count = begin == 0u ? 0u : (end - begin) / 4u;
        const auto new_capacity = count + (count > 1u ? count : 1u);
        const auto storage = runtime.Allocate(new_capacity * 4u);
        if (storage == 0u) {
            (void)runtime.SkillCall(ArenaSkillCall::RemoveClump,
                                    {arena, runtime.ReadWord(actor + 20u)});
            ReleaseSkillRecord(record, runtime);
            return 0u;
        }
        for (auto index = 0u; index < count; ++index)
            runtime.WriteWord(storage + index * 4u,
                              runtime.ReadWord(begin + index * 4u));
        (void)runtime.Free(begin);
        begin = storage;
        end = storage + count * 4u;
        capacity = storage + new_capacity * 4u;
        runtime.WriteWord(manager + 20u, begin);
        runtime.WriteWord(manager + 28u, capacity);
    }
    runtime.WriteWord(end, record);
    runtime.WriteWord(manager + 24u, end + 4u);
    runtime.WriteWord(manager + 52u,
                      runtime.ReadWord(manager + 52u) + 1u);
    return 1u;
    } catch (...) {
        ReleaseSkillRecord(record, runtime);
        throw;
    }
}

std::uint32_t ArenaSkillManager::CleanupPlayerData(
    const std::uint32_t manager, IArenaSkillRuntime& runtime) {
    for (auto cursor = runtime.ReadWord(manager + 4u);
         cursor != runtime.ReadWord(manager + 8u); cursor += 4u)
        ReleaseSkillRecord(runtime.ReadWord(cursor), runtime);
    runtime.WriteWord(manager + 8u, runtime.ReadWord(manager + 4u));

    for (auto cursor = runtime.ReadWord(manager + 20u);
         cursor != runtime.ReadWord(manager + 24u); cursor += 4u) {
        const auto record = runtime.ReadWord(cursor);
        if (record == 0u) continue;
        const auto actor = runtime.ReadWord(record + 32u);
        if (actor == 0u) continue;
        const auto arena = runtime.SkillCall(
            ArenaSkillCall::GetGameArenaManager, {});
        (void)runtime.SkillCall(ArenaSkillCall::RemoveClump,
                                {arena, runtime.ReadWord(actor + 20u)});
        if (runtime.SkillCall(ArenaSkillCall::NeedsContentUpdate,
                              {record + 36u}) != 0u)
            (void)runtime.SkillCall(ArenaSkillCall::UpdateContent,
                                    {record + 36u});
    }

    for (auto cursor = runtime.ReadWord(manager + 36u);
         cursor != runtime.ReadWord(manager + 40u); cursor += 12u)
        ReleaseSkillRecord(runtime.ReadWord(cursor + 8u), runtime);
    const auto old_end = runtime.ReadWord(manager + 40u);
    runtime.WriteWord(manager + 40u, runtime.ReadWord(manager + 36u));
    return old_end;
}

std::uint32_t ArenaSkillManager::UpdatePlayerData(
    const std::uint32_t manager, IArenaSkillRuntime& runtime) {
    auto cursor = runtime.ReadWord(manager + 20u);
    const auto initial_end = runtime.ReadWord(manager + 24u);
    if (cursor == initial_end) return initial_end;
    while (cursor != runtime.ReadWord(manager + 24u)) {
        const auto record = runtime.ReadWord(cursor);
        if (record != 0u) {
            const auto actor = runtime.ReadWord(record + 32u);
            if (actor != 0u) {
                const auto arena = runtime.SkillCall(
                    ArenaSkillCall::GetGameArenaManager, {});
                (void)runtime.SkillCall(ArenaSkillCall::AddClump,
                                        {arena, runtime.ReadWord(actor + 20u)});
                if (runtime.SkillCall(ArenaSkillCall::NeedsContentUpdate,
                                      {record + 36u}) != 0u) {
                    (void)runtime.SkillCall(ArenaSkillCall::UpdateContent,
                                            {record + 36u});
                    const auto effect = runtime.SkillCall(
                        ArenaSkillCall::CreateSignpostResumeEffect,
                        {record + 8u, 0xffffffffu, 0u});
                    runtime.WriteWord(record + 36u, runtime.ReadWord(effect));
                    runtime.WriteWord(record + 40u,
                                      runtime.ReadWord(effect + 4u));
                    (void)runtime.SkillCall(ArenaSkillCall::ResetEffectHandle,
                                            {effect});
                }
            }
        }
        cursor += 4u;
    }
    return manager;
}

std::uint32_t ArenaSkillManager::UpdateTreasureMinimap(
    const std::uint32_t object, const bool enabled,
    IArenaSkillRuntime& runtime) {
    return UpdateMinimapIcon(object, enabled, true, runtime);
}

std::uint32_t ArenaSkillManager::UpdateOreMinimap(
    const std::uint32_t object, const bool enabled,
    IArenaSkillRuntime& runtime) {
    return UpdateMinimapIcon(object, enabled, false, runtime);
}

}  // namespace pal4
