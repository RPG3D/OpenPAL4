#include "pal4/game_resource_manager.h"

#include <cstdlib>
#include <iostream>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) { std::cerr << "FAIL: " << message << '\n'; std::exit(1); }
}

class Runtime final : public pal4::GameResourceManagerRuntimeStub {
public:
    void ProcessAtomicBounds(std::uint32_t clump, std::uint32_t mode,
                             std::uint32_t* bbox) override {
        Check(clump == 0x700 && mode == calls.size() &&
              bbox == manager + (mode == 0 ? 60 : 66),
              "primary bounds order and destination");
        const auto index = mode == 0 ? 60 : 66;
        for (std::size_t i = 0; i < 3; ++i) {
            Check(manager[index + i] == 0xff7fffffu &&
                  manager[index + i + 3] == 0x7f7fffffu,
                  "finite bounds seeds");
        }
        calls.push_back(mode);
    }
    std::uint32_t ProcessAtomicBounds2(std::uint32_t clump,
        std::uint32_t first, std::uint32_t second,
        std::uint32_t* output) override {
        Check(clump == 0x700 && second == 1 &&
              first == (calls.size() == 2 ? 1u : 0u) &&
              output == manager + (first == 1 ? 56 : 52),
              "auxiliary bounds order and destination");
        calls.push_back(first + 10);
        return first == 1 ? 0x111 : 0x222;
    }
    std::uint32_t* manager{};
    std::vector<std::uint32_t> calls;
};

void BoundTraversals() {
    pal4::GameResourceManager manager;
    Runtime runtime;
    (void)manager.Construct();
    runtime.manager = manager.words.data();
    manager.words[60] = 0x99;
    Check(manager.CalculateBoundingBox(runtime) == 0 &&
          manager.words[60] == 0x99 && runtime.calls.empty(),
          "missing clump preserves bounds");
    manager.words[5] = 0x700;
    Check(manager.CalculateBoundingBox(runtime) == 0x222 &&
          runtime.calls == std::vector<std::uint32_t>{0, 1, 11, 10},
          "four traversals and final return");
}

}  // namespace

int main() { BoundTraversals(); }
