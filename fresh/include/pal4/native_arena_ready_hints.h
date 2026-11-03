#pragma once

#include "pal4/native_arena_ready_runtime.h"
#include <string>
#include <vector>

namespace pal4 {
class NativeCeguiUiSession;
class NativeGameDatabase;

// Scene database view and borrowed persistent HintWindow. The shared
// GameArenaReadyHandler retains matching, markup and ready-state ordering.
class NativeArenaReadyHints final : public INativeArenaReadyHints {
public:
    explicit NativeArenaReadyHints(NativeCeguiUiSession& ui) : ui_(ui) {}
    void BindDatabase(const NativeGameDatabase& database);
    std::uint32_t DatabaseAddress() const override;
    std::span<const ArenaReadyHintRecord> Records() override;
    std::uint32_t CreateHint() override;
    void Show(std::uint32_t window, std::string_view markup, float x, float y,
              float duration, std::uint32_t parent, std::uint32_t flag) override;
    std::int32_t ReleaseMatches() override;
    std::uint32_t Created() const { return created_; }
    std::uint32_t Released() const { return released_; }
    std::string_view ShownMarkup() const { return shown_markup_; }
private:
    NativeCeguiUiSession& ui_;
    const NativeGameDatabase* database_{};
    std::vector<ArenaReadyHintRecord> records_;
    std::string shown_markup_;
    std::uint32_t created_{}, released_{};
};
} // namespace pal4
