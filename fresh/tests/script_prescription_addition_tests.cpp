#include "pal4/script_prescription_addition.h"

#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}

struct Runtime final : pal4::IScriptPrescriptionAdditionRuntime {
    std::vector<std::string> calls;
    std::string fail_at;
    std::uint32_t hint = 0x5000u;
    bool has_record = true;
    std::uint32_t name = 0x9000u;
    std::uint32_t format = 0xa000u;
    std::uint8_t expected_hint_flag = 1u;
    std::uint32_t next_text = 0x7000u;

    void Record(std::string call) {
        calls.push_back(call);
        if (call == fail_at) throw std::runtime_error("injected failure");
    }
    std::uint32_t FrameManager() override {
        Record("frames");
        return 0x4000u;
    }
    std::uint32_t CreateHintWindow(std::uint32_t manager) override {
        Check(manager == 0x4000u, "prescription hint manager");
        Record("hint");
        return hint;
    }
    std::uint32_t GameDatabase() override {
        Record("database");
        return 0x1000u;
    }
    bool HasRecord(std::uint32_t table, std::int32_t id) override {
        Check(table == 0x1080u && id == -7,
              "prescription database gate");
        Record("has");
        return has_record;
    }
    std::uint32_t FindRecord(std::uint32_t table,
                             std::int32_t id) override {
        Check(table == 0x1080u && id == -7,
              "prescription database lookup");
        Record("record");
        return 0x3000u;
    }
    std::uint32_t Inventory() override {
        Record("inventory");
        return 0x2000u;
    }
    void AddPrescription(std::uint32_t collection,
                          std::int32_t id) override {
        Check(collection == 0x2104u && id == -7,
              "prescription inventory collection");
        Record("add");
    }
    std::uint32_t InitializeText() override {
        const auto text = next_text;
        next_text += 0x100u;
        Record("text-init");
        return text;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        if (address == 0x3008u) {
            Record("name");
            return name;
        }
        Check(address == 0x6004u, "localized format pointer offset");
        Record("format-pointer");
        return format;
    }
    std::uint32_t EmptyText() override {
        Record("empty");
        return 0xe000u;
    }
    std::uint32_t FindGameString(std::uint32_t table,
                                 std::uint32_t id) override {
        Check(table == 0x1034u && id == 0x7au,
              "prescription localized string ID");
        Record("find-string");
        return 0x6000u;
    }
    void FormatText(std::uint32_t destination,
                    std::uint32_t text_format,
                    std::uint32_t text_name) override {
        Check(destination == 0x7000u &&
                  text_format == (format ? format : 0xe000u) &&
                  text_name == (name ? name : 0xe000u),
              "prescription format arguments");
        Record("format");
    }
    void AppendText(std::uint32_t destination,
                    std::uint32_t source) override {
        Check(destination == 0x7100u && source == 0x7000u,
              "formatted prescription text appends to display text");
        Record("append");
    }
    void ShowHint(std::uint32_t window, std::uint32_t text,
                  float x, float y, float scale,
                  std::uint32_t mode, std::uint32_t visible) override {
        Check(window == 0x5000u && text == 0x7100u &&
                  x == 0.5f && y == 0.15f && scale == 1.0f &&
                  mode == 0u && visible == 1u,
              "prescription hint geometry and flags");
        Record("show");
    }
    std::uint32_t AudioManager() override {
        Record("audio");
        return 0x8000u;
    }
    void PlayMusic(std::uint32_t audio, std::uint8_t flag,
                   std::string_view path, std::uint32_t mode) override {
        Check(audio == 0x8000u && flag == expected_hint_flag &&
                  path == "gamedata\\PALSound\\WA06.wav" && mode == 1u,
              "prescription audio path and mode");
        Record("music");
    }
    std::uint8_t CleanupText(std::uint32_t text) override {
        Check(text == 0x7000u || text == 0x7100u,
              "prescription temporary string handle");
        Record(text == 0x7100u ? "cleanup-display" : "cleanup-format");
        return text == 0x7000u ? 0x34u : 0x56u;
    }
};

void Gates() {
    Runtime no_hint;
    no_hint.hint = 0u;
    Check(pal4::ScriptPrescriptionAddition::Add(-7, 1u, no_hint) == 0u &&
              no_hint.calls == std::vector<std::string>{"frames", "hint"},
          "missing hint window stops before database");
    Runtime no_record;
    no_record.has_record = false;
    Check(pal4::ScriptPrescriptionAddition::Add(-7, 1u, no_record) == 0u &&
              no_record.calls == std::vector<std::string>{
                  "frames", "hint", "database", "has"},
          "missing prescription skips inventory");
    Runtime quiet;
    Check(pal4::ScriptPrescriptionAddition::Add(-7, 0u, quiet) == 0u &&
              quiet.calls == std::vector<std::string>{
                  "frames", "hint", "database", "has", "database",
                  "record", "inventory", "add"},
          "quiet prescription add skips text and sound");
}

void HintAndCleanup() {
    Runtime runtime;
    runtime.name = 0u;
    runtime.format = 0u;
    runtime.expected_hint_flag = 0xffu;
    Check(pal4::ScriptPrescriptionAddition::Add(-7, 0xffu, runtime) ==
                  0x34u,
          "final formatted-text destructor byte forwarded");
    Check(runtime.calls == std::vector<std::string>{
              "frames", "hint", "database", "has", "database", "record",
              "inventory", "add", "text-init", "text-init", "name",
              "empty", "database", "find-string", "format-pointer",
              "empty", "format", "append", "show", "audio", "music",
              "cleanup-display", "cleanup-format"},
          "localized hint and audio precede reverse text cleanup");
    for (const auto* fail : {"format", "append", "show", "music"}) {
        Runtime failing;
        failing.fail_at = fail;
        bool caught = false;
        try {
            (void)pal4::ScriptPrescriptionAddition::Add(-7, 1u, failing);
        } catch (const std::runtime_error&) {
            caught = true;
        }
        Check(caught && failing.calls[failing.calls.size() - 2u] ==
                            "cleanup-display" &&
                  failing.calls.back() == "cleanup-format",
              "failure releases both temporary strings");
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
