#pragma once

#include "pal4/magic_window.h"
#include "pal4/magic_window_stunt_image_updater.h"

#include <cstdint>
#include <optional>
#include <string_view>

namespace pal4 {

class IMagicWindowStuntSelectionRuntime {
public:
    virtual ~IMagicWindowStuntSelectionRuntime() = default;
    virtual bool HasStunt(std::uint32_t key) = 0;
    virtual std::uint32_t GetPlayer(std::int32_t index) = 0;
    virtual std::optional<MagicDbStuntRecord> FindStunt(std::uint32_t key) = 0;
    virtual std::int32_t PlayerValue(std::uint32_t player,
                                     std::uint32_t stat_index) = 0;
    virtual std::string_view TriggerText(std::uint32_t trigger_id) = 0;
    virtual void ShowStatFailure(std::string_view text, float x, float y,
                                 std::uint32_t style,
                                 std::uint32_t flags) = 0;
    virtual std::uint32_t AllocateCallback(std::uint32_t bytes) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    virtual void ShowCastDialog(bool visible, std::uint32_t& callback,
                                const MagicDbStuntRecord& stunt) = 0;
    virtual void DeleteCallback(std::uint32_t callback,
                                std::uint32_t flags) = 0;
};

class MagicWindowStuntSelectionRuntimeStub :
    public IMagicWindowStuntSelectionRuntime {
public:
    bool HasStunt(std::uint32_t) override { return false; }
    std::uint32_t GetPlayer(std::int32_t) override { return 0; }
    std::optional<MagicDbStuntRecord> FindStunt(std::uint32_t) override {
        return std::nullopt;
    }
    std::int32_t PlayerValue(std::uint32_t, std::uint32_t) override { return 0; }
    std::string_view TriggerText(std::uint32_t) override { return {}; }
    void ShowStatFailure(std::string_view, float, float,
                         std::uint32_t, std::uint32_t) override {}
    std::uint32_t AllocateCallback(std::uint32_t) override { return 0; }
    void WriteWord(std::uint32_t, std::uint32_t) override {}
    void ShowCastDialog(bool, std::uint32_t&,
                        const MagicDbStuntRecord&) override {}
    void DeleteCallback(std::uint32_t, std::uint32_t) override {}
};

class MagicWindowStuntSelection final {
public:
    static constexpr std::uint32_t kCastCallbackHandler = 0x45a100u;
    static constexpr std::uint32_t kCastCallbackVtable = 0x843914u;

    static bool Handle(std::uint32_t object, std::uint32_t stunt_key,
                       const MagicWindowState& state,
                       IMagicWindowStuntSelectionRuntime& runtime);
};

}  // namespace pal4
