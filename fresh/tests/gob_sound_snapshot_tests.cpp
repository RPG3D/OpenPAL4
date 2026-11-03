#include "pal4/gob_sound.h"
#include "pal4/gob_sound_snapshot.h"

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

struct Runtime final : pal4::IGobSnapshotRuntime,
                       pal4::IGobSoundSnapshotCaptureRuntime {
    static constexpr std::uint32_t object = 0x1000u;
    static constexpr std::uint32_t snapshot = 0x2000u;
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> calls;
    std::uint32_t allocation = snapshot;
    std::string object_name = "Bell";
    std::string name;
    std::string xml_name;
    bool fail_name_init = false;
    bool xml_valid = true;

    std::uint32_t Allocate(std::uint32_t size) override {
        Check(size == 0x14u, "snapshot allocation size");
        calls.emplace_back("allocate");
        return allocation;
    }
    void Free(std::uint32_t address) override {
        Check(address == snapshot, "snapshot free address");
        calls.emplace_back("free");
    }
    void InitializeNameRecord(std::uint32_t record) override {
        Check(record == snapshot + 4u, "name init offset");
        calls.emplace_back("name init");
        if (fail_name_init) throw std::runtime_error("name init failed");
        name.clear();
    }
    std::uint32_t ReleaseNameRecord(std::uint32_t record) override {
        Check(record == snapshot + 4u, "name release offset");
        calls.emplace_back("name release");
        name.clear();
        return 0x55u;
    }
    std::string ReadNameRecord(std::uint32_t record) override {
        Check(record == snapshot + 4u, "name read offset");
        calls.emplace_back("name read");
        return name;
    }
    void AssignNameRecord(std::uint32_t record,
                          std::string_view value) override {
        Check(record == snapshot + 4u, "name assign offset");
        calls.emplace_back("name assign");
        name = value;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.emplace_back("read " + std::to_string(address));
        return words[address];
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
        calls.emplace_back("write " + std::to_string(address - snapshot));
    }
    std::string_view ReadCString(std::uint32_t address) override {
        Check(address == 0x3000u, "object name pointer");
        calls.emplace_back("read object name");
        return object_name;
    }
    pal4::IGobSnapshotRuntime& SnapshotRuntime() override { return *this; }
    std::uint32_t CreateXmlDocument(std::string_view root) override {
        Check(root == "soGOBSound", "XML root");
        calls.emplace_back("create XML");
        return 0x8000u;
    }
    void WriteXmlString(std::uint32_t document, std::string_view key,
                        std::string_view value) override {
        Check(document == 0x8000u && key == "valName", "XML write key");
        calls.emplace_back("write XML name");
        xml_name = value;
    }
    void WriteXmlInt(std::uint32_t, std::string_view,
                     std::int32_t) override {
        throw std::runtime_error("unexpected XML integer write");
    }
    void FlushXml(std::uint32_t document, std::uint32_t stream) override {
        Check(document == 0x8000u && stream == 0x9000u, "XML flush args");
        calls.emplace_back("flush XML");
    }
    bool ReadXml(std::uint32_t document, std::uint32_t stream) override {
        Check(document == 0x8000u && stream == 0x9000u, "XML read args");
        calls.emplace_back("read XML");
        return xml_valid;
    }
    std::string ReadXmlString(std::uint32_t document, std::string_view key,
                              std::string_view fallback) override {
        Check(document == 0x8000u && key == "valName", "XML read key");
        calls.emplace_back("read XML name");
        return xml_name.empty() ? std::string(fallback) : xml_name;
    }
    std::int32_t ReadXmlInt(std::uint32_t, std::string_view,
                            std::int32_t) override {
        throw std::runtime_error("unexpected XML integer read");
    }
    std::uint32_t DestroyXmlDocument(std::uint32_t document) override {
        Check(document == 0x8000u, "XML destroy args");
        calls.emplace_back("destroy XML");
        return 0x88u;
    }
};

void CheckLifecycle() {
    Runtime runtime;
    Check(pal4::GobSoundStateSnapshot::Create(runtime) == Runtime::snapshot &&
          runtime.words[Runtime::snapshot] ==
              pal4::GobSoundStateSnapshot::kVtable &&
          runtime.words[Runtime::snapshot + 0x10u] == 0u &&
          runtime.calls == (std::vector<std::string>{
              "allocate", "name init", "write 8", "write 12",
              "write 16", "write 0"}), "snapshot construction");
    runtime.calls.clear();
    Check(pal4::GobSoundStateSnapshot::Destruct(
              Runtime::snapshot, runtime) == 0x55u &&
          runtime.words[Runtime::snapshot] ==
              pal4::GobSoundStateSnapshot::kBaseVtable &&
          runtime.calls == (std::vector<std::string>{
              "write 0", "name release", "write 8", "write 12",
              "write 16", "write 0"}), "snapshot destruction");
    Runtime empty;
    empty.allocation = 0u;
    Check(pal4::GobSoundStateSnapshot::Create(empty) == 0u &&
          empty.calls == (std::vector<std::string>{"allocate"}),
          "snapshot allocation failure");
    Runtime failing;
    failing.fail_name_init = true;
    try {
        (void)pal4::GobSoundStateSnapshot::Create(failing);
        throw std::runtime_error("missing constructor failure");
    } catch (const std::runtime_error& error) {
        Check(std::string(error.what()) == "name init failed" &&
              failing.calls == (std::vector<std::string>{
                  "allocate", "name init", "free"}),
              "snapshot constructor cleanup");
    }
}

void CheckCapture() {
    Runtime disabled;
    Check(pal4::GobSound::CreateStateSnapshot(
              Runtime::object, disabled) == 0u &&
          disabled.calls.size() == 1u, "disabled store flag");
    for (const auto& value : {std::string{}, std::string{"Bell"},
                               std::string(80u, 's')}) {
        Runtime runtime;
        runtime.words[Runtime::object + 0x1a4u] = 1u;
        runtime.words[Runtime::object + 4u] = 0x3000u;
        runtime.object_name = value;
        Check(pal4::GobSound::CreateStateSnapshot(
                  Runtime::object, runtime) == Runtime::snapshot &&
              runtime.name == value &&
              runtime.calls.back() == "name assign",
              "snapshot captures object name");
    }
    Runtime null;
    null.words[Runtime::object + 0x1a4u] = 1u;
    null.allocation = 0u;
    Check(pal4::GobSound::CreateStateSnapshot(
              Runtime::object, null) == 0u &&
          null.calls.back() == "allocate", "snapshot capture allocation failure");
}

void CheckXml() {
    Runtime writer;
    writer.name = "Bell";
    Check(pal4::GobSoundStateSnapshot::WriteConfig(
              Runtime::snapshot, 0x9000u, writer) == 0x88u &&
          writer.xml_name == "Bell" &&
          writer.calls == (std::vector<std::string>{
              "create XML", "name read", "write XML name",
              "flush XML", "destroy XML"}), "name-only XML writer");
    Runtime reader;
    reader.xml_name = "Bell";
    Check(pal4::GobSoundStateSnapshot::ReadConfig(
              Runtime::snapshot, 0x9000u, reader) == 0x88u &&
          reader.name == "Bell" &&
          reader.calls == (std::vector<std::string>{
              "create XML", "read XML", "read XML name",
              "name assign", "destroy XML"}), "name-only XML reader");
    Runtime missing;
    missing.name = "Old";
    Check(pal4::GobSoundStateSnapshot::ReadConfig(
              Runtime::snapshot, 0x9000u, missing) == 0x88u &&
          missing.name.empty(), "missing name defaults empty");
    Runtime invalid;
    invalid.name = "Old";
    invalid.xml_valid = false;
    Check(pal4::GobSoundStateSnapshot::ReadConfig(
              Runtime::snapshot, 0x9000u, invalid) == 0x88u &&
          invalid.name == "Old" &&
          invalid.calls == (std::vector<std::string>{
              "create XML", "read XML", "destroy XML"}),
          "invalid XML preserves name");
}

}  // namespace

int main() {
    try {
        CheckLifecycle();
        CheckCapture();
        CheckXml();
        std::cout << "gob_sound_snapshot_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
