#include "pal4/game_arena_config_deserializer.h"

#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IGameArenaConfigDeserializeRuntime {
    std::map<std::uint32_t, std::uint32_t> words{{0x50ecu, 0u}};
    std::vector<std::string> events;
    std::string marker = "ARENA_SERIALIZE_FLAG";
    bool xml_valid = true;
    std::map<std::string, std::string> attributes{
        {"world", std::string("world\0tail", 10)},
        {"area", "hall"}, {"diff", "hard"}};

    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        events.push_back("write:" + std::to_string(address) + ":" +
                         std::to_string(value));
        words[address] = value;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        Check(address == 0x50ecu, "script status offset");
        events.emplace_back("read-status");
        return words[address];
    }
    std::string_view ReadSerializedMarker(std::uint32_t source) override {
        Check(source == 0x6000u, "serialized source handle");
        events.emplace_back("read-input");
        return marker;
    }
    std::uint32_t CreateNode(std::string_view name) override {
        Check(name == "ARENA", "root node name");
        events.emplace_back("create-node");
        return 0x7000u;
    }
    bool CheckVersionAndType(std::uint32_t source,
                             std::uint32_t node) override {
        Check(source == 0x6000u && node == 0x7000u,
              "XML parser context");
        events.emplace_back("check-xml");
        return xml_valid;
    }
    std::string_view ReadAttribute(std::uint32_t node,
                                   std::string_view name,
                                   std::string_view default_value) override {
        Check(node == 0x7000u && default_value.empty(),
              "attribute default and node");
        events.push_back("attribute:" + std::string(name));
        return attributes[std::string(name)];
    }
    void LoadSceneResources(std::uint32_t arena,
                            std::string_view world,
                            std::string_view area,
                            std::uint32_t mode) override {
        Check(arena == 0x5000u && world == "world" && area == "hall" &&
                  mode == 1u,
              "resource loader arguments");
        events.emplace_back("resources");
    }
    void LoadSceneRuntime(std::uint32_t arena,
                          std::string_view scene) override {
        Check(arena == 0x5000u && scene == "hard",
              "runtime loader receives diff attribute");
        events.emplace_back("runtime");
    }
    void InitializePlayerScene(std::uint32_t arena) override {
        Check(arena == 0x5000u, "player scene arena");
        events.emplace_back("player");
    }
    void ReleaseAttribute(std::string_view name) override {
        events.push_back("release:" + std::string(name));
    }
    void DestroyNode(std::uint32_t node) override {
        Check(node == 0x7000u, "destroy XML node");
        events.emplace_back("destroy-node");
    }
    void ReleaseInput() override { events.emplace_back("release-input"); }
};

void RejectedInput() {
    Runtime runtime;
    runtime.marker = "OTHER";
    Check(!pal4::GameArenaConfigDeserializer::Deserialize(
              0x5000u, 0x6000u, runtime),
          "marker mismatch rejects input");
    Check(runtime.events == (std::vector<std::string>{
              "write:20676:0", "write:20924:1", "read-input",
              "release-input"}) &&
              runtime.words[0x50c4u] == 0u &&
              runtime.words[0x51bcu] == 1u,
          "marker failure resets flags and releases source only");

    runtime = Runtime{};
    runtime.marker += "_EXTRA";
    Check(!pal4::GameArenaConfigDeserializer::Deserialize(
              0x5000u, 0x6000u, runtime) &&
              runtime.events == (std::vector<std::string>{
                  "write:20676:0", "write:20924:1", "read-input",
                  "release-input"}),
          "marker compare rejects a longer prefix match");

    runtime = Runtime{};
    runtime.xml_valid = false;
    Check(!pal4::GameArenaConfigDeserializer::Deserialize(
              0x5000u, 0x6000u, runtime),
          "invalid XML rejects input");
    Check(runtime.events == (std::vector<std::string>{
              "write:20676:0", "write:20924:1", "read-input",
              "create-node", "check-xml", "destroy-node", "release-input"}),
          "XML failure destroys node before releasing source");
}

void AcceptedInput() {
    for (const auto script_status : {0u, 1u}) {
        Runtime runtime;
        runtime.words[0x50ecu] = script_status;
        Check(pal4::GameArenaConfigDeserializer::Deserialize(
                  0x5000u, 0x6000u, runtime),
              "valid arena XML returns success");
        auto expected = std::vector<std::string>{
            "write:20676:0", "write:20924:1", "read-input",
            "create-node", "check-xml", "attribute:world",
            "attribute:area", "attribute:diff", "resources", "runtime",
            "player", "read-status"};
        if (script_status == 0u) {
            expected.push_back("write:20676:1");
            expected.push_back("write:20924:0");
        }
        expected.insert(expected.end(), {
            "release:diff", "release:area", "release:world",
            "destroy-node", "release-input"});
        Check(runtime.events == expected &&
                  runtime.words[0x50c4u] == (script_status == 0u ? 1u : 0u) &&
                  runtime.words[0x51bcu] == (script_status == 0u ? 0u : 1u),
              "scene load, script flag and reverse cleanup follow IDA");
    }

    pal4::GameArenaConfigDeserializeRuntimeStub stub;
    Check(!pal4::GameArenaConfigDeserializer::Deserialize(
              0x5000u, 0x6000u, stub),
          "native XML stub links and rejects empty source");
}

}  // namespace

int main() {
    try {
        RejectedInput();
        AcceptedInput();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
