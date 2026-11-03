#pragma once

#include <array>
#include <cstdint>

namespace pal4 {

struct GameWorldObjectState final {
    static constexpr std::uint32_t kWordCount = 49u;
    std::array<std::uint32_t, kWordCount> words{};
};

class IGameWorldDictionaryRuntime {
public:
    virtual ~IGameWorldDictionaryRuntime() = default;
    virtual void InitializeDictionary(GameWorldObjectState& state,
                                      std::uint32_t word_offset) = 0;
    virtual void CleanupDictionary(GameWorldObjectState& state,
                                   std::uint32_t word_offset) = 0;
};

class GameWorldObject final {
public:
    static constexpr std::uint32_t kVtableAddress = 0x84b818u;
    static constexpr std::uint32_t kDefaultParameterBits = 0x4117d70au;

    static GameWorldObjectState& Construct(GameWorldObjectState& state,
                                           IGameWorldDictionaryRuntime& runtime);
    static void Destruct(GameWorldObjectState& state,
                         IGameWorldDictionaryRuntime& runtime);
};

}  // namespace pal4
