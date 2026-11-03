#pragma once

#include <array>
#include <cstdint>

namespace pal4 {

class IGameDataMonsterCollectionRuntime {
public:
    virtual ~IGameDataMonsterCollectionRuntime() = default;
    [[nodiscard]] virtual std::uint32_t GetDatabase() = 0;
    [[nodiscard]] virtual bool StopSound(std::uint32_t database,
                                         std::uint32_t key) = 0;
    [[nodiscard]] virtual std::uint32_t PlaySound(std::uint32_t database,
                                                  std::uint32_t key) = 0;
    [[nodiscard]] virtual std::uint32_t ResolveSound(
        std::uint32_t token) = 0;
    [[nodiscard]] virtual bool StopMusic(std::uint32_t database,
                                         std::uint32_t key) = 0;
    [[nodiscard]] virtual std::uint32_t PlayMusic(std::uint32_t database,
                                                  std::uint32_t key) = 0;
    [[nodiscard]] virtual std::uint32_t ResolveMusic(
        std::uint32_t token) = 0;
    [[nodiscard]] virtual bool FadeIn(std::uint32_t record) = 0;
    [[nodiscard]] virtual std::uint32_t CollectionEnd(
        std::uint32_t collection) = 0;
    [[nodiscard]] virtual std::uint8_t DispatchSound(
        std::uint32_t collection, std::uint32_t collection_end,
        std::uint32_t record_value) = 0;
    [[nodiscard]] virtual std::uint32_t ReadRecordValue(
        std::uint32_t record) = 0;
    [[nodiscard]] virtual std::uint32_t InsertMusicAtFront(
        std::uint32_t collection, std::uint32_t record_value) = 0;
};

class GameDataMonsterCollectionProcessor final {
public:
    // IDA 0x4de590: scan five Stunt/Magic keys, append permanent attached
    // Skill IDs. Runtime method names below are historical, not audio behavior.
    [[nodiscard]] static std::uint8_t Process(
        const std::array<std::uint32_t, 5>& keys,
        std::uint32_t collection,
        IGameDataMonsterCollectionRuntime& runtime);
};

}  // namespace pal4
