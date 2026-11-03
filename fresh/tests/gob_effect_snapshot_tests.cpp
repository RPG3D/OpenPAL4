#include "pal4/gob_effect_snapshot.h"

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

struct Runtime final : pal4::IGobSnapshotRuntime {
    static constexpr std::uint32_t snapshot = 0x2000u;
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> calls;
    std::vector<std::pair<std::string, std::string>> strings_written;
    std::vector<std::pair<std::string, std::int32_t>> ints_written;
    std::uint32_t allocation = snapshot;
    bool fail_name_init = false;
    bool xml_read_success = true;
    std::string name = "Torch";
    std::string xml_name = "Lantern";
    std::int32_t xml_visible = 1;

    std::uint32_t Allocate(std::uint32_t size) override {
        Check(size == pal4::GobEffectStateSnapshot::kSize,
              "effect snapshot allocation size");
        calls.emplace_back("allocate");
        return allocation;
    }
    void Free(std::uint32_t memory) override {
        Check(memory == snapshot, "effect snapshot free");
        calls.emplace_back("free");
    }
    void InitializeNameRecord(std::uint32_t record) override {
        Check(record == snapshot + 4u, "effect snapshot name init");
        calls.emplace_back("initialize name");
        if (fail_name_init) throw std::runtime_error("name init failed");
    }
    std::uint32_t ReleaseNameRecord(std::uint32_t record) override {
        Check(record == snapshot + 4u, "effect snapshot name release");
        calls.emplace_back("release name");
        return 0x4444u;
    }
    std::string ReadNameRecord(std::uint32_t record) override {
        Check(record == snapshot + 4u, "effect snapshot name read");
        calls.emplace_back("read name");
        return name;
    }
    void AssignNameRecord(std::uint32_t record,
                          std::string_view value) override {
        Check(record == snapshot + 4u, "effect snapshot name assign");
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
        Check(root == "soGOBEffect", "effect snapshot XML root");
        calls.emplace_back("create XML document");
        return 0x8000u;
    }
    void WriteXmlString(std::uint32_t document,
                        std::string_view key,
                        std::string_view value) override {
        Check(document == 0x8000u, "effect snapshot XML string doc");
        strings_written.emplace_back(key, value);
        calls.emplace_back("write XML string");
    }
    void WriteXmlInt(std::uint32_t document,
                     std::string_view key,
                     std::int32_t value) override {
        Check(document == 0x8000u, "effect snapshot XML int doc");
        ints_written.emplace_back(key, value);
        calls.emplace_back("write XML int");
    }
    void FlushXml(std::uint32_t document,
                  std::uint32_t stream) override {
        Check(document == 0x8000u && stream == 0x9000u,
              "effect snapshot XML output");
        calls.emplace_back("flush XML");
    }
    bool ReadXml(std::uint32_t document,
                 std::uint32_t stream) override {
        Check(document == 0x8000u && stream == 0x9000u,
              "effect snapshot XML input");
        calls.emplace_back("read XML");
        return xml_read_success;
    }
    std::string ReadXmlString(std::uint32_t document,
                              std::string_view key,
                              std::string_view fallback) override {
        Check(document == 0x8000u && key == "valName" &&
                  fallback.empty(), "effect snapshot XML name key");
        calls.emplace_back("read XML string");
        return xml_name;
    }
    std::int32_t ReadXmlInt(std::uint32_t document,
                            std::string_view key,
                            std::int32_t fallback) override {
        Check(document == 0x8000u && key == "valVisible" &&
                  fallback == 0, "effect snapshot XML visible key");
        calls.emplace_back("read XML int");
        return xml_visible;
    }
    std::uint32_t DestroyXmlDocument(std::uint32_t document) override {
        Check(document == 0x8000u, "effect snapshot XML cleanup");
        calls.emplace_back("destroy XML document");
        return 0x7777u;
    }
};

void CheckLifecycle() {
    Check(pal4::GobEffectStateSnapshot::TypeKey() == 8u,
          "effect snapshot type thunk");
    Runtime unavailable;
    unavailable.allocation = 0u;
    Check(pal4::GobEffectStateSnapshot::Create(unavailable) == 0u &&
              unavailable.calls == (std::vector<std::string>{"allocate"}),
          "effect snapshot allocation failure");
    Runtime created;
    Check(pal4::GobEffectStateSnapshot::Create(created) ==
              Runtime::snapshot &&
              created.words.at(Runtime::snapshot) ==
                  pal4::GobEffectStateSnapshot::kVtable &&
              created.words.at(Runtime::snapshot + 8u) == 0u &&
              created.words.at(Runtime::snapshot + 0xcu) == 0u &&
              created.words.at(Runtime::snapshot + 0x10u) == 0u &&
              created.calls[0] == "allocate" &&
              created.calls[1] == "initialize name",
          "effect snapshot construction layout");
    created.calls.clear();
    Check(pal4::GobEffectStateSnapshot::Destruct(
              Runtime::snapshot, created) == 0x4444u &&
              created.words.at(Runtime::snapshot) ==
                  pal4::GobEffectStateSnapshot::kBaseVtable &&
              created.calls == (std::vector<std::string>{
                  "write word", "release name", "write word",
                  "write word", "write word", "write word"}),
          "effect snapshot destructor order");
    Runtime failed;
    failed.fail_name_init = true;
    bool thrown = false;
    try {
        (void)pal4::GobEffectStateSnapshot::Create(failed);
    } catch (const std::runtime_error&) {
        thrown = true;
    }
    Check(thrown && failed.calls == (std::vector<std::string>{
              "allocate", "initialize name", "free"}),
          "effect snapshot construction exception cleanup");
}

void CheckXml() {
    Runtime writer;
    writer.words[Runtime::snapshot + 0x14u] = 0xffffffffu;
    Check(pal4::GobEffectStateSnapshot::WriteConfig(
              Runtime::snapshot, 0x9000u, writer) == 0x7777u &&
              writer.strings_written ==
                  (std::vector<std::pair<std::string, std::string>>{
                      {"valName", "Torch"}}) &&
              writer.ints_written ==
                  (std::vector<std::pair<std::string, std::int32_t>>{
                      {"valVisible", -1}}) &&
              writer.calls == (std::vector<std::string>{
                  "create XML document", "read name", "write XML string",
                  "write XML int", "flush XML", "destroy XML document"}),
          "effect snapshot XML writer fields and order");
    Runtime reader;
    Check(pal4::GobEffectStateSnapshot::ReadConfig(
              Runtime::snapshot, 0x9000u, reader) == 0x7777u &&
              reader.name == "Lantern" &&
              reader.words.at(Runtime::snapshot + 0x14u) == 1u &&
              reader.calls.back() == "destroy XML document",
          "effect snapshot XML reader");
    Runtime invalid;
    invalid.xml_read_success = false;
    Check(pal4::GobEffectStateSnapshot::ReadConfig(
              Runtime::snapshot, 0x9000u, invalid) == 0x7777u &&
              invalid.name == "Torch" && invalid.words.empty() &&
              invalid.calls == (std::vector<std::string>{
                  "create XML document", "read XML",
                  "destroy XML document"}),
          "invalid effect snapshot XML leaves fields unchanged");
    Runtime defaulted;
    defaulted.xml_visible = 0;
    Check(pal4::GobEffectStateSnapshot::ReadConfig(
              Runtime::snapshot, 0x9000u, defaulted) == 0x7777u &&
              defaulted.words.at(Runtime::snapshot + 0x14u) == 0u,
          "missing effect visible value defaults to zero");
}

}  // namespace

int main() {
    try {
        CheckLifecycle();
        CheckXml();
        std::cout << "gob_effect_snapshot_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
