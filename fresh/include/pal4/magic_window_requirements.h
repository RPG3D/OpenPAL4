#pragma once

#include "pal4/magic_window.h"
#include "pal4/magic_window_stunt_image_updater.h"

#include <cstdint>
#include <optional>

namespace pal4 {

struct MagicItemDefinition {
    std::uint32_t item_id{};
    std::uint32_t variant{};
};

class IMagicWindowRequirementsRuntime {
public:
    virtual ~IMagicWindowRequirementsRuntime() = default;
    virtual std::uint32_t GetPlayer(std::int32_t index) = 0;
    virtual bool ConditionContains(std::uint32_t player,
                                   std::uint32_t key) = 0;
    virtual bool SortedContains(std::uint32_t player,
                                std::uint32_t key) = 0;
    virtual std::int32_t PlayerValue(std::uint32_t player,
                                     std::uint32_t index) = 0;
    virtual bool HasCondition(std::uint32_t player,
                              std::uint32_t condition_key) = 0;
    virtual std::optional<MagicItemDefinition> LookupItem(
        std::uint32_t key) = 0;
    virtual std::optional<std::int32_t> InventoryCount(
        const MagicItemDefinition& item) = 0;
};

class MagicWindowRequirementsRuntimeStub : public IMagicWindowRequirementsRuntime {
public:
    std::uint32_t GetPlayer(std::int32_t) override { return 0; }
    bool ConditionContains(std::uint32_t, std::uint32_t) override { return false; }
    bool SortedContains(std::uint32_t, std::uint32_t) override { return false; }
    std::int32_t PlayerValue(std::uint32_t, std::uint32_t) override { return 0; }
    bool HasCondition(std::uint32_t, std::uint32_t) override { return false; }
    std::optional<MagicItemDefinition> LookupItem(std::uint32_t) override {
        return std::nullopt;
    }
    std::optional<std::int32_t> InventoryCount(
        const MagicItemDefinition&) override { return std::nullopt; }
};

class MagicWindowRequirements final {
public:
    static bool Check(const MagicWindowState& state,
                      const MagicDbStuntRecord& record,
                      IMagicWindowRequirementsRuntime& runtime);
};

}  // namespace pal4
