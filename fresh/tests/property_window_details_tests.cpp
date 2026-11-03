#include "pal4/property_window_details.h"

#include <cstdlib>
#include <iostream>
#include <map>
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

struct Runtime final : pal4::IPropertyWindowDetailsRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::string> strings;
    std::vector<std::string> calls;
    std::uint32_t hover = 0u;
    std::uint32_t found = 0x8100u;

    std::uint32_t HoverItem(std::uint32_t list) override {
        Check(list == 0x7000u, "hover reads list control at +0x58");
        calls.emplace_back("hover");
        return hover;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.push_back("read:" + std::to_string(address));
        return words[address];
    }
    std::uint32_t FindInventoryElement(std::uint32_t id,
                                       std::uint32_t category) override {
        Check(id == 123u && category == 5u,
              "hovered ID searches category five");
        calls.emplace_back("find");
        return found;
    }
    std::string ReadText(std::uint32_t address) override {
        calls.push_back("text:" + std::to_string(address));
        return strings[address];
    }
    std::string ConvertToUtf8(std::string_view text) override {
        calls.push_back("utf8:" + std::string(text));
        return "utf8:" + std::string(text);
    }
    void SetText(std::uint32_t window, std::string_view text) override {
        calls.push_back("set:" + std::to_string(window) + ":" +
                        std::string(text));
    }
    std::uint32_t UpdateModel(pal4::PropertyWindowState&,
                              std::uint32_t record) override {
        calls.push_back("model:" + std::to_string(record));
        return 0x9000u;
    }
};

void TestHoverLookup() {
    pal4::PropertyWindowState state;
    state.Write(0x58u, 0x7000u);
    Runtime runtime;
    Check(pal4::PropertyWindowDetails::HoveredRecord(state, runtime) == 0u,
          "null hovered item returns null");
    Check(runtime.calls == std::vector<std::string>{"hover"},
          "null hover skips inventory lookup");

    runtime.calls.clear();
    runtime.hover = 0x8000u;
    runtime.words[0x8138u] = 123u;
    Check(pal4::PropertyWindowDetails::HoveredRecord(state, runtime) == 0x8100u,
          "hover returns matched inventory element");
    Check(runtime.calls == std::vector<std::string>{
              "hover", "read:33080", "find"},
          "hover reads item field +0x138 before inventory lookup");

    runtime.found = 0u;
    Check(pal4::PropertyWindowDetails::HoveredRecord(state, runtime) == 0u,
          "missing inventory match propagates null");
}

void TestDetailUpdate() {
    pal4::PropertyWindowState state;
    state.Write(0x60u, 0x7000u);
    state.Write(0x64u, 0x7100u);
    state.Write(0x74u, 99u);
    Runtime runtime;
    Check(pal4::PropertyWindowDetails::Update(state, 0u, runtime) == 0x9000u,
          "empty update returns model result");
    Check(runtime.calls == std::vector<std::string>{
              "set:28672:", "set:28928:", "model:0"},
          "empty detail clears both text fields before model refresh");
    Check(state.Read(0x74u) == 0u, "empty detail clears item key");

    runtime.calls.clear();
    runtime.words[0x8100u] = 0x8200u;
    runtime.words[0x8104u] = 123u;
    runtime.words[0x8224u] = 0x8300u;
    runtime.words[0x8234u] = 0x8400u;
    runtime.strings[0x8300u] = "Herb";
    runtime.strings[0x8400u] = "Restores health";
    Check(pal4::PropertyWindowDetails::Update(
              state, 0x8100u, runtime) == 0x9000u,
          "present update returns model result");
    Check(runtime.calls == std::vector<std::string>{
              "read:33024", "read:33316", "text:33536", "utf8:Herb",
              "set:28672:utf8:Herb", "read:33332", "text:33792",
              "utf8:Restores health", "set:28928:utf8:Restores health",
              "model:33024", "read:33028"},
          "title, description, model and key follow IDA order");
    Check(state.Read(0x74u) == 123u, "detail stores inventory item key");

    runtime.calls.clear();
    runtime.words[0x8224u] = 0u;
    runtime.words[0x8234u] = 0u;
    runtime.strings[pal4::PropertyWindowDetails::kEmptyTextAddress] = "";
    (void)pal4::PropertyWindowDetails::Update(state, 0x8100u, runtime);
    Check(runtime.calls[2] == "text:8657252" &&
          runtime.calls[6] == "text:8657252",
          "null title and description use empty literal address");
}

}  // namespace

int main() {
    TestHoverLookup();
    TestDetailUpdate();
}
