#include "pal4/smith_window_requirement_text.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::ISmithWindowRequirementTextRuntime {
    std::vector<pal4::SmithRequirement> requirements;
    std::vector<std::string> calls;
    std::int32_t requested_id = -1;

    void RegisterSmithSystem() override {
        calls.emplace_back("smith");
    }
    std::vector<pal4::SmithRequirement> LoadRequirements(
        std::int32_t prescription_id) override {
        requested_id = prescription_id;
        calls.emplace_back("requirements");
        return requirements;
    }
    std::string_view DialogTemplate(std::uint32_t id) override {
        Check(id == 0xe8u, "material template ID");
        calls.emplace_back("template");
        return "need:%s";
    }
    bool HasItemRecord(std::int32_t id) override {
        calls.emplace_back("item-check");
        return id == 11 || id == 33;
    }
    std::string_view ItemName(std::int32_t id) override {
        Check(id == 11 || id == 33, "item name ID");
        calls.emplace_back("item-name");
        return id == 11 ? "ore" : "dual";
    }
    bool HasElementRecord(std::int32_t id) override {
        calls.emplace_back("element-check");
        return id == 22 || id == 33;
    }
    std::string_view ElementName(std::int32_t id) override {
        Check(id == 22, "element name ID");
        calls.emplace_back("element-name");
        return "fire";
    }
    std::string FormatRequirement(std::string_view format,
                              std::string_view name,std::int32_t quantity) override {
        Check(format == "need:%s", "material format");
        calls.emplace_back("format");
        return "need:" + std::string(name) + "x" + std::to_string(quantity);
    }
};

void CheckMixedRequirements() {
    Runtime runtime;
    runtime.requirements = {
        {11, 100}, {44, 7}, {22, -3}, {33, 0}, {11, 1}};
    Check(pal4::SmithWindowRequirementText::Format(77, runtime) ==
              " need:orex100 need:firex-3 need:dualx0 need:orex1" &&
              runtime.requested_id == 77,
          "item-first, element-second and absent-record formatting");
    Check(runtime.calls == (std::vector<std::string>{
              "smith", "requirements",
              "template", "item-check", "item-name", "format",
              "template", "item-check", "element-check",
              "template", "item-check", "element-check",
              "element-name", "format",
              "template", "item-check", "item-name", "format",
              "template", "item-check", "item-name", "format"}),
          "template lookup and material search order");
}

void CheckEmptyAndMissing() {
    Runtime empty;
    Check(pal4::SmithWindowRequirementText::Format(0, empty).empty() &&
              empty.calls == (std::vector<std::string>{
                  "smith", "requirements"}),
          "empty material collection returns empty text");
    Runtime missing;
    missing.requirements = {{44, 2}};
    Check(pal4::SmithWindowRequirementText::Format(1, missing).empty() &&
              missing.calls == (std::vector<std::string>{
                  "smith", "requirements", "template", "item-check",
                  "element-check"}),
          "missing ID does not append separator");
}

}  // namespace

int main() {
    try {
        CheckMixedRequirements();
        CheckEmptyAndMissing();
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
