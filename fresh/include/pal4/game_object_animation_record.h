#pragma once

#include "pal4/game_object_animation_event_array.h"
#include "pal4/game_object_animation_event_data.h"

#include <array>
#include <cstdint>

namespace pal4 {

struct GameObjectAnimationRecordState final {
    std::array<std::uint32_t, 17> words{};
};
static_assert(sizeof(GameObjectAnimationRecordState) == 0x44u);

class IGameObjectAnimationRecordRuntime
    : public IGameObjectAnimationEventArrayRuntime {
public:
    [[nodiscard]] virtual int FreeAnimationResource(std::uint32_t address) = 0;
    virtual void CleanupTailBuffer2(GameObjectAnimationRecordState& record) = 0;
    virtual void CleanupTailBuffer(GameObjectAnimationRecordState& record) = 0;
    virtual void FreeMemory(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
};

class GameObjectAnimationRecord final {
public:
    static void Construct(GameObjectAnimationRecordState& record,
                          std::uint32_t record_address,
                          std::uint8_t stack_byte);
    [[nodiscard]] static int Cleanup(
        GameObjectAnimationRecordState& record,
        IGameObjectAnimationRecordRuntime& runtime);
    static void Destruct(GameObjectAnimationRecordState& record,
                         IGameObjectAnimationRecordRuntime& runtime);
    [[nodiscard]] static std::uint32_t ResetEventTimes(
        GameObjectAnimationRecordState& record,
        IGameObjectAnimationEventMemory& runtime);
};

}  // namespace pal4
