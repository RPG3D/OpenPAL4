#include "pal4/gob_machine.h"
#include "pal4/gob_machine_snapshot.h"

#include <algorithm>
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

struct Runtime final : pal4::IGobMachineSnapshotRuntime {
    static constexpr std::uint32_t object = 0x1000u;
    static constexpr std::uint32_t snapshot = 0x2000u;
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::string> strings;
    std::vector<std::string> calls;
    std::map<std::uint32_t, float> floats;
    std::map<std::string, std::string> xml_strings;
    std::map<std::string, std::int32_t> xml_integers;
    std::map<std::string, float> xml_floats;
    std::vector<std::string> xml_keys;
    bool xml_valid = true;
    std::uint32_t allocation = snapshot;
    std::uint32_t type = 5u;
    std::uint32_t clear_result = 0x77u;

    std::uint32_t Allocate(std::uint32_t size) override {
        Check(size == 0x50u, "snapshot size");
        calls.emplace_back("allocate");
        return allocation;
    }
    void Free(std::uint32_t address) override {
        Check(address == snapshot, "free snapshot");
        calls.emplace_back("free");
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
        calls.push_back("write " + std::to_string(address));
    }
    void InitializeStringRecord(std::uint32_t record) override {
        Check(record == snapshot + 4u || record == snapshot + 0x40u,
              "string initialize address");
        strings[record].clear();
        calls.push_back("init " + std::to_string(record));
    }
    std::uint32_t ReleaseStringRecord(std::uint32_t record) override {
        Check(record == snapshot + 4u || record == snapshot + 0x40u,
              "string release address");
        calls.push_back("release " + std::to_string(record));
        strings.erase(record);
        return record == snapshot + 4u ? 0x55u : 0x44u;
    }
    std::string ReadCString(std::uint32_t address) override {
        Check(address == 0x5000u, "object name pointer");
        return "Machine";
    }
    std::string ReadStringRecord(std::uint32_t record) override {
        return strings[record];
    }
    void AssignStringRecord(std::uint32_t record,
                            std::string_view value) override {
        strings[record] = value;
        calls.push_back("assign " + std::to_string(record));
    }
    std::uint32_t SnapshotType(std::uint32_t address) override {
        Check(address == snapshot, "type snapshot");
        calls.emplace_back("type");
        return type;
    }
    void SetVisible(std::uint32_t address,
                    std::uint32_t visible) override {
        Check(address == object, "visible object");
        words[object + 0x184u] = visible;
        calls.emplace_back("visible");
    }
    void SetPosition(std::uint32_t address, std::uint32_t vector,
                     std::uint32_t mode) override {
        Check(address == object && vector == snapshot + 0x1cu && mode == 0u,
              "position arguments");
        calls.emplace_back("position");
    }
    void SetRotation(std::uint32_t address, std::uint32_t vector,
                     std::uint32_t mode) override {
        Check(address == object && vector == snapshot + 0x28u && mode == 0u,
              "rotation arguments");
        calls.emplace_back("rotation");
    }
    void SelectAnimation(std::uint32_t state,
                         std::string_view name) override {
        Check(state == object + 0x1dcu && name == "Open",
              "animation selection");
        calls.emplace_back("select animation");
    }
    float AnimationLength(std::uint32_t state) override {
        Check(state == object + 0x1dcu, "animation length state");
        calls.emplace_back("animation length");
        return 2.5f;
    }
    void SetAnimationTime(std::uint32_t state,
                          float time) override {
        Check(state == object + 0x1dcu && time == 2.5f,
              "animation time");
        calls.emplace_back("animation time");
    }
    std::uint32_t ClearAnimationFlag(std::uint32_t state,
                                     std::uint32_t flag) override {
        Check(state == object + 0x1dcu && flag == 1u,
              "animation flag");
        calls.emplace_back("clear animation flag");
        return clear_result;
    }
    float ReadFloat(std::uint32_t address) override {
        return floats[address];
    }
    void WriteFloat(std::uint32_t address, float value) override {
        floats[address] = value;
        calls.emplace_back("write float");
    }
    std::uint32_t CreateXmlDocument(std::string_view root) override {
        Check(root == "soGOBMachine", "snapshot XML root");
        calls.emplace_back("create XML");
        return 0x8000u;
    }
    void WriteXmlString(std::uint32_t document, std::string_view key,
                        std::string_view value) override {
        Check(document == 0x8000u, "XML string document");
        xml_keys.emplace_back(key);
        xml_strings[std::string(key)] = value;
    }
    void WriteXmlInt(std::uint32_t document, std::string_view key,
                     std::int32_t value) override {
        Check(document == 0x8000u, "XML int document");
        xml_keys.emplace_back(key);
        xml_integers[std::string(key)] = value;
    }
    void WriteXmlFloat(std::uint32_t document, std::string_view key,
                       float value) override {
        Check(document == 0x8000u, "XML float document");
        xml_keys.emplace_back(key);
        xml_floats[std::string(key)] = value;
    }
    void FlushXml(std::uint32_t document,
                  std::uint32_t stream) override {
        Check(document == 0x8000u && stream == 0x9000u,
              "flush XML arguments");
        calls.emplace_back("flush XML");
    }
    bool ReadXml(std::uint32_t document,
                 std::uint32_t stream) override {
        Check(document == 0x8000u && stream == 0x9000u,
              "read XML arguments");
        calls.emplace_back("read XML");
        return xml_valid;
    }
    std::string ReadXmlString(std::uint32_t document,
                              std::string_view key,
                              std::string_view fallback) override {
        Check(document == 0x8000u, "read XML string document");
        const auto found = xml_strings.find(std::string(key));
        return found == xml_strings.end() ? std::string(fallback) :
               found->second;
    }
    std::int32_t ReadXmlInt(std::uint32_t document,
                            std::string_view key,
                            std::int32_t fallback) override {
        Check(document == 0x8000u, "read XML int document");
        const auto found = xml_integers.find(std::string(key));
        return found == xml_integers.end() ? fallback : found->second;
    }
    float ReadXmlFloat(std::uint32_t document,
                       std::string_view key,
                       float fallback) override {
        Check(document == 0x8000u, "read XML float document");
        const auto found = xml_floats.find(std::string(key));
        return found == xml_floats.end() ? fallback : found->second;
    }
    std::uint32_t DestroyXmlDocument(std::uint32_t document) override {
        Check(document == 0x8000u, "destroy XML document");
        calls.emplace_back("destroy XML");
        return 0x88u;
    }
};

std::size_t IndexOf(const std::vector<std::string>& calls,
                    const std::string& value) {
    const auto found = std::find(calls.begin(), calls.end(), value);
    Check(found != calls.end(), "missing expected call");
    return static_cast<std::size_t>(found - calls.begin());
}

void PrepareMachine(Runtime& runtime) {
    runtime.words[Runtime::object + 0x1a4u] = 1u;
    runtime.words[Runtime::object + 4u] = 0x5000u;
    runtime.words[Runtime::object + 0x184u] = 3u;
    runtime.words[Runtime::object + 0x198u] = 7u;
    for (std::uint32_t index = 0; index < 3u; ++index) {
        runtime.words[Runtime::object + 0xacu + index * 4u] = 10u + index;
        runtime.words[Runtime::object + 0xa0u + index * 4u] = 20u + index;
    }
    runtime.words[Runtime::object + 0x2b4u] = 1u;
    runtime.words[Runtime::object + 0x2b8u] = 1u;
    runtime.strings[Runtime::object + 0x2bcu] = "Open";
}

void CheckLifecycle() {
    Runtime runtime;
    Check(pal4::GobMachineStateSnapshot::Create(runtime) ==
              Runtime::snapshot &&
          runtime.words[Runtime::snapshot] ==
              pal4::GobMachineStateSnapshot::kVtable &&
          runtime.words[Runtime::snapshot + 0x4cu] == 0u &&
          IndexOf(runtime.calls, "init 8196") <
              IndexOf(runtime.calls, "init 8256"),
          "snapshot constructor order and layout");
    runtime.calls.clear();
    Check(pal4::GobMachineStateSnapshot::Destruct(
              Runtime::snapshot, runtime) == 0x55u &&
          runtime.words[Runtime::snapshot] ==
              pal4::GobMachineStateSnapshot::kBaseVtable &&
          IndexOf(runtime.calls, "release 8256") <
              IndexOf(runtime.calls, "release 8196"),
          "snapshot destructor reverse string ownership");
    Runtime failed;
    failed.allocation = 0u;
    Check(pal4::GobMachineStateSnapshot::Create(failed) == 0u &&
          failed.calls == (std::vector<std::string>{"allocate"}),
          "allocation failure");
}

void CheckCapture() {
    Runtime disabled;
    Check(pal4::GobMachine::CreateStateSnapshot(
              Runtime::object, disabled) == 0u && disabled.calls.empty(),
          "disabled snapshot gate");
    Runtime runtime;
    PrepareMachine(runtime);
    Check(pal4::GobMachine::CreateStateSnapshot(
              Runtime::object, runtime) == Runtime::snapshot &&
          runtime.strings[Runtime::snapshot + 4u] == "Machine" &&
          runtime.strings[Runtime::snapshot + 0x40u] == "Open" &&
          runtime.words[Runtime::snapshot + 0x14u] == 3u &&
          runtime.words[Runtime::snapshot + 0x18u] == 7u &&
          runtime.words[Runtime::snapshot + 0x34u] == 1u &&
          runtime.words[Runtime::snapshot + 0x38u] == 1u &&
          runtime.words[Runtime::snapshot + 0x3cu] == 1u,
          "captured name, flags and counts");
    for (std::uint32_t index = 0; index < 3u; ++index) {
        Check(runtime.words[Runtime::snapshot + 0x1cu + index * 4u] ==
                  10u + index, "captured position");
        Check(runtime.words[Runtime::snapshot + 0x28u + index * 4u] ==
                  20u + index, "captured rotation");
    }
    runtime.words[Runtime::object + 0x2b8u] = 0u;
    Check(pal4::GobMachine::CreateStateSnapshot(
              Runtime::object, runtime) == Runtime::snapshot &&
          runtime.words[Runtime::snapshot + 0x38u] == 0u,
          "active action requires both flags");
}

void CheckApply() {
    Runtime disabled;
    Check(pal4::GobMachine::ApplyStateSnapshotEvent(
              Runtime::object, Runtime::snapshot, disabled) == 0u &&
          disabled.calls.empty(), "disabled event gate");
    Runtime wrong;
    wrong.words[Runtime::object + 0x1a4u] = 1u;
    wrong.type = 4u;
    Check(pal4::GobMachine::ApplyStateSnapshotEvent(
              Runtime::object, Runtime::snapshot, wrong) == 4u &&
          wrong.calls == (std::vector<std::string>{"type"}),
          "wrong snapshot type leaves object unchanged");
    Runtime runtime;
    runtime.words[Runtime::object + 0x1a4u] = 1u;
    runtime.words[Runtime::snapshot + 0x14u] = 4u;
    runtime.words[Runtime::snapshot + 0x18u] = 5u;
    runtime.words[Runtime::snapshot + 0x34u] = 6u;
    runtime.words[Runtime::snapshot + 0x3cu] = 7u;
    runtime.strings[Runtime::snapshot + 0x40u] = "Open";
    Check(pal4::GobMachine::ApplyStateSnapshotEvent(
              Runtime::object, Runtime::snapshot, runtime) == 0u &&
          runtime.words[Runtime::object + 0x184u] == 4u &&
          runtime.words[Runtime::object + 0x198u] == 5u &&
          runtime.words[Runtime::object + 0x2b4u] == 6u &&
          runtime.words[Runtime::object + 0x2b8u] == 7u &&
          runtime.strings[Runtime::object + 0x2bcu] == "Open" &&
          IndexOf(runtime.calls, "visible") <
              IndexOf(runtime.calls, "position") &&
          IndexOf(runtime.calls, "position") <
              IndexOf(runtime.calls, "rotation") &&
          !std::count(runtime.calls.begin(), runtime.calls.end(),
                      "select animation"),
          "snapshot fields apply without inactive animation");
    runtime.calls.clear();
    runtime.words[Runtime::snapshot + 0x38u] = 1u;
    Check(pal4::GobMachine::ApplyStateSnapshotEvent(
              Runtime::object, Runtime::snapshot, runtime) == 0x77u &&
          IndexOf(runtime.calls, "select animation") <
              IndexOf(runtime.calls, "animation length") &&
          IndexOf(runtime.calls, "animation length") <
              IndexOf(runtime.calls, "animation time") &&
          runtime.calls.back() == "clear animation flag",
          "active snapshot seeks action animation to end");
}

void CheckXmlConfig() {
    Runtime written;
    written.strings[Runtime::snapshot + 4u] = "Machine";
    written.strings[Runtime::snapshot + 0x40u] = "Open";
    written.words[Runtime::snapshot + 0x14u] = 1u;
    written.words[Runtime::snapshot + 0x18u] = 3u;
    written.words[Runtime::snapshot + 0x34u] = 4u;
    written.words[Runtime::snapshot + 0x38u] = 5u;
    written.words[Runtime::snapshot + 0x3cu] = 6u;
    for (std::uint32_t index = 0; index < 6u; ++index)
        written.floats[Runtime::snapshot + 0x1cu + index * 4u] =
            static_cast<float>(index) + 0.25f;
    const std::vector<std::string> keys{
        "valName", "valVisible", "researchNum",
        "valPosX", "valPosY", "valPosZ",
        "valRotX", "valRotY", "valRotZ",
        "valHasTouched", "valNeedSetAction", "valHasDoAction",
        "valActionName"};
    Check(pal4::GobMachineStateSnapshot::WriteConfig(
              Runtime::snapshot, 0x9000u, written) == 0x88u &&
          written.xml_keys == keys &&
          written.xml_strings["valName"] == "Machine" &&
          written.xml_strings["valActionName"] == "Open" &&
          written.xml_integers["researchNum"] == 3 &&
          written.xml_integers["valHasDoAction"] == 6 &&
          written.xml_floats["valPosX"] == 0.25f &&
          written.xml_floats["valRotZ"] == 5.25f &&
          written.calls.back() == "destroy XML",
          "snapshot XML output field order and values");

    Runtime read;
    read.xml_strings = written.xml_strings;
    read.xml_integers = written.xml_integers;
    read.xml_floats = written.xml_floats;
    Check(pal4::GobMachineStateSnapshot::ReadConfig(
              Runtime::snapshot, 0x9000u, read) == 0x88u &&
          read.strings[Runtime::snapshot + 4u] == "Machine" &&
          read.strings[Runtime::snapshot + 0x40u] == "Open" &&
          read.words[Runtime::snapshot + 0x18u] == 3u &&
          read.words[Runtime::snapshot + 0x3cu] == 6u &&
          read.floats[Runtime::snapshot + 0x1cu] == 0.25f &&
          read.floats[Runtime::snapshot + 0x30u] == 5.25f,
          "snapshot XML round trip");
    Runtime defaults;
    defaults.words[Runtime::snapshot + 0x14u] = 9u;
    defaults.floats[Runtime::snapshot + 0x1cu] = 3.0f;
    Check(pal4::GobMachineStateSnapshot::ReadConfig(
              Runtime::snapshot, 0x9000u, defaults) == 0x88u &&
          defaults.words[Runtime::snapshot + 0x14u] == 0u &&
          defaults.floats[Runtime::snapshot + 0x1cu] == 0.0f &&
          defaults.strings[Runtime::snapshot + 4u].empty() &&
          defaults.strings[Runtime::snapshot + 0x40u].empty(),
          "missing XML values default to zero or empty");
    Runtime invalid;
    invalid.xml_valid = false;
    invalid.words[Runtime::snapshot + 0x14u] = 9u;
    Check(pal4::GobMachineStateSnapshot::ReadConfig(
              Runtime::snapshot, 0x9000u, invalid) == 0x88u &&
          invalid.words[Runtime::snapshot + 0x14u] == 9u &&
          invalid.calls == (std::vector<std::string>{
              "create XML", "read XML", "destroy XML"}),
          "invalid snapshot XML leaves state untouched");
}

}  // namespace

int main() {
    try {
        CheckLifecycle();
        CheckCapture();
        CheckApply();
        CheckXmlConfig();
        std::cout << "gob_machine_snapshot_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
