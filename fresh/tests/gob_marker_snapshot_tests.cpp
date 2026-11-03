#include "pal4/gob_marker_snapshot.h"
#include "gob_marker_test_runtime.h"

#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

void CheckLifecycle() {
    MarkerSnapshotRuntime runtime;
    Check(pal4::GobMarkerStateSnapshot::Create(runtime) ==
              MarkerSnapshotRuntime::snapshot &&
          runtime.words[MarkerSnapshotRuntime::snapshot] ==
              pal4::GobMarkerStateSnapshot::kVtable &&
          runtime.words[MarkerSnapshotRuntime::snapshot + 8u] == 0u &&
          runtime.words[MarkerSnapshotRuntime::snapshot + 0x10u] == 0u &&
          runtime.calls.front() == "allocate" &&
          runtime.calls[1] == "name init",
          "marker snapshot constructor layout");
    runtime.calls.clear();
    Check(pal4::GobMarkerStateSnapshot::Destruct(
              MarkerSnapshotRuntime::snapshot, runtime) == 0x55u &&
          runtime.words[MarkerSnapshotRuntime::snapshot] ==
              pal4::GobMarkerStateSnapshot::kBaseVtable &&
          runtime.calls[1] == "name release",
          "marker snapshot destructor and vtable");
    MarkerSnapshotRuntime null;
    null.allocation = 0u;
    Check(pal4::GobMarkerStateSnapshot::Create(null) == 0u &&
          null.calls == (std::vector<std::string>{"allocate"}),
          "snapshot null allocation");
    MarkerSnapshotRuntime throwing;
    throwing.fail_name_init = true;
    try {
        (void)pal4::GobMarkerStateSnapshot::Create(throwing);
        throw std::runtime_error("missing constructor exception");
    } catch (const std::runtime_error& error) {
        Check(std::string(error.what()) == "name init failed" &&
              throwing.calls == (std::vector<std::string>{
                  "allocate", "name init", "free"}),
              "snapshot constructor exception releases allocation");
    }
}

void CheckXml() {
    MarkerSnapshotRuntime written;
    written.name = "WayPoint";
    Check(pal4::GobMarkerStateSnapshot::WriteConfig(
              MarkerSnapshotRuntime::snapshot, 0x9000u, written) == 0x88u &&
          written.xml_name == "WayPoint" &&
          written.calls == (std::vector<std::string>{
              "create XML", "name read", "write XML name",
              "flush XML", "destroy XML"}),
          "marker XML write root, name and order");
    MarkerSnapshotRuntime read;
    read.xml_name = "WayPoint";
    Check(pal4::GobMarkerStateSnapshot::ReadConfig(
              MarkerSnapshotRuntime::snapshot, 0x9000u, read) == 0x88u &&
          read.name == "WayPoint" &&
          read.calls == (std::vector<std::string>{
              "create XML", "read XML", "read XML name",
              "name assign", "destroy XML"}),
          "marker XML read name and order");
    MarkerSnapshotRuntime missing;
    missing.name = "Old";
    Check(pal4::GobMarkerStateSnapshot::ReadConfig(
              MarkerSnapshotRuntime::snapshot, 0x9000u, missing) == 0x88u &&
          missing.name.empty(), "missing name defaults empty");
    MarkerSnapshotRuntime invalid;
    invalid.name = "Old";
    invalid.xml_valid = false;
    Check(pal4::GobMarkerStateSnapshot::ReadConfig(
              MarkerSnapshotRuntime::snapshot, 0x9000u, invalid) == 0x88u &&
          invalid.name == "Old" &&
          invalid.calls == (std::vector<std::string>{
              "create XML", "read XML", "destroy XML"}),
          "invalid XML preserves snapshot");
}

}  // namespace

int main() {
    try {
        CheckLifecycle();
        CheckXml();
        std::cout << "gob_marker_snapshot_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
