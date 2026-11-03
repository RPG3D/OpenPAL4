#include "pal4/game_world_resource_name.h"

#include <cstdlib>
#include <iostream>
#include <stdexcept>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

}  // namespace

int main() {
    Check(pal4::GameWorldResourceName::Build(
              "base\\scene\\variant\\variant.bsp") ==
              "Accelerate\\scene\\variant\\variant_d3d9.txd",
          "world path drops outer directory and extension");
    Check(pal4::GameWorldResourceName::Build(
              "C:\\PAL4\\scene\\variant_trans.dff") ==
              "Accelerate\\PAL4\\scene\\variant_trans_d3d9.txd",
          "clump path retains relative directories and basename suffix");
    Check(pal4::GameWorldResourceName::Build(
              "root\\folder.with.dot\\name.clip.dff") ==
              "Accelerate\\folder.with.dot\\name.clip_d3d9.txd",
          "last dot rather than directory dot chooses extension");
    bool rejected = false;
    try {
        static_cast<void>(pal4::GameWorldResourceName::Build("name.dff"));
    } catch (const std::out_of_range&) {
        rejected = true;
    }
    Check(rejected, "invalid resource path has no native directory range");
}
