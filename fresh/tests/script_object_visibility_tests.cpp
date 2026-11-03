#include "pal4/script_object_visibility.h"

#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IScriptObjectVisibilityRuntime {
    std::uint32_t found = 0x2000u;
    std::vector<std::string> calls;
    std::uint32_t ObjectManager() override {
        calls.emplace_back("objects");
        return 0x1000u;
    }
    std::uint32_t FindByName(std::uint32_t manager,
                             std::string_view name) override {
        Check(manager == 0x1000u, "object manager lookup operand");
        calls.emplace_back("find:" + std::string(name));
        return found;
    }
    std::uint32_t ArenaManager() override {
        calls.emplace_back("arena");
        return 0x3000u;
    }
    void RemoveAnimatedObject(std::uint32_t arena,
                              std::uint32_t object) override {
        Check(arena == 0x3000u && object == found,
              "remove operands");
        calls.emplace_back("remove");
    }
    void SetObjectState(std::uint32_t object,
                        std::uint32_t state) override {
        Check(object == found && state == 1u,
              "object virtual slot +0x9c and enabled state");
        calls.emplace_back("state");
    }
    void AddAnimatedObject(std::uint32_t arena,
                           std::uint32_t object) override {
        Check(arena == 0x3000u && object == found,
              "add operands");
        calls.emplace_back("add");
    }
    std::uint32_t MinimapManager() override {
        calls.emplace_back("minimap");
        return 0x4000u;
    }
    void RefreshDialog(std::uint32_t minimap) override {
        Check(minimap == 0x4000u, "minimap refresh operand");
        calls.emplace_back("refresh");
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.emplace_back("read:" + std::to_string(address));
        if (address == 0x30e0u) return 0xabcdu;
        if (address == 0x3008u) return 0x1234u;
        throw std::runtime_error("wrong arena field");
    }
    std::uint32_t LogVisibility(
        std::uint32_t source_line, std::string_view name,
        std::uint32_t arena_word8, std::uint32_t arena_word_e0,
        std::uint8_t visible) override {
        Check(source_line == 0x356u && arena_word8 == 0x1234u &&
              arena_word_e0 == 0xabcdu, "logger source and arena fields");
        calls.emplace_back("log:" + std::string(name) + ":" +
                           std::to_string(visible));
        return 0xfeedu;
    }
};

void MissingObject() {
    Runtime runtime;
    runtime.found = 0u;
    Check(pal4::ScriptObjectVisibility::SetVisible(nullptr, 1u,
                                                     runtime) == 0u &&
          runtime.calls == std::vector<std::string>{"objects", "find:"},
          "missing object returns without arena or logging effects");
}

void BothVisibilityPaths() {
    for (const std::uint8_t visible : {0u, 1u, 2u, 255u}) {
        Runtime runtime;
        Check(pal4::ScriptObjectVisibility::SetVisible("crate", visible,
                                                        runtime) == 0xfeedu,
              "logger return forwarded");
        std::vector<std::string> expected{
            "objects", "find:crate", "arena", "remove"};
        if (visible != 0u) {
            expected.insert(expected.end(), {"state", "arena", "add"});
        }
        expected.insert(expected.end(), {
            "minimap", "refresh", "arena", "read:12512",
            "arena", "read:12296", "log:crate:" +
                std::to_string(visible)});
        Check(runtime.calls == expected,
              "object, minimap and logger effects in IDA order");
    }
}

}  // namespace

int main() {
    try {
        MissingObject();
        BothVisibilityPaths();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
