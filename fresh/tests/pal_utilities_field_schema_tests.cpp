#include "pal4/pal_utilities_field_schema.h"

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

struct Call {
    std::string action;
    std::string name;
    std::uint32_t offset = 0;
    std::uint32_t kind = 0;
    bool operator==(const Call&) const = default;
};

struct Runtime final : pal4::IUtilitiesFieldSchemaRuntime {
    std::vector<Call> calls;
    std::string fail_name;

    void BeginText(std::string_view name) override {
        calls.push_back({"begin", std::string(name)});
    }
    void AddField(std::uint32_t object,
                  pal4::UtilitiesFieldKind kind,
                  std::string_view name,
                  std::uint32_t offset) override {
        Check(object == 0x7000, "schema uses its input object");
        calls.push_back({"field", std::string(name), offset,
                         kind == pal4::UtilitiesFieldKind::Integer
                             ? 0u : 1u});
        if (name == fail_name) throw std::runtime_error("field failed");
    }
    void ReleaseText() override {
        calls.push_back({"release", {}});
    }
};

void CheckIdString() {
    Runtime runtime;
    Check(pal4::PalUtilitiesFieldSchema::RegisterIdString(
              0x7000, runtime) == 1 &&
              runtime.calls == std::vector<Call>{
                  {"begin", "ID"},
                  {"field", "ID", 0, 0},
                  {"release", {}},
                  {"begin", "STRING"},
                  {"field", "STRING", 4, 1},
                  {"release", {}}},
          "ID and STRING schema uses observed kinds, offsets and order");
}

void CheckSceneFields() {
    Runtime runtime;
    Check(pal4::PalUtilitiesFieldSchema::RegisterSceneFields(
              0x7000, runtime) == 1 &&
              runtime.calls == std::vector<Call>{
                  {"begin", "ID"},
                  {"field", "ID", 0, 0},
                  {"release", {}},
                  {"begin", "SCENE"},
                  {"field", "SCENE", 4, 1},
                  {"release", {}},
                  {"begin", "SECTION"},
                  {"field", "SECTION", 20, 1},
                  {"release", {}},
                  {"begin", "NAME"},
                  {"field", "NAME", 36, 1},
                  {"release", {}},
                  {"begin", "TYPE"},
                  {"field", "TYPE", 52, 0},
                  {"release", {}},
                  {"begin", "EARTHBALL"},
                  {"field", "EARTHBALL", 56, 0},
                  {"release", {}}},
          "six-field scene schema matches IDA names, kinds and offsets");
}

void CheckFailureCleanup() {
    Runtime runtime;
    runtime.fail_name = "SECTION";
    try {
        static_cast<void>(
            pal4::PalUtilitiesFieldSchema::RegisterSceneFields(
                0x7000, runtime));
        Check(false, "field failure must propagate");
    } catch (const std::runtime_error& error) {
        Check(std::string(error.what()) == "field failed" &&
                  runtime.calls.size() == 9 &&
                  runtime.calls[7] ==
                      Call{"field", "SECTION", 20, 1} &&
                  runtime.calls.back() == Call{"release", {}},
              "temporary SECTION text releases after failed registration");
    }
}

using RegisterFunction = std::uint8_t (*)(
    std::uint32_t, pal4::IUtilitiesFieldSchemaRuntime&);

void CheckFields(RegisterFunction registration,
                 const std::vector<Call>& expected_fields) {
    Runtime runtime;
    Check(registration(0x7000, runtime) == 1,
          "field registration returns one");
    std::vector<Call> expected_calls;
    for (const auto& field : expected_fields) {
        expected_calls.push_back({"begin", field.name});
        expected_calls.push_back(field);
        expected_calls.push_back({"release", {}});
    }
    Check(runtime.calls == expected_calls,
          "field names, kinds, offsets and call order match IDA");

    for (std::size_t failed = 0; failed < expected_fields.size(); ++failed) {
        Runtime failing_runtime;
        failing_runtime.fail_name = expected_fields[failed].name;
        bool threw = false;
        try {
            static_cast<void>(registration(0x7000, failing_runtime));
        } catch (const std::runtime_error& error) {
            threw = std::string(error.what()) == "field failed";
        }
        Check(threw &&
                  failing_runtime.calls.size() == (failed + 1) * 3 &&
                  failing_runtime.calls.back() == Call{"release", {}},
              "field failure releases current temporary and stops");
    }
}

void CheckAdditionalSchemas() {
    using Schema = pal4::PalUtilitiesFieldSchema;
    CheckFields(&Schema::RegisterPreviewFields, {
        {"field", "ID", 0, 0},
        {"field", "PREVIEW", 4, 1},
        {"field", "DESC", 20, 1},
        {"field", "FILE", 36, 1},
    });
    CheckFields(&Schema::RegisterIntroductionFields, {
        {"field", "ID", 0, 0},
        {"field", "NAME", 4, 1},
        {"field", "INTRODUCTION", 20, 1},
    });
    CheckFields(&Schema::RegisterWindowFields, {
        {"field", "ID", 0, 0},
        {"field", "WINDOW", 4, 0},
        {"field", "TIPS", 24, 1},
        {"field", "CLASS", 8, 1},
        {"field", "QUEST_ID", 40, 0},
    });
    CheckFields(&Schema::RegisterWorldFields, {
        {"field", "ID", 0, 0},
        {"field", "WORLD", 4, 1},
        {"field", "COMBAT_SCENE", 24, 1},
        {"field", "FLOOR", 20, 0},
    });
    CheckFields(&Schema::RegisterStringIdFields, {
        {"field", "ID", 0, 0},
        {"field", "STRING_ID", 4, 0},
    });
}

}  // namespace

int main() {
    try {
        CheckIdString();
        CheckSceneFields();
        CheckFailureCleanup();
        CheckAdditionalSchemas();
        std::cout << "pal_utilities_field_schema_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
