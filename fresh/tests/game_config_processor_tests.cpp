#include "pal4/game_config_processor.h"

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

struct Runtime final : pal4::GameConfigProcessRuntimeStub {
    std::vector<std::string> events;
    std::vector<std::string> child_names;
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    pal4::GameConfigXmlDocument* basic{};
    pal4::GameConfigXmlDocument* player{};
    pal4::ActorControlXmlWriteFacet* facet{};
    std::string fail_at;
    bool allocation_succeeds{true};
    bool shrink_after_first{};
    bool facet_reset_observed{};

    Runtime() {
        words[0x59ecu] = 0x6000u;
        words[0x59f0u] = 0x6008u;
        words[0x6000u] = 0xfffffffbu;
        words[0x6004u] = 42u;
    }
    void Event(std::string event) {
        events.push_back(event);
        if (event == fail_at)
            throw std::runtime_error(event);
    }
    void SetErrorFlag(pal4::GlobalSystemText& text,
                      std::uint8_t flag) override {
        Check(flag == 0u && text.marker == 0x7du &&
                  text.buffer.data == 0u,
              "marker is seeded from the incoming stack byte");
        Event("marker-init");
    }
    bool AllocateText(pal4::GlobalSystemText& text,
                      std::uint32_t length,
                      std::uint8_t mode) override {
        Check(length == 17u && mode == 1u,
              "PR_SERIALIZE_FLAG allocation request");
        Event("marker-allocate");
        if (allocation_succeeds)
            text.buffer.data = 0x7000u;
        return allocation_succeeds;
    }
    void CopyLiteral(std::uint32_t destination,
                     std::string_view value) override {
        Check(destination == 0x7000u && value == "PR_SERIALIZE_FLAG",
              "marker copy");
        Event("marker-copy");
    }
    void WriteByte(std::uint32_t address,
                   std::uint8_t value) override {
        Check(address == 0x7011u && value == 0u,
              "marker null terminator");
        Event("marker-terminate");
    }
    void CheckDataReady(std::uint32_t stream,
                        const pal4::GlobalSystemText& marker) override {
        Check(stream == 0x8123u &&
                  marker.buffer.length ==
                      (allocation_succeeds ? 17u : 0u),
              "stream sees the allocated or empty marker");
        Event("check-data");
    }
    void InitializeDocument(pal4::GameConfigXmlDocument& document,
                            std::string_view root,
                            std::uint32_t parent) override {
        Check(parent == 0u, "both XML documents have no parent");
        if (root == "BASIC") {
            basic = &document;
            Event("init-basic");
        } else {
            Check(root == "Player", "second XML root name");
            player = &document;
            Event("init-player");
        }
    }
    std::uint32_t GameDataCollection() override {
        Event("get-game-data");
        return 0xa000u;
    }
    std::uint32_t InitializeGlobalCollection() override {
        Event("global-init");
        return pal4::GlobalSystem::kCollectionAddress;
    }
    void ForEach(std::uint32_t source,
                 std::uint32_t member,
                 pal4::GameConfigXmlDocument& document) override {
        if (&document == basic) {
            Check(source == 0xa000u && member == 0x5658u,
                  "BASIC collection and object member");
            Event("foreach-basic");
        } else {
            Check(&document == player &&
                      source == pal4::GlobalSystem::kCollectionAddress &&
                      member == 0x5000u,
                  "Player collection and object address");
            Event("foreach-player");
        }
    }
    std::uint32_t CreateNode(pal4::GameConfigXmlDocument& document,
                             std::string_view name) override {
        Check(&document == player && name == "ForbidenSkill",
              "forbidden skill parent and spelling");
        Event("create-forbidden");
        return 0x8888u;
    }
    std::uint32_t CreateChild(std::uint32_t parent,
                              std::string_view name) override {
        Check(parent == 0x8888u, "skill node parent");
        child_names.emplace_back(name);
        Event("child:" + std::string(name));
        if (shrink_after_first)
            words[0x59f0u] = 0x6004u;
        return 0x9000u;
    }
    std::uint32_t ReadDword(std::uint32_t address) override {
        Event("read:" + std::to_string(address));
        return words[address];
    }
    void InitializeFacet(pal4::ActorControlXmlWriteFacet& value,
                         std::uint32_t stream) override {
        Check(stream == 0x8123u, "facet receives output stream");
        facet = &value;
        value.words[0] = 0xdeadbeefu;
        Event("facet-init");
    }
    void WriteConfig(pal4::GameConfigXmlDocument& document,
                     pal4::ActorControlXmlWriteFacet& value) override {
        Check(&value == facet && value.words[0] == 0xdeadbeefu,
              "active facet passed to config writer");
        Event(&document == basic ? "write-basic" : "write-player");
    }
    std::uint8_t SerializeHostServer(std::uint32_t address,
                                     std::uint32_t stream) override {
        Check(address == 0x5938u && stream == 0x8123u,
              "host server member and output stream");
        Event("serialize-host");
        return 0x85u;
    }
    void DestroyDocument(pal4::GameConfigXmlDocument& document) override {
        if (facet) {
            Check(facet->words[0] == 0x8426acu,
                  "facet vtable resets before XML destruction");
            facet_reset_observed = true;
        }
        Event(&document == player ? "destroy-player" : "destroy-basic");
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

void TestCompleteSerialization() {
    Runtime runtime;
    const auto result = pal4::GameConfigProcessor::Process(
        0x5000u, 0x8123u, 0x7du, runtime);
    Check(result == 0x85u, "host server low return byte is forwarded");
    Check(runtime.child_names == std::vector<std::string>{"-5", "42"},
          "skill IDs use signed decimal formatting");
    Check(Position(runtime.events, "check-data") <
              Position(runtime.events, "init-basic") &&
              Position(runtime.events, "foreach-basic") <
                  Position(runtime.events, "init-player") &&
              Position(runtime.events, "foreach-player") <
                  Position(runtime.events, "create-forbidden") &&
              Position(runtime.events, "child:42") <
                  Position(runtime.events, "facet-init") &&
              Position(runtime.events, "write-basic") <
                  Position(runtime.events, "write-player") &&
              Position(runtime.events, "write-player") <
                  Position(runtime.events, "serialize-host") &&
              Position(runtime.events, "serialize-host") <
                  Position(runtime.events, "destroy-player") &&
              Position(runtime.events, "destroy-player") <
                  Position(runtime.events, "destroy-basic") &&
              Position(runtime.events, "destroy-basic") <
                  Position(runtime.events, "release-marker"),
          "serialization and cleanup order");
}

void TestDynamicArrayAndEmptyMarker() {
    Runtime runtime;
    runtime.shrink_after_first = true;
    runtime.allocation_succeeds = false;
    static_cast<void>(pal4::GameConfigProcessor::Process(
        0x5000u, 0x8123u, 0x7du, runtime));
    Check(runtime.child_names == std::vector<std::string>{"-5"},
          "array end is reread after every child");
    Check(std::find(runtime.events.begin(), runtime.events.end(),
                    "marker-copy") == runtime.events.end(),
          "failed allocation skips literal copy");
}

void TestExceptionCleanup() {
    for (const std::string stage :
         {"init-basic", "init-player", "foreach-player",
          "write-basic", "serialize-host"}) {
        Runtime runtime;
        runtime.fail_at = stage;
        bool threw = false;
        try {
            static_cast<void>(pal4::GameConfigProcessor::Process(
                0x5000u, 0x8123u, 0x7du, runtime));
        } catch (const std::runtime_error&) {
            threw = true;
        }
        Check(threw, "injected failure propagates");
        const auto release = Position(runtime.events, "release-marker");
        if (stage != "init-basic") {
            const auto destroy_basic = Position(runtime.events,
                                                "destroy-basic");
            Check(destroy_basic < release,
                  "basic document is destroyed before marker release");
        }
        if (stage == "foreach-player" || stage == "write-basic" ||
            stage == "serialize-host") {
            Check(Position(runtime.events, "destroy-player") <
                      Position(runtime.events, "destroy-basic"),
                  "player document is destroyed first");
        }
        if (stage == "write-basic" || stage == "serialize-host")
            Check(runtime.facet_reset_observed,
                  "facet vtable resets during unwind");
    }
}

}  // namespace

int main() {
    TestCompleteSerialization();
    TestDynamicArrayAndEmptyMarker();
    TestExceptionCleanup();
}
