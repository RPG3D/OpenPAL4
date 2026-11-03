#pragma once

#include "pal4/magic_window.h"
#include "pal4/magic_window_panel.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace pal4 {

struct MagicDbStuntRecord {
    std::uint32_t key{};
    std::uint32_t kind{};
    std::uint32_t unknown_08{};
    std::uint32_t name_address{};
    std::array<std::byte, 0x1cu - 0x10u> unknown_10{};
    std::uint32_t title_address{};
    std::array<std::byte, 0x2cu - 0x20u> unknown_20{};
    std::uint32_t description_address{};
    std::array<std::byte, 0x48u - 0x30u> unknown_30{};
    std::array<std::uint32_t, 5> item_ids{};
    std::array<std::int32_t, 5> item_counts{};
    std::uint32_t effect_id{};
    std::int32_t required_stat{};
    std::int32_t counter_cost{};
    std::uint32_t special_item_id{};
    std::uint32_t condition_key{};
};

static_assert(sizeof(MagicDbStuntRecord) == 0x84u);
static_assert(offsetof(MagicDbStuntRecord, kind) == 4u);
static_assert(offsetof(MagicDbStuntRecord, name_address) == 0xcu);
static_assert(offsetof(MagicDbStuntRecord, title_address) == 0x1cu);
static_assert(offsetof(MagicDbStuntRecord, description_address) == 0x2cu);
static_assert(offsetof(MagicDbStuntRecord, item_ids) == 0x48u);
static_assert(offsetof(MagicDbStuntRecord, item_counts) == 0x5cu);
static_assert(offsetof(MagicDbStuntRecord, effect_id) == 0x70u);
static_assert(offsetof(MagicDbStuntRecord, required_stat) == 0x74u);
static_assert(offsetof(MagicDbStuntRecord, counter_cost) == 0x78u);
static_assert(offsetof(MagicDbStuntRecord, special_item_id) == 0x7cu);
static_assert(offsetof(MagicDbStuntRecord, condition_key) == 0x80u);

class IMagicWindowStuntImageRuntime {
public:
    virtual ~IMagicWindowStuntImageRuntime() = default;
    virtual void SetText(std::uint32_t window, std::string_view text) = 0;
    virtual void SetImage(std::uint32_t window, std::string_view imageset,
                          std::string_view image) = 0;
    virtual void SetVisible(std::uint32_t window, bool visible) = 0;
    virtual void SetStuntPanelVisible(std::uint32_t object, bool visible) = 0;
    virtual std::uint32_t GetPlayer(std::int32_t character_index) = 0;
    virtual void SetMagicPanelVisible(std::uint32_t object, bool visible) = 0;
    virtual std::vector<std::uint32_t> StuntKeys(std::uint32_t player) = 0;
    virtual std::optional<MagicDbStuntRecord> FindStunt(std::uint32_t key) = 0;
    virtual std::string ReadNameUtf8(std::uint32_t address) = 0;
    virtual void SetButtonKey(std::uint32_t button, std::uint32_t key) = 0;
    virtual void UpdateStuntDetails(std::uint32_t object) = 0;
};

class MagicWindowStuntImageRuntimeStub : public IMagicWindowStuntImageRuntime {
public:
    void SetText(std::uint32_t, std::string_view) override {}
    void SetImage(std::uint32_t, std::string_view, std::string_view) override {}
    void SetVisible(std::uint32_t, bool) override {}
    void SetStuntPanelVisible(std::uint32_t, bool) override {}
    std::uint32_t GetPlayer(std::int32_t) override { return 0; }
    void SetMagicPanelVisible(std::uint32_t, bool) override {}
    std::vector<std::uint32_t> StuntKeys(std::uint32_t) override { return {}; }
    std::optional<MagicDbStuntRecord> FindStunt(std::uint32_t) override {
        return std::nullopt;
    }
    std::string ReadNameUtf8(std::uint32_t) override { return {}; }
    void SetButtonKey(std::uint32_t, std::uint32_t) override {}
    void UpdateStuntDetails(std::uint32_t) override {}
};

class MagicWindowStuntImageUpdater final {
public:
    static constexpr std::size_t kVisibleLimit = 7u;

    static std::size_t Update(std::uint32_t object,
                              const MagicWindowState& state,
                              const MagicWindowPanelState& panel,
                              IMagicWindowStuntImageRuntime& runtime);
};

}  // namespace pal4
