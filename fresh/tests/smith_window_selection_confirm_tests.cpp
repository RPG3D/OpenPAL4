#include "pal4/smith_window.h"

#include <array>
#include <bit>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::ISmithWindowSelectionConfirmRuntime {
    static constexpr std::uint32_t object = 0x1000u;
    std::vector<std::string> calls;
    std::vector<std::uint32_t> templates;
    std::vector<std::string> messages;
    bool insufficient = false;
    bool consume_callback = false;
    bool throw_dialog = false;
    std::int32_t craft_result = 0;
    std::uint32_t destroyed_callback = 0u;
    std::uint32_t smith_registrations = 0u;

    std::int32_t PrescriptionHoverItemId(
        std::uint32_t owner) override {
        Check(owner == object, "prescription owner");
        calls.emplace_back("prescription-id");
        return 77;
    }
    std::int32_t EquipmentHoverItemId(
        std::uint32_t owner) override {
        Check(owner == object, "equipment owner");
        calls.emplace_back("equipment-id");
        return 88;
    }
    void RegisterSmithSystem() override {
        ++smith_registrations;
        calls.emplace_back("register");
    }
    std::uint32_t ElementProperty(
        std::int32_t id) override {
        Check(id == 77, "property prescription ID");
        calls.emplace_back("property");
        return 0x1234u;
    }
    bool CheckItemQuantity(std::uint32_t property) override {
        Check(property == 0x1234u, "quantity property");
        calls.emplace_back("check-quantity");
        return insufficient;
    }
    std::int32_t ProcessCrafting(
        std::int32_t prescription_id,
        std::int32_t equipment_id) override {
        Check(prescription_id == 77 && equipment_id == 88,
              "crafting IDs");
        calls.emplace_back("craft");
        return craft_result;
    }
    std::uint32_t UIFrameManager() override {
        calls.emplace_back("manager");
        return 0x5000u;
    }
    void CreateCommonDialog(std::uint32_t manager) override {
        Check(manager == 0x5000u, "dialog manager");
        calls.emplace_back("create-dialog");
    }
    std::string_view DialogTemplate(std::uint32_t id) override {
        Check(id == 0x73u || id == 0x33u, "dialog template ID");
        templates.push_back(id);
        calls.emplace_back("template");
        return id == 0x73u ? "missing resources" : "crafted %s";
    }
    std::string_view EquipmentName(
        std::uint32_t property) override {
        Check(property == 0x1234u, "equipment property");
        calls.emplace_back("name");
        return "sword";
    }
    std::string FormatMessage(std::string_view format,
                               std::string_view name) override {
        Check(format == "crafted %s" && name == "sword",
              "success format args");
        calls.emplace_back("format");
        return "crafted sword";
    }
    std::uint32_t CreateCloseCallback(
        std::uint32_t owner) override {
        Check(owner == object, "callback owner");
        calls.emplace_back("create-callback");
        return 0x6000u;
    }
    void ShowDialog(std::string_view message,
                    const std::array<float, 2>& size,
                    std::uint32_t style,
                    std::uint32_t& callback) override {
        Check(style == 4u &&
                  std::bit_cast<std::uint32_t>(size[0]) ==
                      0x3e99999au &&
                  std::bit_cast<std::uint32_t>(size[1]) ==
                      0x3e99999au,
              "dialog size and style");
        messages.emplace_back(message);
        calls.emplace_back("show-dialog");
        if (consume_callback) callback = 0u;
        if (throw_dialog) throw std::runtime_error("dialog failure");
    }
    void DestroyCallback(std::uint32_t callback,
                         std::uint32_t deleting) override {
        Check(callback == 0x6000u && deleting == 1u,
              "callback destructor args");
        destroyed_callback = callback;
        calls.emplace_back("destroy-callback");
    }
    void Call49F080(std::uint32_t owner) override {
        Check(owner == object, "49F080 owner");
        calls.emplace_back("49F080");
    }
    void Call49EF80(std::uint32_t owner) override {
        Check(owner == object, "49EF80 owner");
        calls.emplace_back("49EF80");
    }
    void HideSelectionPanel(std::uint32_t owner) override {
        Check(owner == object, "hide owner");
        calls.emplace_back("hide");
    }
};

void CheckEventGateAndInsufficientQuantity() {
    Runtime ignored;
    Check(pal4::SmithWindow::OnSelectionConfirm(
              Runtime::object, 2u, ignored) == 1u &&
              ignored.calls.empty(),
          "other events return one without side effects");

    Runtime missing;
    missing.insufficient = true;
    Check(pal4::SmithWindow::OnSelectionConfirm(
              Runtime::object, 1u, missing) == 1u &&
              missing.templates == std::vector<std::uint32_t>{0x73u} &&
              missing.messages ==
                  std::vector<std::string>{"missing resources"} &&
              missing.smith_registrations == 2u &&
              missing.calls.back() == "show-dialog",
          "missing resources dialog stops without refresh or hide");
}

void CheckCraftingResults() {
    Runtime failed;
    failed.craft_result = -1;
    Check(pal4::SmithWindow::OnSelectionConfirm(
              Runtime::object, 1u, failed) == 1u &&
              failed.templates.empty() &&
              failed.smith_registrations == 3u &&
              failed.calls.at(failed.calls.size() - 3u) == "49F080" &&
              failed.calls.at(failed.calls.size() - 2u) == "49EF80" &&
              failed.calls.back() == "hide",
          "failed craft still refreshes and hides selection");

    Runtime success;
    Check(pal4::SmithWindow::OnSelectionConfirm(
              Runtime::object, 1u, success) == 1u &&
              success.templates == std::vector<std::uint32_t>{0x33u} &&
              success.messages ==
                  std::vector<std::string>{"crafted sword"} &&
              success.smith_registrations == 4u &&
              success.destroyed_callback == 0x6000u &&
              success.calls.back() == "hide",
          "successful craft formats equipment name and disposes callback");

    Runtime transferred;
    transferred.consume_callback = true;
    (void)pal4::SmithWindow::OnSelectionConfirm(
        Runtime::object, 1u, transferred);
    Check(transferred.destroyed_callback == 0u,
          "dialog may take ownership of callback");

    Runtime throwing;
    throwing.throw_dialog = true;
    bool caught = false;
    try {
        (void)pal4::SmithWindow::OnSelectionConfirm(
            Runtime::object, 1u, throwing);
    } catch (const std::runtime_error&) {
        caught = true;
    }
    Check(caught && throwing.destroyed_callback == 0x6000u,
          "callback released when success dialog throws");
}

}  // namespace

int main() {
    try {
        CheckEventGateAndInsufficientQuantity();
        CheckCraftingResults();
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
