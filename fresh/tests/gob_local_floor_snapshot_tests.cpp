#include "pal4/gob_local_floor_snapshot.h"

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

struct Runtime final : pal4::IGobSnapshotRuntime {
    static constexpr std::uint32_t snapshot = 0x2000u;
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> calls;
    std::vector<std::pair<std::string, std::string>> strings;
    std::vector<std::pair<std::string, std::int32_t>> integers;
    std::uint32_t allocation = snapshot;
    bool fail_name = false;
    bool xml_valid = true;
    bool visible_present = true;
    std::string name = "Floor";

    std::uint32_t Allocate(std::uint32_t size) override {
        Check(size == 0x18u, "snapshot allocation size");
        calls.emplace_back("allocate");
        return allocation;
    }
    void Free(std::uint32_t address) override {
        Check(address == snapshot, "snapshot freed address");
        calls.emplace_back("free");
    }
    void InitializeNameRecord(std::uint32_t record) override {
        Check(record == snapshot + 4u, "name initialization address");
        calls.emplace_back("initialize name");
        if (fail_name) throw std::runtime_error("name init failed");
    }
    std::uint32_t ReleaseNameRecord(std::uint32_t record) override {
        Check(record == snapshot + 4u, "name release address");
        calls.emplace_back("release name");
        return 0x44u;
    }
    std::string ReadNameRecord(std::uint32_t record) override {
        Check(record == snapshot + 4u, "name read address");
        calls.emplace_back("read name");
        return name;
    }
    void AssignNameRecord(std::uint32_t record,
                          std::string_view value) override {
        Check(record == snapshot + 4u, "name assignment address");
        calls.emplace_back("assign name");
        name = value;
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
        Check(root == "soGOBLocalFloor", "XML root");
        calls.emplace_back("create XML");
        return 0x8000u;
    }
    void WriteXmlString(std::uint32_t document,
                        std::string_view key,
                        std::string_view value) override {
        Check(document == 0x8000u, "XML string document");
        strings.emplace_back(key, value);
        calls.emplace_back("write string");
    }
    void WriteXmlInt(std::uint32_t document,
                     std::string_view key,
                     std::int32_t value) override {
        Check(document == 0x8000u, "XML integer document");
        integers.emplace_back(key, value);
        calls.emplace_back("write int");
    }
    void FlushXml(std::uint32_t document,
                  std::uint32_t stream) override {
        Check(document == 0x8000u && stream == 0x9000u,
              "XML output arguments");
        calls.emplace_back("flush XML");
    }
    bool ReadXml(std::uint32_t document,
                 std::uint32_t stream) override {
        Check(document == 0x8000u && stream == 0x9000u,
              "XML input arguments");
        calls.emplace_back("read XML");
        return xml_valid;
    }
    std::string ReadXmlString(std::uint32_t document,
                              std::string_view key,
                              std::string_view fallback) override {
        Check(document == 0x8000u && key == "valName" &&
              fallback.empty(), "XML string field");
        calls.emplace_back("read string");
        return "LoadedFloor";
    }
    std::int32_t ReadXmlInt(std::uint32_t document,
                            std::string_view key,
                            std::int32_t fallback) override {
        Check(document == 0x8000u && key == "valVisible" &&
              fallback == 0, "XML integer field and fallback");
        calls.emplace_back("read int");
        return visible_present ? -2 : fallback;
    }
    std::uint32_t DestroyXmlDocument(
        std::uint32_t document) override {
        Check(document == 0x8000u, "XML destroy document");
        calls.emplace_back("destroy XML");
        return 0x77u;
    }
};

void CheckLifetime() {
    Runtime unavailable;
    unavailable.allocation = 0u;
    Check(pal4::GobLocalFloorStateSnapshot::Create(unavailable) == 0u &&
          unavailable.calls == (std::vector<std::string>{"allocate"}),
          "null allocation");
    Runtime created;
    Check(pal4::GobLocalFloorStateSnapshot::Create(created) ==
              Runtime::snapshot &&
          created.words.at(Runtime::snapshot) == 0x848f38u &&
          created.words.at(Runtime::snapshot + 8u) == 0u &&
          created.words.at(Runtime::snapshot + 0xcu) == 0u &&
          created.words.at(Runtime::snapshot + 0x10u) == 0u &&
          created.calls == (std::vector<std::string>{
              "allocate", "initialize name", "write word",
              "write word", "write word", "write word"}),
          "snapshot constructor layout");
    created.calls.clear();
    Check(pal4::GobLocalFloorStateSnapshot::Destruct(
              Runtime::snapshot, created) == 0x44u &&
          created.words.at(Runtime::snapshot) == 0x841e40u &&
          created.calls == (std::vector<std::string>{
              "write word", "release name", "write word",
              "write word", "write word", "write word"}),
          "snapshot destructor order");
    Runtime failed;
    failed.fail_name = true;
    bool thrown = false;
    try { (void)pal4::GobLocalFloorStateSnapshot::Create(failed); }
    catch (const std::runtime_error&) { thrown = true; }
    Check(thrown && failed.calls == (std::vector<std::string>{
              "allocate", "initialize name", "free"}),
          "failed name construction frees snapshot");
}

void CheckXml() {
    Runtime written;
    written.words[Runtime::snapshot + 0x14u] = 0xffffffffu;
    Check(pal4::GobLocalFloorStateSnapshot::WriteConfig(
              Runtime::snapshot, 0x9000u, written) == 0x77u &&
          written.strings == (std::vector<std::pair<std::string,
              std::string>>{{"valName", "Floor"}}) &&
          written.integers == (std::vector<std::pair<std::string,
              std::int32_t>>{{"valVisible", -1}}) &&
          written.calls == (std::vector<std::string>{
              "create XML", "read name", "write string", "write int",
              "flush XML", "destroy XML"}),
          "XML output fields, signed integer and order");
    Runtime read;
    Check(pal4::GobLocalFloorStateSnapshot::ReadConfig(
              Runtime::snapshot, 0x9000u, read) == 0x77u &&
          read.name == "LoadedFloor" &&
          read.words.at(Runtime::snapshot + 0x14u) == 0xfffffffeu &&
          read.calls == (std::vector<std::string>{
              "create XML", "read XML", "read string", "assign name",
              "read int", "write word", "destroy XML"}),
          "XML input fields and order");
    Runtime missing;
    missing.visible_present = false;
    Check(pal4::GobLocalFloorStateSnapshot::ReadConfig(
              Runtime::snapshot, 0x9000u, missing) == 0x77u &&
          missing.words.at(Runtime::snapshot + 0x14u) == 0u,
          "missing visibility defaults to zero");
    Runtime invalid;
    invalid.xml_valid = false;
    Check(pal4::GobLocalFloorStateSnapshot::ReadConfig(
              Runtime::snapshot, 0x9000u, invalid) == 0x77u &&
          invalid.name == "Floor" && invalid.words.empty() &&
          invalid.calls == (std::vector<std::string>{
              "create XML", "read XML", "destroy XML"}),
          "invalid XML leaves snapshot unchanged");
}

}  // namespace

int main() {
    try {
        CheckLifetime();
        CheckXml();
        std::cout << "gob_local_floor_snapshot_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
