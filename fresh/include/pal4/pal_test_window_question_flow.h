#pragma once

#include <cstdint>
#include <string_view>

#include "pal4/pal_test_window.h"

namespace pal4 {

class IPalTestWindowQuestionFlowRuntime {
public:
    virtual ~IPalTestWindowQuestionFlowRuntime() = default;
    [[nodiscard]] virtual std::uint32_t GameDB() = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void ResetQuestionCollection(PalTestWindowState& state) = 0;
    virtual void AppendQuestion(PalTestWindowState& state,
                                std::uint32_t record) = 0;
    virtual void SetEnabled(std::uint32_t window, bool enabled) = 0;
    virtual void SetSelected(std::uint32_t window, bool selected) = 0;
    virtual void SetVisible(std::uint32_t window, bool visible) = 0;
    virtual void UpdateQuestionAndAnswers(PalTestWindowState& state) = 0;
    [[nodiscard]] virtual std::uint32_t FrameManager() = 0;
    [[nodiscard]] virtual std::uint32_t InsertActive(
        std::uint32_t array, std::uint32_t member) = 0;
    [[nodiscard]] virtual std::uint32_t RemoveActive(
        std::uint32_t array, std::uint32_t member) = 0;
    virtual void SetCompletionFlag(std::uint32_t offset,
                                   std::uint32_t value) = 0;
    virtual void SaveSettings() = 0;
    virtual void PlayMusic(std::string_view name, bool loop) = 0;
    [[nodiscard]] virtual std::string_view TriggerItemText(
        std::uint32_t item) = 0;
    [[nodiscard]] virtual std::uint32_t ShowCommonDialog(
        std::uint32_t manager, std::string_view text, float width,
        float height, std::uint32_t mode, std::uint32_t flags) = 0;
};

class PalTestWindowQuestionFlowRuntimeStub :
    public IPalTestWindowQuestionFlowRuntime {
public:
    [[nodiscard]] std::uint32_t GameDB() override { return 0u; }
    [[nodiscard]] std::uint32_t ReadWord(std::uint32_t) override {
        return 0u;
    }
    void ResetQuestionCollection(PalTestWindowState&) override {}
    void AppendQuestion(PalTestWindowState&, std::uint32_t) override {}
    void SetEnabled(std::uint32_t, bool) override {}
    void SetSelected(std::uint32_t, bool) override {}
    void SetVisible(std::uint32_t, bool) override {}
    void UpdateQuestionAndAnswers(PalTestWindowState&) override {}
    [[nodiscard]] std::uint32_t FrameManager() override { return 0u; }
    [[nodiscard]] std::uint32_t InsertActive(
        std::uint32_t, std::uint32_t) override { return 0u; }
    [[nodiscard]] std::uint32_t RemoveActive(
        std::uint32_t, std::uint32_t) override { return 0u; }
    void SetCompletionFlag(std::uint32_t, std::uint32_t) override {}
    void SaveSettings() override {}
    void PlayMusic(std::string_view, bool) override {}
    [[nodiscard]] std::string_view TriggerItemText(
        std::uint32_t) override { return {}; }
    [[nodiscard]] std::uint32_t ShowCommonDialog(
        std::uint32_t, std::string_view, float, float,
        std::uint32_t, std::uint32_t) override { return 0u; }
};

class PalTestWindowQuestionFlow final {
public:
    [[nodiscard]] static std::uint32_t SetCurrentQuestion(
        PalTestWindowState& state, std::uint32_t object_address,
        std::uint32_t category, IPalTestWindowQuestionFlowRuntime& runtime);
    [[nodiscard]] static std::uint32_t ProcessAnswer(
        PalTestWindowState& state, std::uint32_t object_address,
        IPalTestWindowQuestionFlowRuntime& runtime);
    [[nodiscard]] static std::uint32_t ResetUI(
        PalTestWindowState& state, std::uint32_t object_address,
        IPalTestWindowQuestionFlowRuntime& runtime);
};

}  // namespace pal4
