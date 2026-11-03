#include "pal4/gob_action_snapshot.h"

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IGobActionSnapshotRuntime {
    static constexpr std::uint32_t snapshot = 0x2000u;
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> calls;
    std::vector<std::pair<std::string, std::string>> written_strings;
    std::vector<std::pair<std::string, std::int32_t>> written_ints;
    std::uint32_t allocation = snapshot;
    bool fail_name_init = false;
    bool xml_read_success = true;
    std::string name = "JQB";
    std::string xml_name = "MC";
    std::map<std::string, std::int32_t> xml_ints;

    std::uint32_t Allocate(std::uint32_t size) override {
        Check(size == pal4::GobActionStateSnapshot::kSize,
              "snapshot allocation size");
        calls.emplace_back("allocate");
        return allocation;
    }
    void Free(std::uint32_t memory) override {
        Check(memory == snapshot, "snapshot freed memory");
        calls.emplace_back("free");
    }
    void InitializeNameRecord(std::uint32_t record) override {
        Check(record == snapshot + 4u, "snapshot name record init");
        calls.emplace_back("initialize name");
        if (fail_name_init) throw std::runtime_error("name init failed");
    }
    std::uint32_t ReleaseNameRecord(std::uint32_t record) override {
        Check(record == snapshot + 4u, "snapshot name record release");
        calls.emplace_back("release name");
        return 0x4444u;
    }
    std::string ReadNameRecord(std::uint32_t record) override {
        Check(record == snapshot + 4u, "snapshot name record read");
        calls.emplace_back("read name");
        return name;
    }
    void AssignNameRecord(std::uint32_t record,
                          std::string_view value) override {
        Check(record == snapshot + 4u, "snapshot name record assign");
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
    std::uint32_t CreateXmlDocument(std::string_view root) override {
        Check(root == "soGOBAction", "snapshot XML root");
        calls.emplace_back("create XML document");
        return 0x8000u;
    }
    void WriteXmlString(std::uint32_t document,
                        std::string_view key,
                        std::string_view value) override {
        Check(document == 0x8000u, "snapshot XML string document");
        written_strings.emplace_back(key, value);
        calls.emplace_back("write XML string");
    }
    void WriteXmlInt(std::uint32_t document,
                     std::string_view key,
                     std::int32_t value) override {
        Check(document == 0x8000u, "snapshot XML int document");
        written_ints.emplace_back(key, value);
        calls.emplace_back("write XML int");
    }
    void FlushXml(std::uint32_t document,
                  std::uint32_t stream) override {
        Check(document == 0x8000u && stream == 0x9000u,
              "snapshot XML output stream");
        calls.emplace_back("flush XML");
    }
    bool ReadXml(std::uint32_t document,
                 std::uint32_t stream) override {
        Check(document == 0x8000u && stream == 0x9000u,
              "snapshot XML input stream");
        calls.emplace_back("read XML");
        return xml_read_success;
    }
    std::string ReadXmlString(std::uint32_t document,
                              std::string_view key,
                              std::string_view fallback) override {
        Check(document == 0x8000u && key == "valName" &&
                  fallback.empty(),
              "snapshot XML name defaults");
        calls.emplace_back("read XML string");
        return xml_name;
    }
    std::int32_t ReadXmlInt(std::uint32_t document,
                            std::string_view key,
                            std::int32_t fallback) override {
        Check(document == 0x8000u && fallback == 0,
              "snapshot XML integer defaults");
        calls.emplace_back("read XML int");
        const auto found = xml_ints.find(std::string(key));
        return found == xml_ints.end() ? fallback : found->second;
    }
    std::uint32_t DestroyXmlDocument(
        std::uint32_t document) override {
        Check(document == 0x8000u, "snapshot XML cleanup document");
        calls.emplace_back("destroy XML document");
        return 0x7777u;
    }
};

void CheckLifecycle() {
    Check(pal4::GobActionStateSnapshot::TypeKey() == 6u,
          "snapshot type thunk");
    Runtime unavailable;
    unavailable.allocation = 0u;
    Check(pal4::GobActionStateSnapshot::Create(unavailable) == 0u &&
              unavailable.calls ==
                  (std::vector<std::string>{"allocate"}),
          "snapshot allocation failure");
    Runtime created;
    Check(pal4::GobActionStateSnapshot::Create(created) ==
              Runtime::snapshot &&
              created.words.at(Runtime::snapshot) ==
                  pal4::GobActionStateSnapshot::kVtable &&
              created.words.at(Runtime::snapshot + 8u) == 0u &&
              created.words.at(Runtime::snapshot + 0xcu) == 0u &&
              created.words.at(Runtime::snapshot + 0x10u) == 0u &&
              created.calls.front() == "allocate" &&
              created.calls[1] == "initialize name",
          "snapshot constructor layout");
    created.calls.clear();
    Check(pal4::GobActionStateSnapshot::Destruct(
              Runtime::snapshot, created) == 0x4444u &&
              created.words.at(Runtime::snapshot) ==
                  pal4::GobActionStateSnapshot::kBaseVtable &&
              created.calls ==
                  (std::vector<std::string>{
                      "write word", "release name", "write word",
                      "write word", "write word", "write word"}),
          "snapshot destructor vtables and name cleanup");
    Runtime failed;
    failed.fail_name_init = true;
    bool thrown = false;
    try {
        (void)pal4::GobActionStateSnapshot::Create(failed);
    } catch (const std::runtime_error&) {
        thrown = true;
    }
    Check(thrown && failed.calls ==
              (std::vector<std::string>{
                  "allocate", "initialize name", "free"}),
          "snapshot constructor exception frees allocation");
}

void CheckConfig() {
    Runtime writer;
    writer.words[Runtime::snapshot + 0x14u] = 1u;
    writer.words[Runtime::snapshot + 0x18u] = 0xffffffffu;
    Check(pal4::GobActionStateSnapshot::WriteConfig(
              Runtime::snapshot, 0x9000u, writer) == 0x7777u &&
              writer.written_strings ==
                  (std::vector<std::pair<std::string, std::string>>{
                      {"valName", "JQB"}}) &&
              writer.written_ints ==
                  (std::vector<std::pair<std::string, std::int32_t>>{
                      {"valVisible", 1}, {"playTimes", -1}}) &&
              writer.calls ==
                  (std::vector<std::string>{
                      "create XML document", "read name",
                      "write XML string", "write XML int",
                      "write XML int", "flush XML",
                      "destroy XML document"}),
          "snapshot XML writer fields and order");
    Runtime reader;
    reader.xml_ints = {{"valVisible", 7}, {"playTimes", -2}};
    Check(pal4::GobActionStateSnapshot::ReadConfig(
              Runtime::snapshot, 0x9000u, reader) == 0x7777u &&
              reader.name == "MC" &&
              reader.words.at(Runtime::snapshot + 0x14u) == 7u &&
              reader.words.at(Runtime::snapshot + 0x18u) ==
                  static_cast<std::uint32_t>(-2) &&
              reader.calls.front() == "create XML document" &&
              reader.calls.back() == "destroy XML document",
          "snapshot XML reader restores name and values");
    Runtime defaults;
    Check(pal4::GobActionStateSnapshot::ReadConfig(
              Runtime::snapshot, 0x9000u, defaults) == 0x7777u &&
              defaults.words.at(Runtime::snapshot + 0x14u) == 0u &&
              defaults.words.at(Runtime::snapshot + 0x18u) == 0u,
          "snapshot XML missing integer defaults");
    Runtime mismatch;
    mismatch.xml_read_success = false;
    Check(pal4::GobActionStateSnapshot::ReadConfig(
              Runtime::snapshot, 0x9000u, mismatch) == 0x7777u &&
              mismatch.name == "JQB" &&
              mismatch.words.empty() &&
              mismatch.calls ==
                  (std::vector<std::string>{
                      "create XML document", "read XML",
                      "destroy XML document"}),
          "snapshot invalid XML leaves state unchanged");
}

}  // namespace

int main() {
    try {
        CheckLifecycle();
        CheckConfig();
        std::cout << "gob_action_snapshot_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
