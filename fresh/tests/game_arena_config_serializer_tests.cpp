#include "pal4/game_arena_config_serializer.h"

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

struct Runtime final : pal4::IGameArenaConfigRuntime {
    std::vector<std::string> events;
    std::map<std::string, std::string> attributes;
    std::map<std::string, bool> present;
    std::uint8_t write_result = 0xa5u;

    void PrepareTarget(std::uint32_t target,
                       std::string_view marker) override {
        Check(target == 0x6000u && marker == "ARENA_SERIALIZE_FLAG",
              "serialize marker and target");
        events.emplace_back("prepare");
    }
    std::uint32_t CreateNode(std::string_view name) override {
        Check(name == "ARENA", "XML root name");
        events.emplace_back("node");
        return 0x7000u;
    }
    std::string_view ReadCString(std::uint32_t address) override {
        events.push_back("read:" + std::to_string(address));
        switch (address) {
        case 0x5008u: return std::string_view{"Qionghua\0ignored", 16};
        case 0x500cu: return "hall";
        case 0x50e0u: return "hard";
        default: throw std::runtime_error("unexpected arena field");
        }
    }
    bool HasAttribute(std::uint32_t node,
                      std::string_view name) override {
        Check(node == 0x7000u, "find in arena node");
        events.push_back("find:" + std::string(name));
        return present[std::string(name)];
    }
    void SetAttribute(std::uint32_t node, std::string_view name,
                      std::string_view value) override {
        Check(node == 0x7000u, "set in arena node");
        events.push_back("set:" + std::string(name));
        attributes[std::string(name)] = value;
    }
    void AddAttribute(std::uint32_t node, std::string_view name,
                      std::string_view value) override {
        Check(node == 0x7000u, "add in arena node");
        events.push_back("add:" + std::string(name));
        attributes[std::string(name)] = value;
    }
    std::uint8_t Write(std::uint32_t node,
                       std::uint32_t target) override {
        Check(node == 0x7000u && target == 0x6000u,
              "write arena node to target");
        events.emplace_back("write");
        return write_result;
    }
    void DestroyNode(std::uint32_t node) override {
        Check(node == 0x7000u, "destroy arena node");
        events.emplace_back("destroy");
    }
    void ReleaseMarker(std::string_view marker) override {
        Check(marker == "ARENA_SERIALIZE_FLAG", "release marker");
        events.emplace_back("release");
    }
};

void CheckCase(bool world_exists, bool area_exists, bool diff_exists) {
    Runtime runtime;
    runtime.present = {{"world", world_exists}, {"area", area_exists},
                       {"diff", diff_exists}};
    const auto result = pal4::GameArenaConfigSerializer::Serialize(
        0x5000u, 0x6000u, runtime);
    const std::vector<std::string> expected{
        "prepare", "node", "read:20488", "find:world",
        world_exists ? "set:world" : "add:world",
        "read:20492", "find:area",
        area_exists ? "set:area" : "add:area",
        "read:20704", "find:diff",
        diff_exists ? "set:diff" : "add:diff",
        "write", "destroy", "release"};
    Check(result == 0xa5u && runtime.events == expected &&
              runtime.attributes ==
                  (std::map<std::string, std::string>{
                      {"world", "Qionghua"}, {"area", "hall"},
                      {"diff", "hard"}}),
          "attribute upsert, C strings, write and cleanup follow IDA order");
}

}  // namespace

int main() {
    try {
        for (int mask = 0; mask < 8; ++mask) {
            CheckCase((mask & 1) != 0, (mask & 2) != 0,
                      (mask & 4) != 0);
        }
        pal4::GameArenaConfigRuntimeStub stub;
        Check(pal4::GameArenaConfigSerializer::Serialize(
                  0x5000u, 0x6000u, stub) == 0u,
              "native XML stub links and returns failure");
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
