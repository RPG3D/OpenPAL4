#pragma once

#include <cstdint>

#include "pal4/pal_test_window.h"

namespace pal4 {

class IPalTestWindowActionsRuntime {
public:
    virtual ~IPalTestWindowActionsRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void SetCurrentQuestion(PalTestWindowState& state,
                                    std::uint32_t question) = 0;
    [[nodiscard]] virtual std::uint32_t SelectedAnswerIndex(
        PalTestWindowState& state) = 0;
    virtual void ProcessAnswer(PalTestWindowState& state) = 0;
    virtual void UpdateQuestionAndAnswers(PalTestWindowState& state) = 0;
    virtual void SetSelectedAnswer(PalTestWindowState& state,
                                   std::uint32_t answer) = 0;
};

class PalTestWindowActionsRuntimeStub :
    public IPalTestWindowActionsRuntime {
public:
    [[nodiscard]] std::uint32_t ReadWord(std::uint32_t) override {
        return 0u;
    }
    void SetCurrentQuestion(PalTestWindowState&,
                            std::uint32_t) override {}
    [[nodiscard]] std::uint32_t SelectedAnswerIndex(
        PalTestWindowState&) override { return 0u; }
    void ProcessAnswer(PalTestWindowState&) override {}
    void UpdateQuestionAndAnswers(PalTestWindowState&) override {}
    void SetSelectedAnswer(PalTestWindowState&,
                           std::uint32_t) override {}
};

class PalTestWindowActions final {
public:
    [[nodiscard]] static std::uint8_t OnCategoryClick(
        PalTestWindowState& state, std::uint32_t event,
        IPalTestWindowActionsRuntime& runtime);
    [[nodiscard]] static std::uint8_t OnOK(
        PalTestWindowState& state, IPalTestWindowActionsRuntime& runtime);
    [[nodiscard]] static std::uint8_t OnAnswerClick(
        PalTestWindowState& state, std::uint32_t event,
        IPalTestWindowActionsRuntime& runtime);
};

}  // namespace pal4
