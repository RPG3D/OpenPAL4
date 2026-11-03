#include "pal4/smith_window.h"

#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::ISmithWindowPrescriptionClickRuntime {
    static constexpr std::uint32_t object = 0x1000u;
    std::map<std::uint32_t, std::uint32_t> words{
        {object + 0x44u, 0x2000u},
        {object + 0x50u, 0x3000u},
        {object + 0x54u, 0x3004u},
        {object + 0xb4u, 1u},
        {0x4000u + 0x138u, 77u},
        {0x5000u + 0x18u, 0x7000u},
        {0x5000u + 0x28u, 0x7004u},
    };
    std::map<std::uint32_t, std::string> text;
    std::vector<std::string> calls;
    std::uint32_t hover = 0x4000u;
    bool has_recipe = true;

    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.emplace_back("read");
        return words.at(address);
    }
    void ClearEquipmentSelection(std::uint32_t owner) override {
        Check(owner == object, "clear owner");
        calls.emplace_back("clear");
    }
    void HideSelectionPanel(std::uint32_t owner) override {
        Check(owner == object, "hide owner");
        calls.emplace_back("hide");
    }
    std::uint32_t HoverItem(std::uint32_t list) override {
        Check(list == 0x2000u, "prescription list");
        calls.emplace_back("hover");
        return hover;
    }
    bool HasRecipe(std::uint32_t id) override {
        Check(id == 77u, "recipe check ID");
        calls.emplace_back("has-recipe");
        return has_recipe;
    }
    std::uint32_t RecipeForId(std::uint32_t id) override {
        Check(id == 77u, "recipe lookup ID");
        calls.emplace_back("recipe");
        return 0x5000u;
    }
    std::string_view Utf8Text(std::uint32_t source) override {
        Check(source == 0x7000u || source == 0x7004u,
              "recipe text source");
        calls.emplace_back("utf8");
        return source == 0x7000u ? "recipe" : "description";
    }
    void SetText(std::uint32_t window,
                 std::string_view value) override {
        text[window] = value;
        calls.emplace_back("text");
    }
    std::uint32_t EquipmentForId(std::uint32_t) override {
        throw std::runtime_error("nonzero mode skips equipment lookup");
    }
    std::string_view DescribeEquipment(std::uint32_t) override {
        throw std::runtime_error("nonzero mode skips description service");
    }
    std::string Utf8Buffer(std::string_view) override {
        throw std::runtime_error("nonzero mode skips buffer conversion");
    }
};

}  // namespace

int main() {
    try {
        Runtime found;
        Check(pal4::SmithWindow::OnPrescriptionListClick(
                  Runtime::object, 0x1234u, found) == 1u &&
                  found.text.at(0x3000u) == "recipe" &&
                  found.text.at(0x3004u) == "description" &&
                  found.calls.at(0) == "clear" &&
                  found.calls.at(1) == "hide" &&
                  found.calls.at(2) == "read" &&
                  found.calls.at(3) == "hover",
              "prescription click composes clear and text update");

        Runtime missing;
        missing.has_recipe = false;
        Check(pal4::SmithWindow::OnPrescriptionListClick(
                  Runtime::object, 0u, missing) == 1u &&
                  missing.text.at(0x3000u).empty() &&
                  missing.text.at(0x3004u).empty(),
              "missing database recipe clears details");

        Runtime no_hover;
        no_hover.hover = 0u;
        Check(pal4::SmithWindow::OnPrescriptionListClick(
                  Runtime::object, 0u, no_hover) == 1u &&
                  no_hover.text.at(0x3000u).empty() &&
                  no_hover.text.at(0x3004u).empty() &&
                  no_hover.calls.at(4) == "read",
              "absent hover skips database lookup");
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
