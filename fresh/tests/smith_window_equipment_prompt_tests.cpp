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

struct Runtime final : pal4::ISmithWindowEquipmentPromptRuntime {
    static constexpr std::uint32_t object = 0x1000u;
    std::vector<std::string> calls;
    std::uint32_t assigned_destination = 0u;
    std::uint32_t assigned_source = 0u;
    std::uint32_t destroyed = 0u;
    std::string dialog;
    bool consume_callback = false;
    bool throw_dialog = false;

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
    std::uint32_t UIFrameManager() override {
        calls.emplace_back("manager");
        return 0x3000u;
    }
    void CreateCommonDialog(std::uint32_t manager) override {
        Check(manager == 0x3000u, "dialog manager");
        calls.emplace_back("create-dialog");
    }
    std::uint32_t CreateSelectCallback(
        std::uint32_t owner) override {
        Check(owner == object, "callback owner");
        calls.emplace_back("create-callback");
        return 0x4000u;
    }
    std::uint32_t CreatePageChangeCallback(
        std::uint32_t owner) override {
        Check(owner == object, "page callback owner");
        calls.emplace_back("create-page-callback");
        return 0x4000u;
    }
    std::uint32_t FindInventoryItem(std::uint32_t id,
                                    std::uint32_t category) override {
        Check(id == 88u && category == 7u,
              "selected inventory item");
        calls.emplace_back("find-item");
        return 0x5000u;
    }
    void AssignText(std::uint32_t destination,
                    std::uint32_t source) override {
        assigned_destination = destination;
        assigned_source = source;
        calls.emplace_back("assign-text");
    }
    std::string_view DialogTemplate(std::uint32_t id) override {
        Check(id == 0x5cu || id == 0x5eu,
              "dialog template ID");
        calls.emplace_back("template");
        return id == 0x5cu ? "choose equipment" : "change page";
    }
    void ShowDialog(std::string_view message,
                    const std::array<float, 2>& size,
                    std::uint32_t style,
                    std::uint32_t& callback) override {
        Check(style == 3u &&
                  std::bit_cast<std::uint32_t>(size[0]) ==
                      0x3e99999au &&
                  std::bit_cast<std::uint32_t>(size[1]) ==
                      0x3e99999au,
              "dialog size and style");
        dialog = message;
        calls.emplace_back("show-dialog");
        if (consume_callback) callback = 0u;
        if (throw_dialog) throw std::runtime_error("dialog failure");
    }
    void DestroyCallback(std::uint32_t callback,
                         std::uint32_t deleting) override {
        Check(callback == 0x4000u && deleting == 1u,
              "callback destructor");
        destroyed = callback;
        calls.emplace_back("destroy-callback");
    }
};

void CheckPrompt() {
    Runtime ignored;
    Check(pal4::SmithWindow::OnEquipmentSelectionPrompt(
              Runtime::object, 0u, ignored) == 1u &&
              ignored.calls.empty(),
          "other events are ignored");

    Runtime runtime;
    Check(pal4::SmithWindow::OnEquipmentSelectionPrompt(
              Runtime::object, 1u, runtime) == 1u &&
              runtime.assigned_destination == Runtime::object + 0xd8u &&
              runtime.assigned_source == 0x5000u + 0x98u &&
              runtime.dialog == "choose equipment" &&
              runtime.destroyed == 0x4000u,
          "selected equipment text and confirmation dialog");
    Check(runtime.calls == (std::vector<std::string>{
              "prescription-id", "equipment-id", "manager",
              "create-dialog", "create-callback", "equipment-id",
              "find-item", "assign-text", "template", "show-dialog",
              "destroy-callback"}),
          "exact prompt operation order");
}

void CheckCallbackOwnership() {
    Runtime transferred;
    transferred.consume_callback = true;
    (void)pal4::SmithWindow::OnEquipmentSelectionPrompt(
        Runtime::object, 1u, transferred);
    Check(transferred.destroyed == 0u,
          "dialog may take callback ownership");

    Runtime throwing;
    throwing.throw_dialog = true;
    bool caught = false;
    try {
        (void)pal4::SmithWindow::OnEquipmentSelectionPrompt(
            Runtime::object, 1u, throwing);
    } catch (const std::runtime_error&) {
        caught = true;
    }
    Check(caught && throwing.destroyed == 0x4000u,
          "callback released if dialog throws");
}

void CheckPageChangePrompt() {
    Runtime ignored;
    Check(pal4::SmithWindow::OnPageChangePrompt(
              Runtime::object, 0u, ignored) == 1u &&
              ignored.calls.empty(),
          "page prompt ignores other events");

    Runtime runtime;
    Check(pal4::SmithWindow::OnPageChangePrompt(
              Runtime::object, 1u, runtime) == 1u &&
              runtime.dialog == "change page" &&
              runtime.assigned_source == 0u &&
              runtime.destroyed == 0x4000u &&
              runtime.calls == (std::vector<std::string>{
                  "manager", "create-dialog",
                  "create-page-callback", "equipment-id",
                  "find-item", "template", "show-dialog",
                  "destroy-callback"}),
          "page prompt preserves lookup and dialog order");

    Runtime transferred;
    transferred.consume_callback = true;
    (void)pal4::SmithWindow::OnPageChangePrompt(
        Runtime::object, 1u, transferred);
    Check(transferred.destroyed == 0u,
          "page dialog may take callback ownership");

    Runtime throwing;
    throwing.throw_dialog = true;
    bool caught = false;
    try {
        (void)pal4::SmithWindow::OnPageChangePrompt(
            Runtime::object, 1u, throwing);
    } catch (const std::runtime_error&) {
        caught = true;
    }
    Check(caught && throwing.destroyed == 0x4000u,
          "page callback released if dialog throws");
}

}  // namespace

int main() {
    try {
        CheckPrompt();
        CheckCallbackOwnership();
        CheckPageChangePrompt();
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
