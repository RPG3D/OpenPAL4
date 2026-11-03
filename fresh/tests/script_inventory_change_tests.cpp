#include "pal4/script_inventory_change.h"

#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}

struct Runtime final : pal4::IScriptInventoryChangeRuntime {
    std::uint32_t window = 0x2200u;
    std::uint8_t present = 1u;
    std::uint8_t cleanup_result = 0x7eu;
    bool fail_format = false;
    std::vector<std::string> calls;
    std::string text;

    std::uint32_t UiFrameManager() override {
        calls.emplace_back("ui"); return 0x1100u;
    }
    std::uint32_t CreateHintWindow(std::uint32_t frames) override {
        Check(frames == 0x1100u, "hint frame manager");
        calls.emplace_back("window"); return window;
    }
    std::uint32_t GameDatabase() override {
        calls.emplace_back("db"); return 0x3300u;
    }
    std::uint8_t HasProperty(std::uint32_t db, std::uint32_t id) override {
        Check(db == 0x3300u && id == 99u, "property validity operands");
        calls.emplace_back("has"); return present;
    }
    std::uint32_t FindProperty(std::uint32_t db,
                               std::uint32_t id) override {
        Check(db == 0x3300u && id == 99u, "property lookup operands");
        calls.emplace_back("property"); return 0x4400u;
    }
    std::uint32_t Inventory() override {
        calls.emplace_back("inventory"); return 0x5500u;
    }
    void AddItem(std::uint32_t inventory, std::uint32_t id,
                 std::int32_t count) override {
        Check(inventory == 0x5500u && id == 99u && count == -3,
              "add inventory operands");
        calls.emplace_back("add");
    }
    void RemoveItem(std::uint32_t inventory, std::uint32_t id,
                    std::int32_t count) override {
        Check(inventory == 0x5500u && id == 99u && count == -3,
              "remove inventory operands");
        calls.emplace_back("remove");
    }
    void InitializeTextBuffer() override {
        calls.emplace_back("text-init");
    }
    std::string_view PropertyName(std::uint32_t property) override {
        Check(property == 0x4400u, "property name offset +20");
        calls.emplace_back("name"); return "Herb";
    }
    std::string_view GameString(std::uint32_t table,
                                std::uint32_t id) override {
        Check(table == 0x3334u && (id == 0xfbu || id == 0xfcu),
              "database game-string table +0x34");
        calls.emplace_back("string:" + std::to_string(id));
        return id == 0xfbu ? "Got %s x%d" : "Lost %s x%d";
    }
    void FormatText(std::string_view format, std::string_view name,
                    std::int32_t count) override {
        Check(name == "Herb" && count == -3, "hint format arguments");
        calls.emplace_back("format");
        if (fail_format) throw std::runtime_error("format failed");
        text = std::string(format) + ":Herb:-3";
    }
    std::string_view TextBuffer() override {
        calls.emplace_back("text"); return text;
    }
    void ShowHint(std::uint32_t hint, std::string_view message,
                  float x, float y, float scale,
                  std::uint32_t mode, std::uint32_t enabled) override {
        Check(hint == window && !message.empty() && x == 0.5f &&
              y == 0.15f && scale == 1.0f && mode == 0u && enabled == 1u,
              "hint presentation operands");
        calls.emplace_back("show");
    }
    std::uint32_t AudioManager() override {
        calls.emplace_back("audio"); return 0x6600u;
    }
    void PlaySound(std::uint32_t audio, std::uint8_t flag,
                   std::string_view path, std::uint32_t mode) override {
        Check(audio == 0x6600u && flag == 0x80u && mode == 1u &&
              (path == "gamedata\\PALSound\\WA06.wav" ||
               path == "gamedata\\PALSound\\WA07.wav"),
              "sound file and flag");
        calls.emplace_back("sound:" + std::string(path));
    }
    std::uint8_t CleanupTextBuffer() noexcept override {
        calls.emplace_back("cleanup"); return cleanup_result;
    }
};

using Entry = std::uint8_t (*)(std::uint32_t, std::int32_t,
                               std::uint8_t,
                               pal4::IScriptInventoryChangeRuntime&);

void Gates(Entry entry, std::string_view operation) {
    Runtime absent_window;
    absent_window.window = 0u;
    Check(entry(99u, -3, 0x80u, absent_window) == 0u &&
          absent_window.calls == std::vector<std::string>{"ui", "window"},
          "window failure short-circuits inventory");

    Runtime absent_property;
    absent_property.present = 0u;
    Check(entry(99u, -3, 0x80u, absent_property) == 0u &&
          absent_property.calls ==
              std::vector<std::string>{"ui", "window", "db", "has"},
          "invalid property short-circuits inventory");

    Runtime quiet;
    Check(entry(99u, -3, 0u, quiet) == 0u &&
          quiet.calls == std::vector<std::string>{
              "ui", "window", "db", "has", "db", "property",
              "inventory", std::string(operation)},
          "quiet change mutates inventory without hint or sound");
}

void Hints(Entry entry, std::string_view operation,
           std::uint32_t string_id, std::string_view sound_path) {
    Runtime runtime;
    Check(entry(99u, -3, 0x80u, runtime) == runtime.cleanup_result,
          "text cleanup byte is returned");
    const std::vector<std::string> expected{
        "ui", "window", "db", "has", "db", "property", "inventory",
        std::string(operation), "text-init", "name", "db",
        "string:" + std::to_string(string_id), "format", "text", "show",
        "audio", "sound:" + std::string(sound_path), "cleanup"};
    Check(runtime.calls == expected, "full hint and audio order");

    Runtime failure;
    failure.fail_format = true;
    try {
        (void)entry(99u, -3, 0x80u, failure);
        throw std::runtime_error("missing format failure");
    } catch (const std::runtime_error& error) {
        Check(std::string_view(error.what()) == "format failed",
              "format failure propagates");
    }
    Check(failure.calls.back() == "cleanup",
          "temporary text buffer cleaned on exception");
}

}  // namespace

int main() {
    try {
        Gates(&pal4::ScriptInventoryChange::Add, "add");
        Gates(&pal4::ScriptInventoryChange::Remove, "remove");
        Hints(&pal4::ScriptInventoryChange::Add, "add", 0xfbu,
              "gamedata\\PALSound\\WA06.wav");
        Hints(&pal4::ScriptInventoryChange::Remove, "remove", 0xfcu,
              "gamedata\\PALSound\\WA07.wav");
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
