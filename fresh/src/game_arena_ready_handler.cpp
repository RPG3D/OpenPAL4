#include "pal4/game_arena_ready_handler.h"

#include "pal4/uppercase_string_pair.h"

#include <cctype>
#include <cmath>
#include <string>

namespace pal4 {
namespace {

bool SameId(std::string_view left, std::string_view right) {
    left = left.substr(0, left.find('\0'));
    right = right.substr(0, right.find('\0'));
    if (left.size() != right.size()) return false;
    for (std::size_t i = 0; i < left.size(); ++i) {
        if (std::tolower(static_cast<unsigned char>(left[i])) !=
            std::tolower(static_cast<unsigned char>(right[i]))) return false;
    }
    return true;
}

void SelectSpecialMinimap(std::string_view arena, std::string_view area,
                          IGameArenaReadyRuntime& runtime) {
    if (SameId(arena, "M15")) {
        if (!SameId(area, "2")) return;
        const auto y = runtime.LeaderY();
        const auto z = runtime.LeaderZ();
        const bool upper_region = y > 3960.0f;
        const bool narrow_region = z < -1400.0f &&
                                   y > 3550.0f && y < 3630.0f;
        runtime.SetMinimapTexture(upper_region || narrow_region ? 1u : 0u);
        return;
    }
    if (SameId(arena, "M08")) {
        if (!SameId(area, "2") && !SameId(area, "3") &&
            !SameId(area, "4")) return;
        const auto y = runtime.LeaderY();
        runtime.SetMinimapTexture(y < 130.0f || std::isnan(y) ? 1u : 0u);
        return;
    }
    if (SameId(arena, "Q06")) {
        if (!SameId(area, "Q06Q") && !SameId(area, "Q06QY")) return;
        const auto y = runtime.LeaderY();
        runtime.SetMinimapTexture(y < 0.0f || std::isnan(y) ? 1u : 0u);
        return;
    }
    if (SameId(arena, "Q14") && SameId(area, "Q14B")) {
        const auto y = runtime.LeaderY();
        runtime.SetMinimapTexture(y < 730.0f || std::isnan(y) ? 2u :
                                  y < 2616.0f ? 1u : 0u);
    }
}

}  // namespace

std::int32_t GameArenaReadyHandler::Run(IGameArenaReadyRuntime& runtime) {
    if (runtime.ReadWord(runtime.ActorControl()) != 0u &&
        runtime.ReadWord(runtime.ArenaManager() + 0xc4u) == 0u) {
        const auto area = std::string(runtime.ReadText(
            runtime.ArenaManager() + 0xe0u));
        const auto arena = std::string(runtime.ReadText(
            runtime.ArenaManager() + 8u));
        const UppercaseStringPair key(arena, area);
        try {
            const auto records = runtime.HintRecords(runtime.GameDatabase() + 52u);
            const ArenaReadyHintRecord* first_match = nullptr;
            for (const auto& record : records) {
                if (record.arena == key.First() && record.area == key.Second() &&
                    first_match == nullptr) first_match = &record;
            }
            if (first_match != nullptr) {
                const auto window = runtime.CreateHintWindow();
                if (window == 0u) return runtime.ReleaseHintMatches();
                const auto text = first_match->text.substr(
                    0, first_match->text.find('\0'));
                const auto markup = "<dc0><df2>" + std::string(text) +
                                    "</df2></dc0><br/>";
                runtime.ShowHint(window, markup, 0.5f, 0.15f, 1.0f, 0u, 1u);
            }
        } catch (...) {
            (void)runtime.ReleaseHintMatches();
            throw;
        }
        (void)runtime.ReleaseHintMatches();
    }

    runtime.ProcessActorInput(runtime.ActorControl());
    runtime.ProcessActorInputAndClearVector(runtime.ActorControl());
    runtime.InitializeInputState(runtime.InputManager());
    runtime.InitializePetData(runtime.PetManager());
    const auto actor = runtime.ReadWord(runtime.ActorControl() + 4u);
    if (actor != 0u)
        runtime.UpdateGrid(runtime.MinimapManager(), actor + 0xacu,
                           actor + 0xa0u);

    const auto arena = std::string(runtime.ReadText(
        runtime.ArenaManager() + 8u));
    const auto area = std::string(runtime.ReadText(
        runtime.ArenaManager() + 0xe0u));
    SelectSpecialMinimap(arena, area, runtime);

    const auto time = runtime.Milliseconds();
    runtime.WriteWord(runtime.GameState() + 0x56cu, time);
    runtime.WriteWord(runtime.GameState() + 0x574u, time);
    const auto copy = runtime.ReadWord(runtime.GameState() + 0x574u);
    runtime.WriteWord(runtime.GameState() + 0x574u, copy);
    runtime.WriteWord(runtime.GameState() + 0x570u, copy);
    runtime.WriteWord(runtime.ArenaManager() + 0xc4u, 1u);
    const auto manager = runtime.ArenaManager();
    runtime.WriteWord(manager + 0x1bcu, 0u);
    return static_cast<std::int32_t>(manager);
}

std::int32_t GameArenaReadyHandler::Restore(IGameArenaReadyRuntime& runtime) {
    const auto arena = runtime.ArenaManager();
    if (runtime.ReadWord(arena + 0x1bcu) != 0u) return Run(runtime);
    return static_cast<std::int32_t>(arena);
}

}  // namespace pal4
