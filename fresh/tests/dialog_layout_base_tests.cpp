#include "pal4/dialog_layout_base.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IDialogLayoutRuntime {
    static constexpr std::uint32_t kObject = 0x1000u;
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::unordered_map<std::uint32_t, std::uint8_t> bytes;
    std::unordered_map<std::uint32_t, std::string> buffers;
    std::vector<std::string> events;
    std::string loaded_path;
    std::string loaded_prefix;
    std::string fail_at;
    std::uint32_t allocation_calls = 0u;

    void FailAt(const std::string& step) {
        if (fail_at == step) throw std::runtime_error(step);
    }
    std::uint8_t IndeterminateBufferByte() override {
        events.emplace_back("stack-byte");
        return 0xa5u;
    }
    void WriteByte(std::uint32_t address,
                   std::uint8_t value) override {
        bytes[address] = value;
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
    }
    void SetErrorFlag(std::uint32_t record,
                      std::uint32_t value) override {
        Check(value == 0u, "initial error flag");
        buffers[record].clear();
        events.emplace_back(record == kObject + 4u ?
                                "init-name" : "init-prefix");
        FailAt(events.back());
    }
    bool AllocateBuffer(std::uint32_t record,
                        std::uint32_t length,
                        bool copy_mode) override {
        Check(copy_mode && (record == kObject + 4u ||
                            record == kObject + 20u),
              "buffer allocation arguments");
        ++allocation_calls;
        events.emplace_back("allocate-" +
                            std::to_string(record - kObject) +
                            "-" + std::to_string(length));
        FailAt("allocate");
        return true;
    }
    void CopyBytes(std::uint32_t record,
                   const std::string& value) override {
        buffers[record] = value;
        events.emplace_back("copy");
    }
    void SetBufferSize(std::uint32_t record,
                       std::uint32_t length) override {
        Check(buffers.at(record).size() == length,
              "buffer size matches text");
        events.emplace_back("size");
    }
    void AppendBuffer(std::uint32_t record,
                      const std::string& suffix) override {
        Check(record == kObject + 20u && suffix == "/",
              "default prefix suffix");
        buffers[record] += suffix;
        events.emplace_back("append");
    }
    std::string ReadBuffer(std::uint32_t record) override {
        Check(record == kObject + 20u,
              "prefix buffer read");
        events.emplace_back("read-prefix");
        return buffers.at(record);
    }
    void ProcessPkScripts() override {
        events.emplace_back("pk-begin");
        FailAt("pk-begin");
    }
    void CleanupPkScripts() override {
        events.emplace_back("pk-end");
    }
    std::string ConvertToUtf8(
        const std::string& value) override {
        events.emplace_back("convert");
        FailAt("convert");
        return value;
    }
    std::uint32_t LoadWindowLayout(
        const std::string& path,
        const std::string& utf8_prefix,
        const std::string& error_prefix) override {
        Check(error_prefix.empty(), "empty error prefix");
        loaded_path = path;
        loaded_prefix = utf8_prefix;
        events.emplace_back("load");
        FailAt("load");
        return 0x4000u;
    }
    void CleanupBuffer(std::uint32_t record) override {
        events.emplace_back(record == kObject + 20u ?
                                "cleanup-prefix" :
                                "cleanup-name");
    }
};

void CheckDefaultPrefix() {
    Runtime runtime;
    Check(pal4::DialogLayoutBase::LoadLayout(
              Runtime::kObject, "SelectDialog", nullptr,
              nullptr, runtime) == Runtime::kObject &&
              runtime.words.at(Runtime::kObject) ==
                  pal4::DialogLayoutBase::kVtable &&
              runtime.words.at(Runtime::kObject + 36u) ==
                  0x4000u,
          "base layout vtable, root and return");
    Check(runtime.bytes.at(Runtime::kObject + 4u) == 0xa5u &&
              runtime.bytes.at(Runtime::kObject + 20u) == 0xa5u &&
              runtime.buffers.at(Runtime::kObject + 4u) ==
                  "SelectDialog" &&
              runtime.buffers.at(Runtime::kObject + 20u) ==
                  "SelectDialog/",
          "both buffer records and generated prefix");
    Check(runtime.loaded_path ==
              "gamedata\\ui\\layouts\\SelectDialog.xml" &&
              runtime.loaded_prefix == "SelectDialog/" &&
              runtime.allocation_calls == 2u &&
              runtime.events.back() == "pk-end",
          "default path, prefix and PK cleanup");
}

void CheckOverrides() {
    Runtime runtime;
    const std::string prefix = "Menu/";
    const std::string override_name = "Localized";
    Check(pal4::DialogLayoutBase::LoadLayout(
              Runtime::kObject, "SelectDialog", &prefix,
              &override_name, runtime) == Runtime::kObject &&
              runtime.buffers.at(Runtime::kObject + 20u) ==
                  "Menu/" &&
              runtime.loaded_prefix == "Menu/" &&
              runtime.buffers.at(Runtime::kObject + 4u) ==
                  "Localized" &&
              runtime.allocation_calls == 3u,
          "explicit prefix and post-load name override");
    Check(runtime.events.back() == "pk-end" &&
              runtime.events[runtime.events.size() - 4u] ==
                  "allocate-4-9",
          "name override occurs after window load");
}

void CheckCStringInputs() {
    Runtime runtime;
    const std::string layout("Dialog\0ignored", 14u);
    const std::string prefix("Prefix/\0ignored", 15u);
    const std::string name("Short\0ignored", 13u);
    Check(pal4::DialogLayoutBase::LoadLayout(
              Runtime::kObject, layout, &prefix, &name,
              runtime) == Runtime::kObject &&
              runtime.loaded_path ==
                  "gamedata\\ui\\layouts\\Dialog.xml" &&
              runtime.loaded_prefix == "Prefix/" &&
              runtime.buffers.at(Runtime::kObject + 4u) ==
                  "Short",
          "strlen input boundaries stop at first NUL");
}

void CheckFailure(const std::string& step,
                  const std::vector<std::string>& cleanup) {
    Runtime runtime;
    runtime.fail_at = step;
    bool threw = false;
    try {
        (void)pal4::DialogLayoutBase::LoadLayout(
            Runtime::kObject, "SelectDialog", nullptr,
            nullptr, runtime);
    } catch (const std::runtime_error& error) {
        threw = error.what() == step;
    }
    Check(threw, "layout failure propagated");
    std::vector<std::string> actual;
    for (const auto& event : runtime.events) {
        if (event.find("cleanup-") == 0u) actual.push_back(event);
    }
    Check(actual == cleanup, "buffer cleanup stage order");
}

}  // namespace

int main() {
    try {
        CheckDefaultPrefix();
        CheckOverrides();
        CheckCStringInputs();
        CheckFailure("init-name", {});
        CheckFailure("allocate", {"cleanup-name"});
        CheckFailure("init-prefix", {"cleanup-name"});
        CheckFailure("pk-begin", {"cleanup-prefix",
                                       "cleanup-name"});
        CheckFailure("convert", {"cleanup-prefix",
                                      "cleanup-name"});
        CheckFailure("load", {"cleanup-prefix",
                                   "cleanup-name"});
        std::cout << "dialog_layout_base_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
