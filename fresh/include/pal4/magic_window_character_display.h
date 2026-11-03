#pragma once

#include "pal4/magic_window.h"
#include "pal4/magic_window_panel.h"

#include <cstdint>
#include <optional>
#include <string_view>

namespace pal4 {

struct MagicPlayerSnapshot {
    std::uint32_t portrait_id{};
    bool active{};
};

class IMagicWindowCharacterDisplayRuntime {
public:
    virtual ~IMagicWindowCharacterDisplayRuntime() = default;
    virtual std::optional<MagicPlayerSnapshot> GetPlayer(
        std::int32_t index) = 0;
    virtual void SetPortrait(std::uint32_t window,
                             std::string_view image_set,
                             std::string_view image_name) = 0;
    virtual void InitializeMagicImages() = 0;
    virtual void UpdateMagicImages() = 0;
    virtual void SetVisible(std::uint32_t window, bool visible) = 0;
};

class MagicWindowCharacterDisplayRuntimeStub :
    public IMagicWindowCharacterDisplayRuntime {
public:
    std::optional<MagicPlayerSnapshot> GetPlayer(std::int32_t) override {
        return std::nullopt;
    }
    void SetPortrait(std::uint32_t, std::string_view,
                     std::string_view) override {}
    void InitializeMagicImages() override {}
    void UpdateMagicImages() override {}
    void SetVisible(std::uint32_t, bool) override {}
};

class MagicWindowCharacterDisplay final {
public:
    static void Update(MagicWindowState& state,
                       const MagicWindowPanelState& panel,
                       std::int32_t character_index,
                       IMagicWindowCharacterDisplayRuntime& runtime);
};

}  // namespace pal4
