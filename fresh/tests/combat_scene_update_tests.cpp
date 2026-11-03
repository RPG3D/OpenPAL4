#include "pal4/combat_scene_update.h"

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::ICombatSceneUpdateRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> calls;
    std::uint32_t direct_attribute = 0u;
    std::uint32_t tree_attribute = 0u;
    bool clear_result_on_document = false;
    bool throw_on_add = false;
    bool throw_on_transition = false;
    double ui_delta = 0.0;

    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.push_back("read " + std::to_string(address));
        return words[address];
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        calls.push_back("byte " + std::to_string(address));
        return bytes[address];
    }
    std::uint32_t CombatSystem() override {
        calls.emplace_back("combat-system");
        return 0x3000u;
    }
    void AdvanceCombat(std::uint32_t system, float delta) override {
        Check(system == 0x3000u && delta == 0.25f,
              "combat update receives original float delta");
        calls.emplace_back("advance-combat");
    }
    std::uint32_t UIFrameManager() override {
        calls.emplace_back("ui-manager");
        return 0x4000u;
    }
    void UpdateUI(std::uint32_t manager, double delta) override {
        Check(manager == 0x4000u, "UI update receives manager");
        ui_delta = delta;
        calls.emplace_back("update-ui");
    }
    std::uint32_t ConstructXmlDocument() override {
        calls.emplace_back("doc-construct");
        if (clear_result_on_document) words[0x3000u + 504u] = 0u;
        return 0x7000u;
    }
    std::uint32_t DestroyXmlDocument(std::uint32_t document) override {
        Check(document == 0x7000u, "destroy correct document");
        calls.emplace_back("doc-destroy");
        return 99u;
    }
    std::uint32_t GetConfigStringValue(std::uint32_t document,
                                        std::uint32_t key) override {
        calls.push_back("direct-find " + std::to_string(document) + " " +
                        std::to_string(key));
        return direct_attribute;
    }
    std::uint32_t FindTreeAttribute(std::uint32_t document,
                                    std::uint32_t key) override {
        Check(document == 0x7000u, "tree lookup belongs to outcome document");
        calls.push_back("tree-find " + std::to_string(document) + " " +
                        std::to_string(key));
        return tree_attribute;
    }
    void SetXmlAttributeInt(std::uint32_t attribute,
                            std::uint32_t value) override {
        calls.push_back("attr-set " + std::to_string(attribute) + " " +
                        std::to_string(value));
    }
    std::uint32_t ConstructXmlAttribute(std::uint32_t key,
                                        std::uint32_t value) override {
        calls.push_back("attr-construct " + std::to_string(key) + " " +
                        std::to_string(value));
        return 0x8000u;
    }
    void AddXmlAttribute(std::uint32_t document,
                         std::uint32_t attribute) override {
        calls.push_back("attr-add " + std::to_string(document) + " " +
                        std::to_string(attribute));
        if (throw_on_add) throw std::runtime_error("add failed");
    }
    void DestroyXmlAttribute(std::uint32_t attribute) override {
        calls.push_back("attr-destroy " + std::to_string(attribute));
    }
    std::uint32_t ScriptExecutor() override {
        calls.emplace_back("script-executor");
        return 0x5000u;
    }
    void CleanupScriptCache(std::uint32_t cache) override {
        calls.push_back("script-cleanup " + std::to_string(cache));
    }
    std::uint32_t GameInstance() override {
        calls.emplace_back("game-instance");
        return 0x6000u;
    }
    void InitializeStateAndUI(std::uint32_t game,
                              std::uint32_t state,
                              std::uint32_t document) override {
        calls.push_back("transition " + std::to_string(game) + " " +
                        std::to_string(state) + " " +
                        std::to_string(document));
        if (throw_on_transition) throw std::runtime_error("transition failed");
    }
};

std::size_t Position(const Runtime& runtime, const std::string& call) {
    const auto found = std::find(runtime.calls.begin(), runtime.calls.end(), call);
    return found == runtime.calls.end()
               ? runtime.calls.size()
               : static_cast<std::size_t>(found - runtime.calls.begin());
}

void Nonterminal() {
    Runtime runtime;
    runtime.words[0x3000u + 508u] = 3u;
    Check(pal4::CombatSceneUpdate::Advance(0.25f, runtime) == 3u &&
          runtime.ui_delta == 0.25 &&
          Position(runtime, "advance-combat") <
              Position(runtime, "update-ui") &&
          Position(runtime, "doc-construct") == runtime.calls.size(),
          "nonterminal state only advances combat and UI");
}

void DirectAttributeRoute() {
    Runtime existing;
    existing.words[0x3000u + 508u] = 7u;
    existing.words[0x3000u + 504u] = 1u;
    existing.words[pal4::CombatSceneUpdate::kMessageGlobal] = 0x9000u;
    existing.direct_attribute = 0xa000u;
    Check(pal4::CombatSceneUpdate::Advance(0.25f, existing) == 99u &&
          Position(existing, "direct-find 28672 36864") <
              Position(existing, "attr-set 40960 1") &&
          Position(existing, "attr-set 40960 1") <
              Position(existing, "transition 24576 1 28672") &&
          existing.calls.back() == "doc-destroy",
          "normal route updates existing XML attribute and enters state one");

    Runtime missing;
    missing.words[0x3000u + 508u] = 7u;
    Check(pal4::CombatSceneUpdate::Advance(0.25f, missing) == 99u &&
          Position(missing, "direct-find 28672 8657252") <
              Position(missing, "attr-construct 8657252 0") &&
          Position(missing, "attr-add 28672 32768") <
              Position(missing, "attr-destroy 32768") &&
          Position(missing, "attr-destroy 32768") <
              Position(missing, "transition 24576 1 28672"),
          "missing normal attribute uses empty key and destroys temporary");
}

void AlternateRoutes() {
    Runtime cleanup;
    cleanup.words[0x3000u + 508u] = 7u;
    cleanup.bytes[0x3000u + 500u] = 1u;
    Check(pal4::CombatSceneUpdate::Advance(0.25f, cleanup) == 99u &&
          Position(cleanup, "script-cleanup 20564") <
              Position(cleanup, "doc-construct") &&
          Position(cleanup, "transition 24576 7 28672") <
              Position(cleanup, "doc-destroy"),
          "alternate zero result cleans script cache and enters state seven");

    Runtime existing;
    existing.words[0x3000u + 508u] = 7u;
    existing.words[0x3000u + 504u] = 1u;
    existing.bytes[0x3000u + 500u] = 1u;
    existing.words[pal4::CombatSceneUpdate::kMessageGlobal] = 0x9000u;
    existing.tree_attribute = 0xa000u;
    Check(pal4::CombatSceneUpdate::Advance(0.25f, existing) == 99u &&
          Position(existing, "tree-find 28672 36864") <
              Position(existing, "attr-set 40988 1") &&
          Position(existing, "transition 24576 1 28672") <
              Position(existing, "doc-destroy"),
          "alternate nonzero result updates tree attribute field +28");

    Runtime missing;
    missing.words[0x3000u + 508u] = 7u;
    missing.words[0x3000u + 504u] = 1u;
    missing.bytes[0x3000u + 500u] = 1u;
    missing.clear_result_on_document = true;
    Check(pal4::CombatSceneUpdate::Advance(0.25f, missing) == 99u &&
          Position(missing, "tree-find 28672 8657252") <
              Position(missing, "attr-construct 8657252 0") &&
          Position(missing, "transition 24576 1 28672") <
              Position(missing, "doc-destroy"),
          "branch choice uses pre-construction result while attribute rereads it");
}

void CleanupOnException() {
    Runtime transition;
    transition.words[0x3000u + 508u] = 7u;
    transition.throw_on_transition = true;
    bool threw = false;
    try { static_cast<void>(pal4::CombatSceneUpdate::Advance(
        0.25f, transition)); }
    catch (const std::runtime_error&) { threw = true; }
    Check(threw && transition.calls.back() == "doc-destroy",
          "transition failure destroys XML document");

    Runtime add;
    add.words[0x3000u + 508u] = 7u;
    add.throw_on_add = true;
    threw = false;
    try { static_cast<void>(pal4::CombatSceneUpdate::Advance(0.25f, add)); }
    catch (const std::runtime_error&) { threw = true; }
    Check(threw &&
          std::vector<std::string>(add.calls.end() - 2, add.calls.end()) ==
              std::vector<std::string>{"attr-destroy 32768", "doc-destroy"},
          "attribute-add failure destroys temporary attribute and document");
}

}  // namespace

int main() {
    try {
        Nonterminal();
        DirectAttributeRoute();
        AlternateRoutes();
        CleanupOnException();
        std::cout << "combat scene update tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
