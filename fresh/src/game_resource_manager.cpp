#include "pal4/game_resource_manager.h"

#include "pal4/config_data.h"

#include <bit>
#include <algorithm>

namespace pal4 {

GameResourceManager& GameResourceManager::Construct() {
    ConfigDataState config_data;
    (void)ConfigData::Initialize(config_data);
    std::copy(config_data.words.begin(), config_data.words.end(),
              words.begin() + 14);

    RwDictSchema schema;
    (void)schema.Initialize();
    words[37] = schema.dict;
    words[38] = schema.list;
    words[39] = std::bit_cast<std::uint32_t>(schema.time_scale);

    words[0] = 0x84b8c8u;
    for (std::size_t i = 46; i <= 51; ++i) words[i] = 0x3f800000u;
    for (std::size_t i = 1; i <= 9; ++i) words[i] = 0;
    words[10] = 1;
    for (std::size_t i = 11; i <= 13; ++i) words[i] = 0;
    for (std::size_t i = 73; i <= 75; ++i) words[i] = 0;
    for (std::size_t i = 40; i <= 45; ++i) words[i] = 0;

    GameConfigState config;
    (void)config.Initialize();
    std::copy(config.words.begin(), config.words.end(), words.begin() + 76);
    return *this;
}

void GameResourceManager::Destruct(IGameConfigMemory& memory,
                                   IRwDictSchemaRuntime& schema_runtime) {
    words[0] = 0x84b8c8u;
    for (std::size_t i = 1; i <= 4; ++i) {
        (void)GameConfigState::FreeAndNull(words[i], memory);
    }

    GameConfigState config;
    std::copy(words.begin() + 76, words.begin() + 92, config.words.begin());
    (void)config.Cleanup(memory);
    std::copy(config.words.begin(), config.words.end(), words.begin() + 76);

    RwDictSchema schema;
    schema.dict = words[37];
    schema.list = words[38];
    schema.time_scale = std::bit_cast<float>(words[39]);
    (void)schema.CleanupWrapper(schema_runtime);
    words[37] = schema.dict;
    words[38] = schema.list;
    // nullsub_91 at +14 has no side effects.
}

}  // namespace pal4
