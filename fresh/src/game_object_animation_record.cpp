#include "pal4/game_object_animation_record.h"

#include "pal4/singleton_manager.h"

namespace pal4 {

void GameObjectAnimationRecord::Construct(
    GameObjectAnimationRecordState& record,
    const std::uint32_t record_address, const std::uint8_t stack_byte) {
    auto& words = record.words;
    for (const auto index : {0, 4, 9})
        words[index] = (words[index] & 0xffffff00u) | stack_byte;
    for (const auto index : {1, 2, 3, 5, 6, 7, 10, 11, 12})
        words[index] = 0;

    SingletonManagerState manager{words[13], words[14], words[15], words[16]};
    (void)SingletonManager(manager).Initialize(record_address + 52u);
    words[13] = manager.first_word;
    words[14] = manager.begin;
    words[15] = manager.end;
    words[16] = manager.capacity;
    words[8] = 0;
}

int GameObjectAnimationRecord::Cleanup(
    GameObjectAnimationRecordState& record,
    IGameObjectAnimationRecordRuntime& runtime) {
    auto& words = record.words;
    GameObjectAnimationEventArray::CleanupRange(words[10], words[11], runtime);
    words[11] = words[10];
    if (words[8] == 0) return 0;
    const auto result = runtime.FreeAnimationResource(words[8]);
    words[8] = 0;
    return result;
}

void GameObjectAnimationRecord::Destruct(
    GameObjectAnimationRecordState& record,
    IGameObjectAnimationRecordRuntime& runtime) {
    auto& words = record.words;
    (void)Cleanup(record, runtime);
    runtime.CleanupTailBuffer2(record);
    runtime.CleanupTailBuffer(record);
    GameObjectAnimationEventArray::CleanupRange(words[10], words[11], runtime);
    runtime.FreeMemory(words[10]);
    words[10] = words[11] = words[12] = 0;

    for (const auto first_word : {5, 1}) {
        const auto pointer = words[first_word];
        if (pointer != 0) {
            const auto count = runtime.ReadByte(pointer - 1u);
            if (count == 0 || count == 0xffu)
                runtime.FreeMemory(pointer - 1u);
            else
                runtime.WriteByte(pointer - 1u,
                                  static_cast<std::uint8_t>(count - 1u));
        }
        words[first_word] = words[first_word + 1] =
            words[first_word + 2] = 0;
    }
}

std::uint32_t GameObjectAnimationRecord::ResetEventTimes(
    GameObjectAnimationRecordState& record,
    IGameObjectAnimationEventMemory& runtime) {
    auto address = record.words[10];
    const auto end = record.words[11];
    if (end < address ||
        (end - address) % GameObjectAnimationEventArray::kEventStride != 0)
        return end;
    for (; address != record.words[11];
         address += GameObjectAnimationEventArray::kEventStride) {
        auto event = runtime.ReadEvent(address);
        (void)GameObjectAnimationEventData::NormalizeTime(event);
        runtime.WriteEvent(address, event);
    }
    return record.words[11];
}

}  // namespace pal4
