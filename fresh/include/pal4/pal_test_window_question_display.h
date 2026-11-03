#pragma once

#include <cstdint>
#include <string>
#include <string_view>

#include "pal4/pal_test_window.h"

namespace pal4 {

class IPalTestWindowQuestionDisplayRuntime {
public:
    virtual ~IPalTestWindowQuestionDisplayRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::string ReadCString(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::string ConvertToUtf8(std::string_view text) = 0;
    virtual void SetText(std::uint32_t window, std::u32string_view text) = 0;
    virtual void SetSelected(std::uint32_t window, bool selected) = 0;
    virtual void ProcessAnswer(PalTestWindowState& state) = 0;
};

class PalTestWindowQuestionDisplayRuntimeStub :
    public IPalTestWindowQuestionDisplayRuntime {
public:
    [[nodiscard]] std::uint32_t ReadWord(std::uint32_t) override {
        return 0u;
    }
    [[nodiscard]] std::uint8_t ReadByte(std::uint32_t) override {
        return 0u;
    }
    [[nodiscard]] std::string ReadCString(std::uint32_t) override {
        return {};
    }
    [[nodiscard]] std::string ConvertToUtf8(
        std::string_view text) override { return std::string(text); }
    void SetText(std::uint32_t, std::u32string_view) override {}
    void SetSelected(std::uint32_t, bool) override {}
    void ProcessAnswer(PalTestWindowState&) override {}
};

class PalTestWindowQuestionDisplay final {
public:
    static void Update(PalTestWindowState& state,
                       IPalTestWindowQuestionDisplayRuntime& runtime);
    static void SetSelectedAnswer(
        PalTestWindowState& state, std::uint32_t index,
        IPalTestWindowQuestionDisplayRuntime& runtime);
    [[nodiscard]] static std::int32_t SelectedAnswerIndex(
        const PalTestWindowState& state,
        IPalTestWindowQuestionDisplayRuntime& runtime);
};

}  // namespace pal4
