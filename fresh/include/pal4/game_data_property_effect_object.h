#pragma once

#include "pal4/game_data_ai_config_object.h"

#include <cstdint>

namespace pal4 {

class IGameDataPropertyEffectRuntime : public IGameDataAiConfigObjectRuntime {
public:
    virtual void DestroyTextRecord(std::uint32_t address) = 0;
};

class GameDataPropertyEffectObject final {
public:
    // IDA 0x4e57e0: allocate the 140-byte Property Effect object.
    [[nodiscard]] static std::uint32_t Create(
        std::uint32_t name, IGameDataAiConfigObjectRuntime& runtime);
    // IDA 0x4e58b0: release text records, fields and Effect base.
    [[nodiscard]] static std::uint32_t Destroy(
        std::uint32_t object, IGameDataPropertyEffectRuntime& runtime);
    // IDA 0x4e5a40: clear effects and destroy live Property records.
    static void ResetEffects(std::uint32_t object,
                             IGameDataPropertyEffectRuntime& runtime);
};

class GameDataPropertyRecordStorage final {
public:
    // IDA 0x4e5a00: destroy and free an array of 168-byte records.
    [[nodiscard]] static std::uint32_t Clear(
        std::uint32_t array, IGameDataPropertyEffectRuntime& runtime);
    // IDA 0x4e61d0: destroy a half-open range of 168-byte records.
    static void DestroyRange(std::uint32_t begin, std::uint32_t end,
                             IGameDataPropertyEffectRuntime& runtime);
    // IDA 0x4e6390: destroy seven text members in reverse order.
    static void DestroyRecord(std::uint32_t record,
                              IGameDataPropertyEffectRuntime& runtime);
};

}  // namespace pal4
