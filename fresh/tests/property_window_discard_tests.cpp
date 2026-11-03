#include "pal4/property_window_discard.h"

#include <cstdlib>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(bool ok, const char* message) {
    if (!ok) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IPropertyWindowDiscardRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::string> strings;
    std::vector<std::string> calls;
    std::uint32_t format_address = 0x8500u;
    std::uint32_t allocation = 0xa000u;
    bool throw_format = false;
    bool throw_show = false;
    std::uint32_t hovered_record = 0u;

    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.push_back("read:" + std::to_string(address));
        return words[address];
    }
    std::string ReadText(std::uint32_t address) override {
        calls.push_back("text:" + std::to_string(address));
        return strings[address];
    }
    std::uint32_t CreateCommonDialog() override {
        calls.emplace_back("dialog-create");
        return 0xd000u;
    }
    void InitializeBuffer() override { calls.emplace_back("buffer-init"); }
    std::uint32_t TriggerTextAddress(std::uint32_t trigger) override {
        Check(trigger == 0x68u, "description reads trigger 104");
        calls.emplace_back("trigger");
        return format_address;
    }
    std::string FormatBuffer(std::string_view format,
                             std::string_view argument) override {
        calls.emplace_back("format");
        if (throw_format) throw std::runtime_error("format");
        Check(format == "Discard %s?" && argument == "herb",
              "format receives full item description pointer content");
        return "Discard herb?";
    }
    std::string ConvertToUtf8(std::string_view text) override {
        Check(text == "Discard herb?", "UTF-8 conversion uses formatted text");
        calls.emplace_back("utf8");
        return "utf8:Discard herb?";
    }
    std::uint32_t Allocate(std::uint32_t bytes) override {
        Check(bytes == 0x18u, "description callback allocates 24 bytes");
        calls.emplace_back("allocate");
        return allocation;
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        calls.push_back("write:" + std::to_string(address));
        words[address] = value;
    }
    std::uint32_t ObjectAddress(
        const pal4::PropertyWindowState&) override {
        calls.emplace_back("owner");
        return 0x9000u;
    }
    void ShowWithFlags(std::uint32_t dialog, std::string_view text,
                       float x, float y, std::uint32_t flags,
                       std::uint32_t callback) override {
        Check(dialog == 0xd000u && text == "utf8:Discard herb?" &&
                  x == 0.3f && y == 0.3f && flags == 3u,
              "dialog receives converted text, two 0.3 coordinates and flags 3");
        calls.push_back("show:" + std::to_string(callback));
        if (throw_show) throw std::runtime_error("show");
    }
    void ReleaseCallback(std::uint32_t callback,
                         std::uint32_t flag) override {
        Check(callback == 0xa000u && flag == 1u,
              "callback ownership releases with flag one");
        calls.emplace_back("release");
    }
    void CleanupBuffer() override { calls.emplace_back("buffer-cleanup"); }
    std::uint32_t HoveredRecord(pal4::PropertyWindowState&) override {
        calls.emplace_back("hovered");
        return hovered_record;
    }
    void PlayMusic(std::string_view file, std::uint32_t mode) override {
        Check(file == "wa07.wav" && mode == 1u,
              "confirmed removal plays exact sound and mode");
        calls.emplace_back("music");
    }
    void RemoveItem(std::uint32_t id, std::uint32_t count) override {
        Check(id == 123u && count == 4u,
              "confirmed removal uses definition ID and record +8 count");
        calls.emplace_back("remove");
    }
    void PopulateList(pal4::PropertyWindowState&,
                      std::uint32_t category,
                      bool preserve_scroll) override {
        Check(category == 5u && !preserve_scroll,
              "confirmed removal refreshes selected category");
        calls.emplace_back("populate");
    }
};

void TestDialogAndFallbacks() {
    pal4::PropertyWindowState state;
    Runtime runtime;
    runtime.words[0x7004u] = 0x8000u;
    runtime.words[0x8000u] = 0x8100u;
    runtime.words[0x8114u] = 0x8400u;
    runtime.strings[0x8400u] = "herb";
    runtime.strings[0x8500u] = "Discard %s?";
    runtime.strings[pal4::PropertyWindowDiscard::kEmptyTextAddress] = "";

    Check(pal4::PropertyWindowDiscard::Show(
              state, 0x7000u, runtime), "description action returns true");
    Check(runtime.words[0xa000u] ==
              pal4::PropertyWindowDiscard::kCallbackVtable &&
          runtime.words[0xa008u] ==
              pal4::PropertyWindowDiscard::kConfirmationCallback &&
          runtime.words[0xa00cu] == 0u &&
          runtime.words[0xa010u] == 0x9000u,
          "description callback stores vtable, handler, zero and owner");
    Check(runtime.calls == std::vector<std::string>{
              "read:28676", "dialog-create", "buffer-init", "read:32768",
              "read:33044", "text:33792", "trigger", "text:34048",
              "format", "allocate", "write:40968", "write:40972",
              "owner", "write:40976", "write:40960", "utf8",
              "show:40960", "release", "buffer-cleanup"},
          "description operation order matches IDA constructor and cleanup path");

    runtime.calls.clear();
    runtime.words[0x8114u] = 0u;
    runtime.format_address = 0u;
    runtime.throw_format = true;
    try {
        (void)pal4::PropertyWindowDiscard::Show(
            state, 0x7000u, runtime);
        Check(false, "format exception must propagate");
    } catch (const std::runtime_error&) {
        Check(runtime.calls.back() == "buffer-cleanup",
              "format failure cleans buffer");
        Check(runtime.calls[5] == "text:8657252" &&
              runtime.calls[7] == "text:8657252",
              "both absent pointers use empty literal address");
    }
}

void TestAllocationAndExceptions() {
    pal4::PropertyWindowState state;
    Runtime runtime;
    runtime.words[0x7004u] = 0x8000u;
    runtime.words[0x8000u] = 0x8100u;
    runtime.words[0x8114u] = 0x8400u;
    runtime.strings[0x8400u] = "herb";
    runtime.strings[0x8500u] = "Discard %s?";
    runtime.allocation = 0u;
    Check(pal4::PropertyWindowDiscard::Show(
              state, 0x7000u, runtime), "null callback still shows dialog");
    Check(runtime.calls[runtime.calls.size() - 2u] == "show:0" &&
          runtime.calls.back() == "buffer-cleanup",
          "null callback skips writes and release");

    runtime.calls.clear();
    runtime.allocation = 0xa000u;
    runtime.throw_show = true;
    try {
        (void)pal4::PropertyWindowDiscard::Show(
            state, 0x7000u, runtime);
        Check(false, "show exception must propagate");
    } catch (const std::runtime_error&) {
        Check(runtime.calls[runtime.calls.size() - 2u] == "release" &&
              runtime.calls.back() == "buffer-cleanup",
              "show exception releases callback then buffer");
    }
}

void TestConfirm() {
    pal4::PropertyWindowState state;
    state.Write(0x6cu, 5u);
    Runtime runtime;
    Check(pal4::PropertyWindowDiscard::Confirm(
              state, 1u, runtime), "missing hovered item returns true");
    Check(runtime.calls == std::vector<std::string>{"hovered"},
          "missing item skips sound and removal");

    runtime.calls.clear();
    runtime.hovered_record = 0x8100u;
    Check(pal4::PropertyWindowDiscard::Confirm(
              state, 0u, runtime), "cancelled choice returns true");
    Check(runtime.calls == std::vector<std::string>{"hovered"},
          "cancelled choice still resolves hover but skips mutation");

    runtime.calls.clear();
    runtime.words[0x8100u] = 0x8200u;
    runtime.words[0x8108u] = 4u;
    runtime.words[0x8200u] = 123u;
    Check(pal4::PropertyWindowDiscard::Confirm(
              state, 1u, runtime), "confirmed choice returns true");
    Check(runtime.calls == std::vector<std::string>{
              "hovered", "music", "read:33024", "read:33032",
              "read:33280", "remove", "populate"},
          "confirmed choice uses hovered record, count and ID in source order");
}

}  // namespace

int main() {
    TestDialogAndFallbacks();
    TestAllocationAndExceptions();
    TestConfirm();
}
