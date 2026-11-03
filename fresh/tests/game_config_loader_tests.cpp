#include "pal4/game_config_loader.h"

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::GameConfigLoadRuntimeStub {
    std::vector<std::string> events;
    std::vector<std::int32_t> inserted;
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    pal4::GameConfigXmlDocument* player{};
    pal4::GameConfigXmlDocument* basic{};
    pal4::ActorControlXmlWriteFacet* facet{};
    std::string fail_at;
    bool marker_matches{true};
    bool mp_present{};
    bool forbidden_present{true};
    bool facet_reset_observed{};

    Runtime() {
        words[0x59ecu] = 0xa000u;
        words[0x59f0u] = 0xa008u;
        words[0xb038u] = 2u;
    }
    void Event(std::string event) {
        events.push_back(event);
        if (event == fail_at)
            throw std::runtime_error(event);
    }
    void SetErrorFlag(pal4::GlobalSystemText& marker,
                      std::uint8_t flag) override {
        if (flag == 0u) {
            Check(marker.marker == 0x6cu, "incoming stack byte");
            Event("marker-init");
        } else {
            Event("marker-reset");
        }
    }
    void FillMarker(std::uint32_t stream,
                    pal4::GlobalSystemText&) override {
        Check(stream == 0x8123u, "marker stream");
        Event("fill-marker");
    }
    std::int32_t CompareMarker(const pal4::GlobalSystemText&,
                               std::string_view expected) override {
        Check(expected == "PR_SERIALIZE_FLAG", "marker constant");
        Event("compare-marker");
        return marker_matches ? 0 : 1;
    }
    void InitializeDocument(pal4::GameConfigXmlDocument& document,
                            std::string_view root,
                            std::uint32_t parent) override {
        Check(parent == 0u, "XML root has no parent");
        if (root == "Player") {
            player = &document;
            Event("init-player");
        } else {
            Check(root == "BASIC", "second root name");
            basic = &document;
            Event("init-basic");
        }
    }
    void InitializeFacet(pal4::ActorControlXmlWriteFacet& value,
                         std::uint32_t stream) override {
        Check(stream == 0x8123u, "facet stream");
        facet = &value;
        value.words[0] = 0xdeadbeefu;
        Event("facet-init");
    }
    void ReadConfig(pal4::ActorControlXmlWriteFacet& value,
                    pal4::GameConfigXmlDocument& document) override {
        Check(&value == facet && value.words[0] == 0xdeadbeefu,
              "active facet reads config");
        Event(&document == basic ? "read-basic" : "read-player");
    }
    std::uint32_t GameDataCollection() override {
        Event("get-game-data");
        return 0xc000u;
    }
    std::uint32_t InitializeGlobalCollection() override {
        Event("global-init");
        return pal4::GlobalSystem::kCollectionAddress;
    }
    void SetProperties(std::uint32_t source,
                       std::uint32_t member,
                       pal4::GameConfigXmlDocument& document) override {
        if (&document == basic) {
            Check(source == 0xc000u && member == 0x5658u,
                  "basic property collection and target");
            Event("set-basic");
        } else {
            Check(&document == player &&
                      source == pal4::GlobalSystem::kCollectionAddress &&
                      member == 0x5000u,
                  "player property collection and target");
            Event("set-player");
        }
    }
    std::uint32_t FindAttribute(pal4::GameConfigXmlDocument& document,
                                std::string_view name) override {
        Check(&document == player, "properties come from Player XML");
        Event("find:" + std::string(name));
        if (name == "CA_HP") return 0xd000u;
        Check(name == "CA_MP", "second property name");
        return mp_present ? 0xd100u : 0u;
    }
    std::int32_t AttributeToInt(std::uint32_t attribute,
                                std::int32_t& initial_value) override {
        Check(initial_value == 0, "attribute default is zero");
        Event("convert:" + std::to_string(attribute));
        return attribute == 0xd000u ? 88 : -7;
    }
    void SetPlayerProperty(std::uint32_t script,
                           std::uint32_t index,
                           std::int32_t value) override {
        Check(script == 0x5654u, "script member offset");
        Event("property:" + std::to_string(index) + ":" +
              std::to_string(value));
    }
    void DeserializeHostServer(std::uint32_t host,
                               std::uint32_t stream) override {
        Check(host == 0x5938u && stream == 0x8123u,
              "host server member and stream");
        Event("host-deserialize");
    }
    std::uint32_t ReadDword(std::uint32_t address) override {
        Event("read:" + std::to_string(address));
        return words[address];
    }
    void WriteDword(std::uint32_t address,
                    std::uint32_t value) override {
        Check(address == 0x59f0u && value == 0xa000u,
              "old forbidden skill array is cleared");
        words[address] = value;
        Event("clear-skills");
    }
    std::uint32_t FindChild(pal4::GameConfigXmlDocument& document,
                            std::string_view name) override {
        Check(&document == player && name == "ForbidenSkill",
              "forbidden skill node lookup");
        Event("find-forbidden");
        return forbidden_present ? 0xb000u : 0u;
    }
    std::uint32_t ChildByIndex(std::uint32_t parent,
                               std::int32_t& index) override {
        Check(parent == 0xb000u && index >= 0 && index < 2,
              "indexed forbidden child");
        Event("child:" + std::to_string(index));
        return 0xb100u + static_cast<std::uint32_t>(index) * 4u;
    }
    const char* ChildText(std::uint32_t child) override {
        Event("child-text");
        return child == 0xb100u ? "-3" : nullptr;
    }
    void InsertForbiddenSkill(std::uint32_t array,
                              std::uint32_t end,
                              std::uint32_t mode,
                              std::int32_t value) override {
        Check(array == 0x59e8u && mode == 1u &&
                  end == 0xa000u + inserted.size() * 4u,
              "skill insertion uses current end pointer");
        inserted.push_back(value);
        words[0x59f0u] += 4u;
        Event("insert:" + std::to_string(value));
    }
    void ProcessPlayerCollections(std::uint32_t object) override {
        Check(object == 0x5000u, "player collection object");
        Event("player-collections");
    }
    void FinalizePlayerScript(std::uint32_t script) override {
        Check(script == 0x5654u, "player script virtual target");
        Event("finalize-player");
    }
    void ProcessScript(std::uint32_t object) override {
        Check(object == 0x5000u, "script cleanup object");
        Event("process-script");
    }
    void DestroyDocument(pal4::GameConfigXmlDocument& document) override {
        if (facet) {
            Check(facet->words[0] == 0x8426acu,
                  "facet resets before XML destruction");
            facet_reset_observed = true;
        }
        Event(&document == basic ? "destroy-basic" : "destroy-player");
    }
    void ReleaseText(pal4::GlobalSystemText&) override {
        Event("release-marker");
    }
};

std::size_t Position(const std::vector<std::string>& events,
                     const std::string& event) {
    const auto it = std::find(events.begin(), events.end(), event);
    Check(it != events.end(), "expected event exists");
    return static_cast<std::size_t>(it - events.begin());
}

void TestLoad() {
    Runtime runtime;
    Check(pal4::GameConfigLoader::Load(0x5000u, 0x8123u, 0x6cu,
                                       runtime) == 1u,
          "matching marker returns success");
    Check(runtime.inserted == std::vector<std::int32_t>{-3, 0},
          "atoi parses signed text and null child text as zero");
    Check(Position(runtime.events, "init-player") <
              Position(runtime.events, "init-basic") &&
              Position(runtime.events, "read-basic") <
                  Position(runtime.events, "read-player") &&
              Position(runtime.events, "set-basic") <
                  Position(runtime.events, "set-player") &&
              Position(runtime.events, "property:7:88") <
                  Position(runtime.events, "property:9:0") &&
              Position(runtime.events, "host-deserialize") <
                  Position(runtime.events, "clear-skills") &&
              Position(runtime.events, "clear-skills") <
                  Position(runtime.events, "insert:-3") &&
              Position(runtime.events, "insert:0") <
                  Position(runtime.events, "player-collections") &&
              Position(runtime.events, "player-collections") <
                  Position(runtime.events, "finalize-player") &&
              Position(runtime.events, "finalize-player") <
                  Position(runtime.events, "process-script") &&
              Position(runtime.events, "destroy-basic") <
                  Position(runtime.events, "destroy-player") &&
              Position(runtime.events, "destroy-player") <
                  Position(runtime.events, "release-marker"),
          "load and reverse cleanup order");
}

void TestMismatchAndMissingNodes() {
    Runtime mismatch;
    mismatch.marker_matches = false;
    Check(pal4::GameConfigLoader::Load(0x5000u, 0x8123u, 0x6cu,
                                       mismatch) == 0u,
          "bad marker returns failure");
    Check(mismatch.events.back() == "marker-reset" &&
              mismatch.player == nullptr,
          "bad marker resets text and skips XML");

    Runtime absent;
    absent.mp_present = true;
    absent.forbidden_present = false;
    Check(pal4::GameConfigLoader::Load(0x5000u, 0x8123u, 0x6cu,
                                       absent) == 1u,
          "missing forbidden node does not fail load");
    Check(absent.inserted.empty() &&
              Position(absent.events, "property:9:-7") <
                  Position(absent.events, "host-deserialize"),
          "present MP converts and absent skill node is skipped");
}

void TestExceptionCleanup() {
    for (const std::string stage :
         {"init-player", "init-basic", "read-basic",
          "set-player", "process-script"}) {
        Runtime runtime;
        runtime.fail_at = stage;
        bool threw = false;
        try {
            static_cast<void>(pal4::GameConfigLoader::Load(
                0x5000u, 0x8123u, 0x6cu, runtime));
        } catch (const std::runtime_error&) {
            threw = true;
        }
        Check(threw, "injected load failure propagates");
        const auto release = Position(runtime.events, "release-marker");
        if (stage != "init-player")
            Check(Position(runtime.events, "destroy-player") < release,
                  "player XML is released before marker");
        if (stage == "read-basic" || stage == "set-player" ||
            stage == "process-script") {
            Check(Position(runtime.events, "destroy-basic") <
                      Position(runtime.events, "destroy-player"),
                  "basic XML is released before player XML");
            Check(runtime.facet_reset_observed,
                  "facet reset precedes XML cleanup");
        }
    }
}

}  // namespace

int main() {
    TestLoad();
    TestMismatchAndMissingNodes();
    TestExceptionCleanup();
}
