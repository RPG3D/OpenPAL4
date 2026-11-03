#include "pal4/gob_local_float_snapshot.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IGobLocalFloatSnapshotRuntime {
    static constexpr std::uint32_t snapshot = 0x2000u;
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, float> floats;
    std::map<std::string, std::int32_t> xml_ints;
    std::map<std::string, float> xml_floats;
    std::vector<std::string> calls;
    std::vector<std::pair<std::string, std::string>> strings_written;
    std::vector<std::pair<std::string, std::int32_t>> ints_written;
    std::vector<std::pair<std::string, float>> floats_written;
    std::uint32_t allocation = snapshot;
    bool fail_name_init = false;
    bool xml_read_success = true;
    std::string name = "Raft";
    std::string xml_name = "Platform";

    std::uint32_t Allocate(std::uint32_t size) override {
        Check(size == pal4::GobLocalFloatStateSnapshot::kSize,
              "local float snapshot size");
        calls.emplace_back("allocate");
        return allocation;
    }
    void Free(std::uint32_t address) override {
        Check(address == snapshot, "local float snapshot free");
        calls.emplace_back("free");
    }
    void InitializeNameRecord(std::uint32_t record) override {
        Check(record == snapshot + 4u,
              "local float snapshot name init");
        calls.emplace_back("initialize name");
        if (fail_name_init) throw std::runtime_error("name init failed");
    }
    std::uint32_t ReleaseNameRecord(std::uint32_t record) override {
        Check(record == snapshot + 4u,
              "local float snapshot name release");
        calls.emplace_back("release name");
        return 0x4444u;
    }
    std::string ReadNameRecord(std::uint32_t record) override {
        Check(record == snapshot + 4u,
              "local float snapshot name read");
        calls.emplace_back("read name");
        return name;
    }
    void AssignNameRecord(std::uint32_t record,
                          std::string_view value) override {
        Check(record == snapshot + 4u,
              "local float snapshot name assignment");
        name = value;
        calls.emplace_back("assign name");
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words.at(address);
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
        calls.emplace_back("write word");
    }
    float ReadFloat(std::uint32_t address) override {
        return floats.at(address);
    }
    void WriteFloat(std::uint32_t address, float value) override {
        floats[address] = value;
        calls.emplace_back("write float");
    }
    std::uint32_t CreateXmlDocument(std::string_view root) override {
        Check(root == "soGOBLocalFloat",
              "local float snapshot XML root");
        calls.emplace_back("create XML document");
        return 0x8000u;
    }
    void WriteXmlString(std::uint32_t document,
                        std::string_view key,
                        std::string_view value) override {
        Check(document == 0x8000u, "snapshot XML string document");
        strings_written.emplace_back(key, value);
        calls.emplace_back("write XML string");
    }
    void WriteXmlInt(std::uint32_t document,
                     std::string_view key,
                     std::int32_t value) override {
        Check(document == 0x8000u, "snapshot XML integer document");
        ints_written.emplace_back(key, value);
        calls.emplace_back("write XML int");
    }
    void WriteXmlFloat(std::uint32_t document,
                       std::string_view key,
                       float value) override {
        Check(document == 0x8000u, "snapshot XML float document");
        floats_written.emplace_back(key, value);
        calls.emplace_back("write XML float");
    }
    void FlushXml(std::uint32_t document,
                  std::uint32_t stream) override {
        Check(document == 0x8000u && stream == 0x9000u,
              "local float snapshot XML output");
        calls.emplace_back("flush XML");
    }
    bool ReadXml(std::uint32_t document,
                 std::uint32_t stream) override {
        Check(document == 0x8000u && stream == 0x9000u,
              "local float snapshot XML input");
        calls.emplace_back("read XML");
        return xml_read_success;
    }
    std::string ReadXmlString(std::uint32_t document,
                              std::string_view key,
                              std::string_view fallback) override {
        Check(document == 0x8000u && key == "valName" &&
                  fallback.empty(), "local float XML name key");
        calls.emplace_back("read XML string");
        return xml_name;
    }
    std::int32_t ReadXmlInt(std::uint32_t document,
                            std::string_view key,
                            std::int32_t fallback) override {
        Check(document == 0x8000u && fallback == 0 &&
                  (key == "valVisible" || key == "researchNum"),
              "local float XML integer key/default");
        calls.emplace_back("read XML int");
        const auto found = xml_ints.find(std::string(key));
        return found == xml_ints.end() ? fallback : found->second;
    }
    float ReadXmlFloat(std::uint32_t document,
                       std::string_view key,
                       float fallback) override {
        static constexpr std::array<std::string_view, 6> keys{
            "valPosX", "valPosY", "valPosZ",
            "valRotX", "valRotY", "valRotZ"};
        Check(document == 0x8000u && fallback == 0.0f &&
                  std::find(keys.begin(), keys.end(), key) != keys.end(),
              "local float XML float key/default");
        calls.emplace_back(std::string(key));
        const auto found = xml_floats.find(std::string(key));
        return found == xml_floats.end() ? fallback : found->second;
    }
    std::uint32_t DestroyXmlDocument(
        std::uint32_t document) override {
        Check(document == 0x8000u,
              "local float snapshot XML cleanup");
        calls.emplace_back("destroy XML document");
        return 0x7777u;
    }
};

void CheckLifecycle() {
    Check(pal4::GobLocalFloatStateSnapshot::TypeKey() == 2u,
          "local float snapshot type thunk");
    Runtime unavailable;
    unavailable.allocation = 0u;
    Check(pal4::GobLocalFloatStateSnapshot::Create(unavailable) == 0u &&
              unavailable.calls == (std::vector<std::string>{"allocate"}),
          "local float snapshot allocation failure");
    Runtime created;
    Check(pal4::GobLocalFloatStateSnapshot::Create(created) ==
              Runtime::snapshot &&
              created.words.at(Runtime::snapshot) ==
                  pal4::GobLocalFloatStateSnapshot::kVtable &&
              created.words.at(Runtime::snapshot + 8u) == 0u &&
              created.words.at(Runtime::snapshot + 0xcu) == 0u &&
              created.words.at(Runtime::snapshot + 0x10u) == 0u &&
              created.calls[0] == "allocate" &&
              created.calls[1] == "initialize name",
          "local float snapshot construction layout");
    created.calls.clear();
    Check(pal4::GobLocalFloatStateSnapshot::Destruct(
              Runtime::snapshot, created) == 0x4444u &&
              created.words.at(Runtime::snapshot) ==
                  pal4::GobLocalFloatStateSnapshot::kBaseVtable &&
              created.calls == (std::vector<std::string>{
                  "write word", "release name", "write word",
                  "write word", "write word", "write word"}),
          "local float snapshot destructor order");
    Runtime failed;
    failed.fail_name_init = true;
    bool thrown = false;
    try {
        (void)pal4::GobLocalFloatStateSnapshot::Create(failed);
    } catch (const std::runtime_error&) {
        thrown = true;
    }
    Check(thrown && failed.calls == (std::vector<std::string>{
              "allocate", "initialize name", "free"}),
          "local float snapshot constructor exception cleanup");
}

void CheckXml() {
    Runtime writer;
    writer.words[Runtime::snapshot + 0x14u] = 0xffffffffu;
    writer.words[Runtime::snapshot + 0x30u] = 9u;
    for (std::uint32_t i = 0u; i < 6u; ++i)
        writer.floats[Runtime::snapshot + 0x18u + i * 4u] =
            static_cast<float>(i + 1u);
    Check(pal4::GobLocalFloatStateSnapshot::WriteConfig(
              Runtime::snapshot, 0x9000u, writer) == 0x7777u &&
              writer.strings_written ==
                  (std::vector<std::pair<std::string, std::string>>{
                      {"valName", "Raft"}}) &&
              writer.ints_written ==
                  (std::vector<std::pair<std::string, std::int32_t>>{
                      {"valVisible", -1}, {"researchNum", 9}}) &&
              writer.floats_written ==
                  (std::vector<std::pair<std::string, float>>{
                      {"valPosX", 1.0f}, {"valPosY", 2.0f},
                      {"valPosZ", 3.0f}, {"valRotX", 4.0f},
                      {"valRotY", 5.0f}, {"valRotZ", 6.0f}}) &&
              writer.calls.back() == "destroy XML document",
          "local float snapshot XML writer exact fields and order");
    Runtime reader;
    reader.xml_ints = {{"valVisible", -2}, {"researchNum", 7}};
    reader.xml_floats = {{"valPosX", 1.0f}, {"valPosY", 2.0f},
                         {"valPosZ", 3.0f}, {"valRotX", 4.0f},
                         {"valRotY", 5.0f}, {"valRotZ", 6.0f}};
    Check(pal4::GobLocalFloatStateSnapshot::ReadConfig(
              Runtime::snapshot, 0x9000u, reader) == 0x7777u &&
              reader.name == "Platform" &&
              reader.words.at(Runtime::snapshot + 0x14u) ==
                  static_cast<std::uint32_t>(-2) &&
              reader.words.at(Runtime::snapshot + 0x30u) == 7u &&
              reader.floats.at(Runtime::snapshot + 0x18u) == 1.0f &&
              reader.floats.at(Runtime::snapshot + 0x2cu) == 6.0f &&
              reader.calls.back() == "destroy XML document",
          "local float snapshot XML reader restores fields");
    Runtime defaults;
    Check(pal4::GobLocalFloatStateSnapshot::ReadConfig(
              Runtime::snapshot, 0x9000u, defaults) == 0x7777u &&
              defaults.words.at(Runtime::snapshot + 0x14u) == 0u &&
              defaults.words.at(Runtime::snapshot + 0x30u) == 0u &&
              defaults.floats.at(Runtime::snapshot + 0x18u) == 0.0f &&
              defaults.floats.at(Runtime::snapshot + 0x2cu) == 0.0f,
          "local float missing XML numeric attributes default to zero");
    Runtime invalid;
    invalid.xml_read_success = false;
    Check(pal4::GobLocalFloatStateSnapshot::ReadConfig(
              Runtime::snapshot, 0x9000u, invalid) == 0x7777u &&
              invalid.name == "Raft" && invalid.words.empty() &&
              invalid.floats.empty() &&
              invalid.calls == (std::vector<std::string>{
                  "create XML document", "read XML",
                  "destroy XML document"}),
          "invalid local float snapshot XML leaves fields unchanged");
}

}  // namespace

int main() {
    try {
        CheckLifecycle();
        CheckXml();
        std::cout << "gob_local_float_snapshot_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
