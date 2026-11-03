#include "pal4/game_arena_load_handler.h"

namespace pal4 {
namespace {

void StoreInt(std::uint32_t document, std::string_view key,
              std::int32_t value, IGameArenaLoadRuntime& runtime) {
    if (const auto found = runtime.FindAttribute(document, key)) {
        runtime.SetInt(*found, value);
        return;
    }
    const auto attribute = runtime.InitializeIntAttribute(key, value);
    try {
        runtime.AddAttribute(document, attribute);
    } catch (...) {
        runtime.DestroyAttribute(attribute);
        throw;
    }
    runtime.DestroyAttribute(attribute);
}

void StoreString(std::uint32_t document, std::string_view key,
                 const char* value, IGameArenaLoadRuntime& runtime) {
    const std::string_view text = value ? value : "";
    if (const auto found = runtime.FindAttribute(document, key)) {
        runtime.SetString(*found, text);
        return;
    }
    const auto attribute = runtime.InitializeStringAttribute(key, text);
    try {
        runtime.AddAttribute(document, attribute);
    } catch (...) {
        runtime.DestroyAttribute(attribute);
        throw;
    }
    runtime.DestroyAttribute(attribute);
}

}  // namespace

std::int32_t GameArenaLoadHandler::Load(
    GameLoadArgument arena, GameLoadArgument area,
    GameLoadArgument area_resource, IGameArenaLoadRuntime& runtime) {
    const auto state = runtime.ReadWord(runtime.ArenaManager() + 0xc4u);
    if (state != 1u) return static_cast<std::int32_t>(state);

    runtime.WriteWord(runtime.ArenaManager() + 0xc4u, 0u);
    const auto document = runtime.InitializeXmlDocument();
    try {
        StoreInt(document, "PALGAMEIV_LOADING_TYPE", 0, runtime);
        StoreString(document, "PALGAMEIV_LOADING_ARENA", arena.value, runtime);
        StoreString(document, "PALGAMEIV_LOADING_AREA", area.value, runtime);
        StoreString(document, "PALGAMEIV_LOADING_AREA_RES",
                    area_resource.value, runtime);

        runtime.InitializeStateAndUi(runtime.GameInstance(), 6u, document);
        runtime.ProcessActorInput(runtime.ActorControl());
        runtime.ProcessActorInputAndClearVector(runtime.ActorControl());
        runtime.InitializeInputState(runtime.InputManager());

        const auto time = runtime.Milliseconds();
        runtime.WriteWord(runtime.GameState() + 0x56cu, time);
        runtime.WriteWord(runtime.GameState() + 0x574u, time);
        const auto copy = runtime.ReadWord(runtime.GameState() + 0x574u);
        runtime.WriteWord(runtime.GameState() + 0x574u, copy);
        runtime.WriteWord(runtime.GameState() + 0x570u, copy);
    } catch (...) {
        (void)runtime.DestroyXmlDocument(document);
        throw;
    }
    return runtime.DestroyXmlDocument(document);
}

}  // namespace pal4
