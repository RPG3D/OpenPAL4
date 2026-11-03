#include "pal4/smith_window.h"

#include <array>
#include <bit>
#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::ISmithWindowEffectPanelRuntime {
    static constexpr std::uint32_t object = 0x1000u;
    std::uint8_t shown = 0u;
    std::map<std::uint32_t, std::uint32_t> words{
        {object + 0x84u, 0x3000u},
        {object + 0x88u, 0x3004u},
        {object + 0x8cu, 0x3008u},
    };
    std::vector<std::string> calls;
    std::vector<std::pair<std::uint32_t, bool>> visibility;
    std::uint8_t result = 0x7bu;

    std::uint8_t ReadByte(std::uint32_t address) override {
        Check(address == object + 0x80u, "effect shown byte");
        calls.emplace_back("read-byte");
        return shown;
    }
    void WriteByte(std::uint32_t address,
                   std::uint8_t value) override {
        Check(address == object + 0x80u && value == 1u,
              "effect shown write");
        shown = value;
        calls.emplace_back("write-byte");
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.emplace_back("read-word");
        return words.at(address);
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
        calls.emplace_back("write-word");
    }
    void SetVisible(std::uint32_t window,
                    bool visible) override {
        visibility.emplace_back(window, visible);
        calls.emplace_back("visible");
    }
    std::uint32_t UIFrameManager() override {
        calls.emplace_back("manager");
        return 0x5000u;
    }
    void InsertFrameRecord(std::uint32_t array,
                           std::uint32_t record) override {
        Check(array == 0x5134u && record == object + 0xe8u,
              "effect frame record");
        calls.emplace_back("insert");
    }
    void HandleEffectEvent(std::uint32_t handle,
                           std::uint32_t event) override {
        Check(handle == object + 0xa4u && event == 1u,
              "prior effect event");
        calls.emplace_back("effect-event");
    }
    std::array<std::uint32_t, 2> CreateEffect(
        std::uint32_t manager, std::string_view name,
        const std::array<float, 3>& position,
        std::uint32_t sentinel,
        std::uint32_t flag) override {
        Check(manager == 0x5000u && name == "LH" &&
                  sentinel == 0xffffffffu && flag == 0u &&
                  std::bit_cast<std::uint32_t>(position[0]) ==
                      0xc26eae14u &&
                  std::bit_cast<std::uint32_t>(position[1]) ==
                      0x4348dc29u &&
                  std::bit_cast<std::uint32_t>(position[2]) ==
                      0x43952148u,
              "effect creation arguments");
        calls.emplace_back("create-effect");
        return {0xaaaau, 0xbbbbu};
    }
    void ResetTemporaryEffect() override {
        calls.emplace_back("reset-temporary");
    }
    std::uint8_t UpdateEffectProperties(
        std::uint32_t handle,
        const std::array<float, 3>& scale,
        std::uint8_t flag) override {
        Check(handle == object + 0xa4u &&
                  scale == (std::array<float, 3>{2.2f, 2.2f, 2.2f}) &&
                  flag == 0u,
              "effect properties arguments");
        calls.emplace_back("properties");
        return result;
    }
};

void CheckFirstShow() {
    Runtime runtime;
    Check(pal4::SmithWindow::ShowEffectPanel(
              Runtime::object, runtime) == 0x7bu &&
              runtime.shown == 1u &&
              runtime.visibility == (std::vector<
                  std::pair<std::uint32_t, bool>>{
                      {0x3000u, true}, {0x3004u, true},
                      {0x3008u, true}}),
          "first show forwards effect result and shows controls");
    Check(runtime.words.at(Runtime::object + 0xa4u) == 0xaaaau &&
              runtime.words.at(Runtime::object + 0xa8u) == 0xbbbbu &&
              runtime.calls.at(9) == "insert" &&
              runtime.calls.at(10) == "effect-event" &&
              runtime.calls.at(runtime.calls.size() - 2u) ==
                  "reset-temporary",
          "effect handle copy and call order");

    const auto calls_before = runtime.calls.size();
    Check(pal4::SmithWindow::ShowEffectPanel(
              Runtime::object, runtime) == 1u &&
              runtime.calls.size() == calls_before + 1u,
          "already shown returns state byte");
}

void CheckExistingNonBooleanFlag() {
    Runtime runtime;
    runtime.shown = 0xfeu;
    Check(pal4::SmithWindow::ShowEffectPanel(
              Runtime::object, runtime) == 0xfeu &&
              runtime.visibility.empty(),
          "non-Boolean shown byte preserved");
}

}  // namespace

int main() {
    try {
        CheckFirstShow();
        CheckExistingNonBooleanFlag();
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
