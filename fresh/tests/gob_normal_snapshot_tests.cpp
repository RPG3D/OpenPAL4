#include "pal4/gob_normal_snapshot.h"
#include "gob_normal_test_runtime.h"

#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

void CheckLifecycle() {
    NormalSnapshotRuntime runtime;
    const auto address = NormalSnapshotRuntime::snapshot;
    Check(pal4::GobNormalStateSnapshot::Create(runtime) == address &&
          runtime.words[address] == pal4::GobNormalStateSnapshot::kVtable &&
          runtime.words[address + 8u] == 0u &&
          runtime.words[address + 0x10u] == 0u &&
          runtime.calls.front() == "allocate" &&
          runtime.calls[1] == "name init", "snapshot constructor layout");
    runtime.calls.clear();
    Check(pal4::GobNormalStateSnapshot::Destruct(address, runtime) == 0x55u &&
          runtime.words[address] ==
              pal4::GobNormalStateSnapshot::kBaseVtable &&
          runtime.calls[1] == "name release", "snapshot destructor");
    NormalSnapshotRuntime null;
    null.allocation = 0;
    Check(pal4::GobNormalStateSnapshot::Create(null) == 0u &&
          null.calls == (std::vector<std::string>{"allocate"}),
          "snapshot null allocation");
    NormalSnapshotRuntime failing;
    failing.fail_name_init = true;
    try {
        (void)pal4::GobNormalStateSnapshot::Create(failing);
        throw std::runtime_error("missing name exception");
    } catch (const std::runtime_error& error) {
        Check(std::string(error.what()) == "name init failed" &&
              failing.calls == (std::vector<std::string>{
                  "allocate", "name init", "free"}),
              "snapshot constructor exception cleanup");
    }
}

void CheckXml() {
    const auto address = NormalSnapshotRuntime::snapshot;
    NormalSnapshotRuntime written;
    written.name = "Normal";
    written.words[address + 0x14u] = 3u;
    Check(pal4::GobNormalStateSnapshot::WriteConfig(
              address, 0x9000u, written) == 0x88u &&
          written.xml_name == "Normal" && written.xml_visible == 3 &&
          written.calls == (std::vector<std::string>{
              "create XML", "name read", "write XML name", "read word",
              "write XML visible", "flush XML", "destroy XML"}),
          "snapshot XML writer fields and order");
    NormalSnapshotRuntime read;
    read.xml_name = "Normal";
    read.xml_visible = -1;
    Check(pal4::GobNormalStateSnapshot::ReadConfig(
              address, 0x9000u, read) == 0x88u &&
          read.name == "Normal" && read.words[address + 0x14u] ==
              0xffffffffu && read.calls == (std::vector<std::string>{
                  "create XML", "read XML", "read XML name", "name assign",
                  "read XML visible", "write word", "destroy XML"}),
          "snapshot XML reader signed visible value and order");
    NormalSnapshotRuntime missing;
    missing.name = "Old";
    missing.words[address + 0x14u] = 5u;
    Check(pal4::GobNormalStateSnapshot::ReadConfig(
              address, 0x9000u, missing) == 0x88u &&
          missing.name.empty() && missing.words[address + 0x14u] == 0u,
          "missing XML fields default empty and zero");
    NormalSnapshotRuntime invalid;
    invalid.name = "Old";
    invalid.words[address + 0x14u] = 5u;
    invalid.xml_valid = false;
    Check(pal4::GobNormalStateSnapshot::ReadConfig(
              address, 0x9000u, invalid) == 0x88u &&
          invalid.name == "Old" && invalid.words[address + 0x14u] == 5u &&
          invalid.calls == (std::vector<std::string>{
              "create XML", "read XML", "destroy XML"}),
          "invalid XML preserves snapshot");
}

}  // namespace

int main() {
    try {
        CheckLifecycle();
        CheckXml();
        std::cout << "gob_normal_snapshot_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
