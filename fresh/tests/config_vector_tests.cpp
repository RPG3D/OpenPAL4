#include "pal4/config_vector.h"

#include <cstdlib>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IConfigVectorRuntime {
    std::map<std::string, std::uint32_t> x_values;
    std::map<std::string, std::uint32_t> nodes;
    std::map<std::uint32_t, float> floats;
    std::vector<std::string> events;
    std::uint32_t next_attribute = 0xa000u;
    bool fail_add = false;

    [[nodiscard]] std::uint32_t GetConfigValue(
        const std::uint32_t config,
        const std::string_view key) override {
        Check(config == 0x5000u, "looks up vector in supplied config");
        events.emplace_back("get:" + std::string(key));
        return x_values[std::string(key)];
    }
    [[nodiscard]] std::uint32_t FindAttributeNode(
        const std::uint32_t config,
        const std::string_view key) override {
        Check(config == 0x5000u, "finds vector in supplied tree");
        events.emplace_back("find:" + std::string(key));
        return nodes[std::string(key)];
    }
    [[nodiscard]] std::uint32_t ReadWord(
        const std::uint32_t address) override {
        Check(address == 0x5018u, "reads tree header at config plus 24");
        return 0x9000u;
    }
    void SetFloat(const std::uint32_t attribute,
                  const float value) override {
        floats[attribute] = value;
        events.emplace_back("set:" + std::to_string(attribute));
    }
    [[nodiscard]] std::uint32_t CreateFloatAttribute(
        const std::string_view name, const float value) override {
        const auto attribute = next_attribute++;
        floats[attribute] = value;
        events.emplace_back("create:" + std::string(name));
        return attribute;
    }
    void AddAttribute(const std::uint32_t config,
                      const std::uint32_t attribute) override {
        Check(config == 0x5000u, "adds attribute to supplied config");
        events.emplace_back("add:" + std::to_string(attribute));
        if (fail_add) throw std::runtime_error("test add failure");
    }
    void DestroyAttribute(const std::uint32_t attribute) override {
        events.emplace_back("destroy:" + std::to_string(attribute));
    }
    [[nodiscard]] float ToFloat(const std::uint32_t attribute,
                                const float fallback) override {
        Check(fallback == 0.0f, "missing component defaults to zero");
        events.emplace_back("float:" + std::to_string(attribute));
        return floats[attribute];
    }
    [[nodiscard]] std::uint8_t ReleaseNameBuffer() override {
        events.emplace_back("release-name");
        return 7u;
    }
};

}  // namespace

int main() {
    Runtime runtime;
    runtime.x_values["Axis_X"] = 0x1000u;
    runtime.nodes["Axis_Y"] = 0x2000u;
    runtime.nodes["Axis_Z"] = 0xffffffe4u;
    Check(pal4::ConfigVector::Set(
              0x5000u, "Axis", {1.0f, 2.0f, 3.0f}, runtime) == 7u &&
              runtime.floats[0x1000u] == 1.0f &&
              runtime.floats[0x201cu] == 2.0f &&
              runtime.floats[0xa000u] == 3.0f &&
              runtime.events == std::vector<std::string>({
                  "get:Axis_X", "set:4096", "find:Axis_Y", "set:8220",
                  "find:Axis_Z", "create:Axis_Z", "add:40960",
                  "destroy:40960", "release-name"}),
          "setter updates existing X/Y and creates missing Z in order");
    runtime.events.clear();
    runtime.x_values["New_X"] = 0u;
    runtime.nodes["New_Y"] = 0x9000u;
    runtime.nodes["New_Z"] = 0xffffffe4u;
    Check(pal4::ConfigVector::Set(
              0x5000u, "New", {-1.0f, 0.0f, 9.0f}, runtime) == 7u &&
              runtime.events[1] == "create:New_X" &&
              runtime.events[4] == "find:New_Y" &&
              runtime.events[5] == "create:New_Y" &&
              runtime.events[9] == "create:New_Z" &&
              runtime.events.back() == "release-name",
          "setter handles absent X and both native tree-miss sentinels");

    runtime.events.clear();
    pal4::ConfigVector3 output{99.0f, 99.0f, 99.0f};
    runtime.floats[0x1000u] = 4.5f;
    runtime.floats[0x201cu] = -2.0f;
    Check(&pal4::ConfigVector::Get(output, 0x5000u, "Axis", runtime) ==
              &output && output.x == 4.5f && output.y == -2.0f &&
              output.z == 0.0f && runtime.events.back() == "release-name",
          "getter converts existing components and zeros missing Z");
    runtime.events.clear();
    Check(&pal4::ConfigVector::Get(output, 0x5000u, "New", runtime) ==
              &output && output.x == 0.0f && output.y == 0.0f &&
              output.z == 0.0f,
          "getter defaults missing X and both tree-miss sentinels to zero");

    runtime.events.clear();
    runtime.fail_add = true;
    bool threw = false;
    try {
        (void)pal4::ConfigVector::Set(
            0x5000u, "New", {1.0f, 2.0f, 3.0f}, runtime);
    } catch (const std::runtime_error&) {
        threw = true;
    }
    Check(threw && runtime.events.back() == "release-name" &&
              runtime.events[runtime.events.size() - 2u].starts_with("destroy:"),
          "failed XML add destroys the attribute and name buffer");
    pal4::ConfigVectorRuntimeStub stub;
    Check(pal4::ConfigVector::Set(0u, "v", {}, stub) == 0u,
          "config and XML boundary has a linkable stub");
    return EXIT_SUCCESS;
}
