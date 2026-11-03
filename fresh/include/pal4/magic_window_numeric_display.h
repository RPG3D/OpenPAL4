#pragma once

#include "pal4/magic_window_panel.h"

#include <cstdint>
#include <string_view>

namespace pal4 {

class IMagicWindowNumericRuntime {
public:
    virtual ~IMagicWindowNumericRuntime() = default;
    virtual std::uint32_t RoleId(std::uint32_t player) = 0;
    virtual std::int32_t QueryPlayerValue(std::uint32_t player,
                                           std::uint32_t role_id) = 0;
    virtual std::int32_t RemainingValue(std::uint32_t player,
                                         std::int32_t base_value) = 0;
    virtual std::int32_t TotalValue(std::uint32_t player,
                                     std::uint32_t category_key) = 0;
    virtual void SetText(std::uint32_t window, std::string_view text) = 0;
};

class MagicWindowNumericRuntimeStub : public IMagicWindowNumericRuntime {
public:
    std::uint32_t RoleId(std::uint32_t) override { return 0; }
    std::int32_t QueryPlayerValue(std::uint32_t, std::uint32_t) override { return 0; }
    std::int32_t RemainingValue(std::uint32_t, std::int32_t) override { return 0; }
    std::int32_t TotalValue(std::uint32_t, std::uint32_t) override { return 0; }
    void SetText(std::uint32_t, std::string_view) override {}
};

class MagicWindowNumericDisplay final {
public:
    static void Update(std::uint32_t player, std::uint32_t role_id,
                       std::uint32_t category_key,
                       const MagicWindowPanelState& panel,
                       IMagicWindowNumericRuntime& runtime);
};

}  // namespace pal4
