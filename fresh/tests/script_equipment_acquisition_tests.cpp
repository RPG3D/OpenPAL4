#include "pal4/script_equipment_acquisition.h"

#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}

struct Runtime final : pal4::IScriptEquipmentAcquisitionRuntime {
    std::uint32_t window = 0x1000u;
    std::uint8_t valid = 1u;
    std::string fail_at;
    std::vector<std::string> calls;

    void MaybeFail(std::string_view stage) {
        if (fail_at == stage) throw std::runtime_error(std::string(stage));
    }
    std::uint32_t UiFrameManager() override {
        calls.emplace_back("ui"); return 0x2000u;
    }
    std::uint32_t CreateHintWindow(std::uint32_t frames) override {
        Check(frames == 0x2000u, "hint frame manager");
        calls.emplace_back("window"); return window;
    }
    std::uint32_t GameDatabase() override {
        calls.emplace_back("database"); return 0x3000u;
    }
    std::uint8_t HasEquipment(std::uint32_t table,
                              std::uint32_t id) override {
        Check(table == 0x3058u && id == 0x66u,
              "equipment validation table and ID");
        calls.emplace_back("validate"); return valid;
    }
    std::uint32_t FindEquipment(std::uint32_t table,
                                std::uint32_t id) override {
        Check(table == 0x3058u && id == 0x66u,
              "equipment lookup table and ID");
        calls.emplace_back("find"); return 0x4000u;
    }
    std::uint32_t Inventory() override {
        calls.emplace_back("inventory"); return 0x5000u;
    }
    void AddToInventory(std::uint32_t collection,
                        std::uint32_t id) override {
        Check(collection == 0x507cu && id == 0x66u,
              "special equipment collection +0x7c");
        calls.emplace_back("add");
    }
    void InitializeText(std::uint32_t buffer) override {
        Check(buffer <= 1u, "two text buffers");
        calls.emplace_back("init:" + std::to_string(buffer));
        MaybeFail(buffer == 0u ? "init0" : "init1");
    }
    std::string_view EquipmentName(std::uint32_t record) override {
        Check(record == 0x4000u, "equipment name at record +0x9c");
        calls.emplace_back("name"); return "Blade";
    }
    std::string_view GameString(std::uint32_t table,
                                std::uint32_t id) override {
        Check(table == 0x3034u && id == 0xfbu,
              "acquisition format ID and table");
        calls.emplace_back("string"); return "Got %s x%d";
    }
    void FormatText(std::string_view format, std::string_view name,
                    std::uint32_t count) override {
        Check(format == "Got %s x%d" && name == "Blade" && count == 1u,
              "equipment hint uses fixed quantity one");
        calls.emplace_back("format"); MaybeFail("format");
    }
    void AppendText() override { calls.emplace_back("append"); }
    std::string_view HintText() override {
        calls.emplace_back("text"); return "Got Blade x1";
    }
    void ShowHint(std::uint32_t hint, std::string_view text,
                  float x, float y, float scale,
                  std::uint32_t mode, std::uint32_t enabled) override {
        Check(hint == window && text == "Got Blade x1" &&
              x == 0.5f && y == 0.15f && scale == 1.0f &&
              mode == 0u && enabled == 1u,
              "equipment hint geometry and flags");
        calls.emplace_back("show"); MaybeFail("show");
    }
    std::uint32_t AudioManager() override {
        calls.emplace_back("audio"); return 0x6000u;
    }
    void PlaySound(std::uint32_t audio, std::uint8_t flag,
                   std::string_view path, std::uint32_t mode) override {
        Check(audio == 0x6000u && flag == 0x80u &&
              path == "gamedata\\PALSound\\WA06.wav" && mode == 1u,
              "equipment acquisition sound");
        calls.emplace_back("sound"); MaybeFail("sound");
    }
    std::uint8_t CleanupText(std::uint32_t buffer) noexcept override {
        calls.emplace_back("cleanup:" + std::to_string(buffer));
        return buffer == 0u ? 0x7eu : 0x55u;
    }
};

void Gates() {
    Runtime no_window;
    no_window.window = 0u;
    Check(pal4::ScriptEquipmentAcquisition::Add(0x66u, 0x80u,
                                                  no_window) == 0u &&
          no_window.calls == std::vector<std::string>{"ui", "window"},
          "hint window creation gates all inventory changes");

    Runtime invalid;
    invalid.valid = 0u;
    Check(pal4::ScriptEquipmentAcquisition::Add(0x66u, 0x80u,
                                                  invalid) == 0u &&
          invalid.calls == std::vector<std::string>{
              "ui", "window", "database", "validate"},
          "invalid equipment exits before inventory");

    Runtime quiet;
    Check(pal4::ScriptEquipmentAcquisition::Add(0x66u, 0u,
                                                  quiet) == 0u &&
          quiet.calls == std::vector<std::string>{
              "ui", "window", "database", "validate", "database",
              "find", "inventory", "add"},
          "quiet acquisition updates inventory without text or sound");
}

void HintAndCleanup() {
    Runtime runtime;
    Check(pal4::ScriptEquipmentAcquisition::Add(0x66u, 0x80u,
                                                  runtime) == 0x7eu &&
          runtime.calls == std::vector<std::string>{
              "ui", "window", "database", "validate", "database",
              "find", "inventory", "add", "init:0", "init:1",
              "name", "database", "string", "format", "append",
              "text", "show", "audio", "sound", "cleanup:1",
              "cleanup:0"},
          "full equipment hint and reverse cleanup return first text byte");

    for (const std::string stage : {"init0", "init1", "format",
                                    "show", "sound"}) {
        Runtime failure;
        failure.fail_at = stage;
        try {
            (void)pal4::ScriptEquipmentAcquisition::Add(
                0x66u, 0x80u, failure);
            throw std::runtime_error("missing failure");
        } catch (const std::runtime_error& error) {
            Check(std::string_view(error.what()) == stage,
                  "original callback exception propagates");
        }
        if (stage == "init0")
            Check(failure.calls.back() == "init:0",
                  "failed first buffer has no cleanup");
        else if (stage == "init1")
            Check(failure.calls.back() == "cleanup:0",
                  "failed second buffer cleans first");
        else
            Check(failure.calls[failure.calls.size() - 2u] ==
                      "cleanup:1" &&
                  failure.calls.back() == "cleanup:0",
                  "constructed buffers clean in reverse order");
    }
}

}  // namespace

int main() {
    try {
        Gates();
        HintAndCleanup();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
