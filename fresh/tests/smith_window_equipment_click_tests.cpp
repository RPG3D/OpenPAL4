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

struct Runtime final : pal4::ISmithWindowEquipmentClickRuntime {
    static constexpr std::uint32_t object = 0x1000u;
    std::map<std::uint32_t, std::uint32_t> words{
        {object + 0x40u, 0x2000u},
        {object + 0x48u, 0x3000u},
        {object + 0x4cu, 0x3004u},
        {0x4000u + 0x138u, 77u},
        {0x5000u + 0xacu, 0x7000u},
        {0x5000u + 0xbcu, 0u},
    };
    std::map<std::uint32_t, std::string> text;
    std::vector<std::string> calls;
    std::uint32_t hovered_item = 0x4000u;

    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.emplace_back("read");
        return words.at(address);
    }
    std::uint32_t HoverItem(std::uint32_t list) override {
        Check(list == 0x2000u, "equipment list");
        calls.emplace_back("hover");
        return hovered_item;
    }
    std::uint32_t FindInventoryItem(std::uint32_t id,
                                    std::uint32_t category) override {
        Check(category == 7u &&
                  (id == 77u || id == 0xffffffffu),
              "inventory item lookup");
        calls.emplace_back("find");
        return id == 77u ? 0x5000u : 0u;
    }
    std::string_view Utf8Text(std::uint32_t source) override {
        Check(source == 0x7000u, "name source");
        calls.emplace_back("utf8");
        return "sword";
    }
    void SetText(std::uint32_t window,
                 std::string_view value) override {
        text[window] = value;
        calls.emplace_back("text");
    }
    std::string_view DescribeItem(std::uint32_t item) override {
        Check(item == 0x5000u, "description item");
        calls.emplace_back("describe");
        return "description";
    }
    std::string_view ReadText(std::uint32_t source) override {
        Check(source == 0u, "extra text source");
        calls.emplace_back("read-text");
        return {};
    }
    std::string Utf8Buffer(std::string_view value) override {
        calls.emplace_back("utf8-buffer");
        return std::string(value);
    }
};

}  // namespace

int main() {
    try {
        Runtime selected;
        Check(pal4::SmithWindow::OnEquipmentListClick(
                  Runtime::object, 0x12345678u, selected) == 1u &&
                  selected.text.at(0x3000u) == "sword" &&
                  selected.text.at(0x3004u) == "description" &&
                  selected.calls.at(0) == "read" &&
                  selected.calls.at(1) == "hover" &&
                  selected.calls.at(2) == "read" &&
                  selected.calls.at(3) == "find",
              "equipment click composes hover lookup and detail update");

        Runtime empty;
        empty.hovered_item = 0u;
        Check(pal4::SmithWindow::OnEquipmentListClick(
                  Runtime::object, 0u, empty) == 1u &&
                  empty.text.at(0x3000u).empty() &&
                  empty.text.at(0x3004u).empty(),
              "absent equipment hover clears both details");
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
