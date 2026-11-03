#include "pal4/gob_obstacle_snapshot.h"
#include "gob_obstacle_snapshot_runtime.h"

#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

void CheckLifecycle() {
    const auto address = ObstacleSnapshotRuntime::snapshot;
    ObstacleSnapshotRuntime runtime;
    Check(pal4::GobObstacleStateSnapshot::Create(runtime) == address &&
          runtime.words[address] ==
              pal4::GobObstacleStateSnapshot::kVtable &&
          runtime.words[address + 8u] == 0u &&
          runtime.words[address + 0x10u] == 0u &&
          runtime.calls == (std::vector<std::string>{
              "allocate", "name init", "write 8", "write 12",
              "write 16", "write 0"}),
          "snapshot allocation and constructor layout");
    runtime.calls.clear();
    Check(pal4::GobObstacleStateSnapshot::Destruct(address, runtime) ==
              0x55u && runtime.words[address] ==
              pal4::GobObstacleStateSnapshot::kBaseVtable &&
          runtime.calls == (std::vector<std::string>{
              "write 0", "name release", "write 8", "write 12",
              "write 16", "write 0"}),
          "snapshot destructor release order and vtables");
    ObstacleSnapshotRuntime empty;
    empty.allocation = 0u;
    Check(pal4::GobObstacleStateSnapshot::Create(empty) == 0u &&
          empty.calls == (std::vector<std::string>{"allocate"}),
          "snapshot allocation failure");
    ObstacleSnapshotRuntime failed;
    failed.fail_name_init = true;
    try {
        (void)pal4::GobObstacleStateSnapshot::Create(failed);
        throw std::runtime_error("missing constructor failure");
    } catch (const std::runtime_error& error) {
        Check(std::string(error.what()) == "name init failed" &&
              failed.calls == (std::vector<std::string>{
                  "allocate", "name init", "free"}),
              "snapshot constructor exception cleanup");
    }
}

void CheckXml() {
    const auto address = ObstacleSnapshotRuntime::snapshot;
    ObstacleSnapshotRuntime writer;
    writer.name = "Barrier";
    Check(pal4::GobObstacleStateSnapshot::WriteConfig(
              address, 0x9000u, writer) == 0x88u &&
          writer.xml_name == "Barrier" &&
          writer.calls == (std::vector<std::string>{
              "create XML", "name read", "write XML name",
              "flush XML", "destroy XML"}),
          "name-only XML writer");
    ObstacleSnapshotRuntime reader;
    reader.xml_name = "Barrier";
    Check(pal4::GobObstacleStateSnapshot::ReadConfig(
              address, 0x9000u, reader) == 0x88u &&
          reader.name == "Barrier" &&
          reader.calls == (std::vector<std::string>{
              "create XML", "read XML", "read XML name",
              "name assign", "destroy XML"}),
          "name-only XML reader");
    ObstacleSnapshotRuntime missing;
    missing.name = "Old";
    Check(pal4::GobObstacleStateSnapshot::ReadConfig(
              address, 0x9000u, missing) == 0x88u &&
          missing.name.empty(), "missing name defaults to empty");
    ObstacleSnapshotRuntime invalid;
    invalid.name = "Old";
    invalid.xml_valid = false;
    Check(pal4::GobObstacleStateSnapshot::ReadConfig(
              address, 0x9000u, invalid) == 0x88u &&
          invalid.name == "Old" &&
          invalid.calls == (std::vector<std::string>{
              "create XML", "read XML", "destroy XML"}),
          "invalid XML preserves old snapshot name");
}

}  // namespace

int main() {
    try {
        CheckLifecycle();
        CheckXml();
        std::cout << "gob_obstacle_snapshot_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
