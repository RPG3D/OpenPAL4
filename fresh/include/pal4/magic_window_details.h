#pragma once

#include "pal4/magic_window_panel.h"

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace pal4 {

struct MagicWindowDetailText {
    std::string title;
    std::string description;
};

class IMagicWindowDetailsRuntime {
public:
    virtual ~IMagicWindowDetailsRuntime() = default;
    virtual bool IsVisible(std::uint32_t window) = 0;
    virtual void SetSelected(std::uint32_t window, bool selected) = 0;
    virtual std::uint32_t ButtonKey(std::uint32_t window) = 0;
    virtual std::optional<MagicWindowDetailText> FindMagicDetail(
        std::uint32_t key) = 0;
    virtual std::optional<MagicWindowDetailText> FindStuntDetail(
        std::uint32_t key) = 0;
    virtual void SetText(std::uint32_t window, std::string_view text) = 0;
};

class MagicWindowDetailsRuntimeStub : public IMagicWindowDetailsRuntime {
public:
    bool IsVisible(std::uint32_t) override { return false; }
    void SetSelected(std::uint32_t, bool) override {}
    std::uint32_t ButtonKey(std::uint32_t) override { return 0; }
    std::optional<MagicWindowDetailText> FindMagicDetail(std::uint32_t) override {
        return std::nullopt;
    }
    std::optional<MagicWindowDetailText> FindStuntDetail(std::uint32_t) override {
        return std::nullopt;
    }
    void SetText(std::uint32_t, std::string_view) override {}
};

class MagicWindowDetails final {
public:
    static void UpdateMagic(std::int32_t index,
                            const MagicWindowPanelState& panel,
                            IMagicWindowDetailsRuntime& runtime);
    static void UpdateStunt(std::int32_t index,
                            const MagicWindowPanelState& panel,
                            IMagicWindowDetailsRuntime& runtime);
};

}  // namespace pal4
