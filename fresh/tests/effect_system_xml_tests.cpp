#include "pal4/effect_system.h"

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(bool condition, std::string_view message) {
    if (condition) return;
    std::cerr << message << '\n';
    std::exit(1);
}

struct Runtime final : pal4::IEffectSystemXmlFileRuntime {
    static constexpr std::uint32_t kSystem = 0x1000u;
    static constexpr std::uint32_t kXml = 0x2000u;
    static constexpr std::uint32_t kContext = 0x3000u;
    bool enabled = true;
    bool file_found = true;
    bool parsed = true;
    bool throw_on_parse = false;
    std::uint32_t primary = 0x4000u;
    std::uint32_t secondary = 0x5000u;
    std::uint32_t next_id = 9u;
    std::vector<std::string> calls;

    std::uint8_t ReadByte(std::uint32_t address) override {
        Check(address == kSystem + 4u, "XML enabled address");
        calls.emplace_back("enabled");
        return enabled ? 1u : 0u;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        Check(address == kSystem + 8u, "XML id address");
        calls.emplace_back("read id");
        return next_id;
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        Check(address == kSystem + 8u && value == next_id + 1u,
              "XML id increment");
        next_id = value;
        calls.emplace_back("write id");
    }
    std::uint32_t CreatePrimaryEffect(std::uint32_t xml,
                                      std::uint32_t context,
                                      std::uint32_t system) override {
        Check(xml == kXml && context == kContext && system == kSystem,
              "primary factory arguments");
        calls.emplace_back("primary");
        return primary;
    }
    std::uint32_t CreateSecondaryEffect(std::uint32_t xml,
                                        std::uint32_t context,
                                        std::uint32_t system) override {
        Check(xml == kXml && context == kContext && system == kSystem,
              "secondary factory arguments");
        calls.emplace_back("secondary");
        return secondary;
    }
    void InsertXmlEffect(std::uint32_t tree, std::uint32_t id,
                         std::uint32_t effect) override {
        Check(tree == kSystem + 228u && id == next_id - 1u &&
              effect == (primary ? primary : secondary),
              "XML effect registration");
        calls.emplace_back("insert");
    }
    void BeginScriptScope() override { calls.emplace_back("begin"); }
    void EndScriptScope() override { calls.emplace_back("end"); }
    std::uint32_t OpenFile(std::string_view path) override {
        Check(path == "gamedata\\effect\\script\\spell.hef",
              "effect script path");
        calls.emplace_back("open");
        return file_found ? 0x6000u : 0u;
    }
    void CloseFile(std::uint32_t file) override {
        Check(file == 0x6000u, "close effect file");
        calls.emplace_back("close");
    }
    std::uint32_t CreateFileAdapter(std::uint32_t file) override {
        Check(file == 0x6000u, "effect adapter source");
        calls.emplace_back("adapter");
        return 0x7000u;
    }
    void DestroyFileAdapter(std::uint32_t adapter) override {
        Check(adapter == 0x7000u, "destroy effect adapter");
        calls.emplace_back("destroy adapter");
    }
    std::uint32_t CreateXmlDocument() override {
        calls.emplace_back("document");
        return kXml;
    }
    void DestroyXmlDocument(std::uint32_t xml) override {
        Check(xml == kXml, "destroy effect XML");
        calls.emplace_back("destroy document");
    }
    bool ParseXml(std::uint32_t adapter,
                  std::uint32_t xml) override {
        Check(adapter == 0x7000u && xml == kXml, "effect parse inputs");
        calls.emplace_back("parse");
        if (throw_on_parse) throw std::runtime_error("parse");
        return parsed;
    }
};

constexpr pal4::EffectSystemHandle kInvalid{0xffffffffu, 0u};

void TestXmlFactorySelection() {
    Runtime inactive;
    inactive.enabled = false;
    Check(pal4::EffectSystem::LoadEffectFromXml(
              Runtime::kSystem, Runtime::kXml, Runtime::kContext,
              inactive) == kInvalid &&
          inactive.calls == std::vector<std::string>{"enabled"},
          "disabled XML load calls no factory");

    Runtime primary;
    Check(pal4::EffectSystem::LoadEffectFromXml(
              Runtime::kSystem, Runtime::kXml, Runtime::kContext,
              primary) == pal4::EffectSystemHandle{9u, Runtime::kSystem} &&
          primary.calls == (std::vector<std::string>{
              "enabled", "primary", "read id", "write id", "insert"}),
          "primary factory short circuits secondary");

    Runtime secondary;
    secondary.primary = 0u;
    Check(pal4::EffectSystem::LoadEffectFromXml(
              Runtime::kSystem, Runtime::kXml, Runtime::kContext,
              secondary) == pal4::EffectSystemHandle{9u, Runtime::kSystem} &&
          secondary.calls == (std::vector<std::string>{
              "enabled", "primary", "secondary", "read id",
              "write id", "insert"}),
          "secondary factory receives failed primary XML");

    Runtime missing;
    missing.primary = 0u;
    missing.secondary = 0u;
    Check(pal4::EffectSystem::LoadEffectFromXml(
              Runtime::kSystem, Runtime::kXml, Runtime::kContext,
              missing) == kInvalid && missing.next_id == 9u &&
          missing.calls == (std::vector<std::string>{
              "enabled", "primary", "secondary"}),
          "two factory failures leave ID unchanged");
}

void TestXmlFileLifecycle() {
    Runtime inactive;
    inactive.enabled = false;
    Check(pal4::EffectSystem::LoadEffectFromXmlFile(
              Runtime::kSystem, "spell", Runtime::kContext,
              inactive) == kInvalid &&
          inactive.calls == std::vector<std::string>{"enabled"},
          "disabled file load does not open script scope");

    Runtime missing;
    missing.file_found = false;
    Check(pal4::EffectSystem::LoadEffectFromXmlFile(
              Runtime::kSystem, "spell", Runtime::kContext,
              missing) == kInvalid &&
          missing.calls == (std::vector<std::string>{
              "enabled", "begin", "open", "end"}),
          "missing file releases only script scope");

    Runtime invalid_xml;
    invalid_xml.parsed = false;
    Check(pal4::EffectSystem::LoadEffectFromXmlFile(
              Runtime::kSystem, "spell", Runtime::kContext,
              invalid_xml) == kInvalid &&
          invalid_xml.calls == (std::vector<std::string>{
              "enabled", "begin", "open", "adapter", "document",
              "parse", "close", "destroy document",
              "destroy adapter", "end"}),
          "invalid XML closes resources in native order");

    Runtime valid;
    Check(pal4::EffectSystem::LoadEffectFromXmlFile(
              Runtime::kSystem, "spell", Runtime::kContext,
              valid) == pal4::EffectSystemHandle{9u, Runtime::kSystem} &&
          valid.calls == (std::vector<std::string>{
              "enabled", "begin", "open", "adapter", "document",
              "parse", "enabled", "primary", "read id", "write id",
              "insert", "close", "destroy document",
              "destroy adapter", "end"}),
          "valid script delegates to XML entry and closes in native order");

    Runtime thrown;
    thrown.throw_on_parse = true;
    bool caught = false;
    try {
        (void)pal4::EffectSystem::LoadEffectFromXmlFile(
            Runtime::kSystem, "spell", Runtime::kContext, thrown);
    } catch (const std::runtime_error&) {
        caught = true;
    }
    Check(caught && thrown.calls == (std::vector<std::string>{
              "enabled", "begin", "open", "adapter", "document",
              "parse", "close", "destroy document",
              "destroy adapter", "end"}),
          "parse exception releases file, XML and script resources");
}

}  // namespace

int main() {
    TestXmlFactorySelection();
    TestXmlFileLifecycle();
}
