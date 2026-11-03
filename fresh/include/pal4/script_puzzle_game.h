#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace pal4 {

// The 32-byte wait handler shared by the two puzzle script entries.
struct PuzzleWaitState {
    std::uint32_t vtable_address{};
    std::array<std::byte, 28> base{};
};
static_assert(sizeof(PuzzleWaitState) == 0x20u);

// Native XML document, puzzle UI and script-queue operations.
class IScriptPuzzleRuntime {
public:
    virtual ~IScriptPuzzleRuntime() = default;
    virtual void ClearPuzzleUiFlag() = 0;
    [[nodiscard]] virtual std::uint32_t CreateDocument() = 0;
    virtual void UpsertIntAttribute(std::uint32_t document,
                                    std::string_view key,
                                    std::int32_t value) = 0;
    virtual void EnterState(std::uint32_t state,
                            std::uint32_t document) = 0;
    [[nodiscard]] virtual PuzzleWaitState* AllocateWait(
        std::uint32_t bytes) = 0;
    [[nodiscard]] virtual std::uint32_t ScriptExecutor() = 0;
    virtual void ConstructWait(PuzzleWaitState& state,
                               std::uint8_t mode,
                               std::uint32_t queue) = 0;
    virtual void FlushWait(PuzzleWaitState& state) = 0;
    virtual void AddHandler(std::uint32_t queue,
                            PuzzleWaitState* state) = 0;
    [[nodiscard]] virtual std::uint32_t DestroyDocument(
        std::uint32_t document) = 0;
};

// Opens puzzle state eight from script and waits on its shared handler.
class ScriptPuzzleGame final {
public:
    static constexpr std::uint32_t kWaitVtable = 0x84960cu;
    static constexpr std::uint32_t kQueueOffset = 0x54u;

    explicit ScriptPuzzleGame(IScriptPuzzleRuntime& runtime)
        : runtime_(runtime) {}

    // 0x5e6f30: ordinary puzzle, Mode=0.
    [[nodiscard]] std::uint32_t StartPuzzle(std::int32_t type);
    // 0x5e7100: jigsaw puzzle, Mode=1.
    [[nodiscard]] std::uint32_t StartJigsaw(std::int32_t type);

private:
    [[nodiscard]] std::uint32_t Start(std::int32_t mode,
                                      std::int32_t type);

    IScriptPuzzleRuntime& runtime_;
};

}  // namespace pal4
