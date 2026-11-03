#include "pal4/equipment_level_formatter.h"
#include "pal4/equipment_window_actions.h"

#include <cstdlib>
#include <iostream>
#include <string>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

void TestBoundaries() {
    for (const auto level : {-100, -1, 0, 100, 101})
        Check(pal4::EquipmentLevelFormatter::Format(level) == "0",
              "outside 1-99 formats as zero");
    Check(pal4::EquipmentLevelFormatter::Format(1) == "1\n \n" &&
              pal4::EquipmentLevelFormatter::Format(9) == "9\n \n",
          "single-digit levels use ones-only format");
    Check(pal4::EquipmentLevelFormatter::Format(10) ==
              "1\n\n0\n \n " &&
              pal4::EquipmentLevelFormatter::Format(90) ==
              "9\n\n0\n \n ",
          "multiples of ten use blank line before zero");
    Check(pal4::EquipmentLevelFormatter::Format(11) ==
              "1\n0\n1\n \n " &&
              pal4::EquipmentLevelFormatter::Format(99) ==
              "9\n0\n9\n \n ",
          "two-digit nonmultiples use explicit zero separator");
}

void TestRuntimeWiring() {
    pal4::EquipmentWindowActionsRuntimeStub runtime;
    Check(runtime.FormatLevel(37u) == "3\n0\n7\n \n " &&
              runtime.FormatLevel(0u) == "0",
          "equipment window runtime stub uses reconstructed formatter");
}

}  // namespace

int main() {
    TestBoundaries();
    TestRuntimeWiring();
}
