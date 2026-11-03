#pragma once

#include "pal4/mission_window_lifecycle.h"

#include <cstdint>
#include <string>
#include <string_view>

namespace pal4 {

class IMissionWindowQuestDetailsRuntime {
public:
    virtual ~IMissionWindowQuestDetailsRuntime() = default;
    virtual void SetVisible(std::uint32_t window, bool visible) = 0;
    [[nodiscard]] virtual std::string QuestDescription(
        std::uint32_t quest, std::uint32_t offset) = 0;
    [[nodiscard]] virtual std::string ConvertToUtf8(
        std::string_view description) = 0;
    virtual void SetText(std::uint32_t window,
                         std::string_view utf8_text) = 0;
    [[nodiscard]] virtual std::uint32_t LoadQuestImage(
        std::uint32_t quest) = 0;
    virtual void SetImage(std::uint32_t window,
                          std::uint32_t image) = 0;
};

class MissionWindowQuestDetails final {
public:
    static void Update(const MissionWindowState& state,
                       std::uint32_t quest,
                       IMissionWindowQuestDetailsRuntime& runtime);
};

}  // namespace pal4
