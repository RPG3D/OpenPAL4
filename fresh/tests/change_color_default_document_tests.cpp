#include "pal4/change_color_default_document.h"

#include <array>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(const bool ok, const char* message) {
    if (!ok) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

constexpr std::array<std::string_view, 9u> kKeys{
    "Attr_Class_Name", "Attr_Task_Wait_Mode", "Attr_Task_Wait_Time",
    "Attr_Task_Expect_Time", "Attr_Task_ChangeColor_Alpha",
    "Attr_Task_ChangeColor_End_Red", "Attr_Task_ChangeColor_End_Green",
    "Attr_Task_ChangeColor_End_Blue", "Attr_Task_ChangeColor_End_Alpha"};

struct Runtime final : pal4::IChangeColorDefaultDocumentRuntime {
    std::vector<std::string> events;
    std::vector<std::string> keys;
    std::vector<std::string> values;
    std::uint32_t next_memory = 0x1000u;
    std::uint32_t existing = 0u;
    std::uint32_t add_count = 0u;
    std::uint32_t destroy_count = 0u;
    std::uint32_t free_count = 0u;
    std::uint32_t fail_add_at = 0u;
    bool fail_init = false;

    std::uint32_t Allocate(const std::uint32_t bytes) override {
        Check(bytes == 0x3cu, "document allocation size");
        events.emplace_back("alloc");
        return next_memory;
    }
    void Free(const std::uint32_t address) override {
        Check(address == 0x1000u, "failed initializer frees allocation");
        ++free_count;
        events.emplace_back("free");
    }
    std::uint32_t InitializeDocument(const std::uint32_t address) override {
        Check(address == 0x1000u, "document initializer receives memory");
        events.emplace_back("init");
        if (fail_init)
            throw std::runtime_error("init");
        return 0x2000u;
    }
    void SetName(const std::uint32_t document,
                 const std::string_view name,
                 const std::uint32_t parent) override {
        Check(document == (next_memory == 0u ? 0u : 0x2000u) &&
                  name == "ChangeColorTask" && parent == 0u,
              "document name and parent");
        events.emplace_back("name");
    }
    std::uint32_t FindAttribute(const std::uint32_t document,
                                const std::string_view key) override {
        Check(document == (next_memory == 0u ? 0u : 0x2000u),
              "attribute search uses document");
        keys.emplace_back(key);
        events.emplace_back("find");
        return existing;
    }
    std::uint32_t FindAlphaAttribute(const std::uint32_t document,
                                     const std::string_view key) override {
        Check(document == (next_memory == 0u ? 0u : 0x2000u) &&
                  key == "Attr_Task_ChangeColor_Alpha",
              "alpha uses dedicated tree lookup");
        keys.emplace_back(key);
        events.emplace_back("alpha-find");
        return existing;
    }
    void SetString(const std::uint32_t attribute,
                   const std::string_view value) override {
        Check(attribute == existing && value == "ChangeColorTask",
              "existing class-name setter");
        values.emplace_back("string:ChangeColorTask");
    }
    void SetInt(const std::uint32_t attribute,
                const std::int32_t value) override {
        Check(attribute == existing, "existing integer setter address");
        values.emplace_back("int:" + std::to_string(value));
    }
    void SetFloat(const std::uint32_t attribute,
                  const float value) override {
        Check(attribute == existing, "existing float setter address");
        values.emplace_back("float:" + std::to_string(value));
    }
    std::uint32_t InitializeStringAttribute(
        pal4::ChangeColorAttributeTemporary&,
        const std::string_view key,
        const std::string_view value) override {
        Check(key == "Attr_Class_Name" && value == "ChangeColorTask",
              "class attribute type and value");
        values.emplace_back("string:ChangeColorTask");
        events.emplace_back("new");
        return 0x3000u;
    }
    std::uint32_t InitializeIntAttribute(
        pal4::ChangeColorAttributeTemporary&,
        const std::string_view,
        const std::int32_t value) override {
        values.emplace_back("int:" + std::to_string(value));
        events.emplace_back("new");
        return 0x3000u;
    }
    std::uint32_t InitializeFloatAttribute(
        pal4::ChangeColorAttributeTemporary&,
        const std::string_view,
        const float value) override {
        values.emplace_back("float:" + std::to_string(value));
        events.emplace_back("new");
        return 0x3000u;
    }
    void AddAttribute(const std::uint32_t document,
                      const std::uint32_t attribute) override {
        Check(document == (next_memory == 0u ? 0u : 0x2000u) &&
                  attribute == 0x3000u,
              "new attribute submission");
        ++add_count;
        events.emplace_back("add");
        if (add_count == fail_add_at)
            throw std::runtime_error("add");
    }
    void DestroyAttribute(pal4::ChangeColorAttributeTemporary&) override {
        ++destroy_count;
        events.emplace_back("destroy");
    }
};

void CheckDefaults(const Runtime& runtime) {
    Check(runtime.keys.size() == kKeys.size(), "nine ordered XML keys");
    for (std::size_t i = 0u; i < kKeys.size(); ++i)
        Check(runtime.keys[i] == kKeys[i], "IDA default-attribute order");
    Check(runtime.values == std::vector<std::string>{
                                "string:ChangeColorTask", "int:2",
                                "float:0.000000", "float:0.000000", "int:0",
                                "int:255", "int:255", "int:255", "int:255"},
          "nine typed default values");
    std::uint32_t alpha_lookups = 0u;
    for (const auto& event : runtime.events)
        alpha_lookups += event == "alpha-find" ? 1u : 0u;
    Check(runtime.events[0u] == "alloc" &&
              runtime.events[1u] == "init" &&
              runtime.events[2u] == "name" &&
              runtime.events[3u] == "find" &&
              alpha_lookups == 1u,
          "document start and special alpha lookup ordering");
}

}  // namespace

int main() {
    Runtime fresh;
    Check(pal4::ChangeColorDefaultDocument::Create(fresh) == 0x2000u &&
              fresh.add_count == 9u && fresh.destroy_count == 9u,
          "new document creates and releases nine temporary attributes");
    CheckDefaults(fresh);

    Runtime existing;
    existing.existing = 0x4000u;
    Check(pal4::ChangeColorDefaultDocument::Create(existing) == 0x2000u &&
              existing.add_count == 0u && existing.destroy_count == 0u,
          "existing attributes are updated in place");
    CheckDefaults(existing);

    Runtime no_memory;
    no_memory.next_memory = 0u;
    Check(pal4::ChangeColorDefaultDocument::Create(no_memory) == 0u &&
              no_memory.add_count == 9u,
          "null allocation preserves native document flow");

    Runtime failing_add;
    failing_add.fail_add_at = 5u;
    bool caught = false;
    try {
        (void)pal4::ChangeColorDefaultDocument::Create(failing_add);
    } catch (const std::runtime_error&) {
        caught = true;
    }
    Check(caught && failing_add.add_count == 5u &&
              failing_add.destroy_count == 5u &&
              failing_add.keys[4u] == "Attr_Task_ChangeColor_Alpha",
          "special alpha temporary is destroyed on add failure");

    Runtime failing_init;
    failing_init.fail_init = true;
    caught = false;
    try {
        (void)pal4::ChangeColorDefaultDocument::Create(failing_init);
    } catch (const std::runtime_error&) {
        caught = true;
    }
    Check(caught && failing_init.free_count == 1u &&
              failing_init.keys.empty(),
          "document initialization failure frees allocation");
}
