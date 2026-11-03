#include "pal4/script_puzzle_game.h"

namespace pal4 {

std::uint32_t ScriptPuzzleGame::StartPuzzle(std::int32_t type) {
    return Start(0, type);
}

std::uint32_t ScriptPuzzleGame::StartJigsaw(std::int32_t type) {
    return Start(1, type);
}

std::uint32_t ScriptPuzzleGame::Start(std::int32_t mode,
                                      std::int32_t type) {
    runtime_.ClearPuzzleUiFlag();
    const auto document = runtime_.CreateDocument();
    try {
        runtime_.UpsertIntAttribute(document, "Mode", mode);
        runtime_.UpsertIntAttribute(document, "Type", type);
        runtime_.EnterState(8u, document);
        auto* state = runtime_.AllocateWait(sizeof(PuzzleWaitState));
        if (state != nullptr) {
            runtime_.ConstructWait(*state, 1u,
                                   runtime_.ScriptExecutor() + kQueueOffset);
            state->vtable_address = kWaitVtable;
            runtime_.FlushWait(*state);
        }
        runtime_.AddHandler(runtime_.ScriptExecutor() + kQueueOffset, state);
    } catch (...) {
        static_cast<void>(runtime_.DestroyDocument(document));
        throw;
    }
    return runtime_.DestroyDocument(document);
}

}  // namespace pal4
