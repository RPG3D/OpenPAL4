#include "pal4/rw_clump_optimizer.h"

#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) { std::cerr << "FAIL: " << message << '\n'; std::exit(1); }
}

class Runtime final : public pal4::IRwClumpOptimizerRuntime {
public:
    std::uint32_t ReadOptimizerWord(std::uint32_t address) override {
        events.push_back("read"); return words[address];
    }
    void WriteOptimizerWord(std::uint32_t address,
                            std::uint32_t value) override {
        events.push_back("write"); words[address] = value;
    }
    void SetTriStripMethod(std::uint32_t context) override {
        Check(context == 0, "tri-strip context");
        events.push_back("set_method");
    }
    std::uint8_t TriStripMeshFlag() override {
        events.push_back("mesh_flag"); return 0x6a;
    }
    std::uint32_t ForAllOptimizerMeshes(std::uint32_t geometry,
        std::uint32_t (*callback)(std::uint32_t, std::uint32_t,
                                   std::uint32_t, pal4::IRwClumpOptimizerRuntime&),
        std::uint32_t context) override {
        Check(geometry == 0xB00 && context == 0,
              "geometry mesh traversal");
        events.push_back("meshes");
        Check(callback(geometry, 0xD00, context, *this) == geometry,
              "mesh callback result");
        return geometry;
    }
    void OptimizeAtomicRaw(std::uint32_t atomic) override {
        Check(atomic == 0xA00 || atomic == 0xC00,
              "native atomic optimization");
        events.push_back("atomic_raw");
    }
    void OptimizeClumpRaw(std::uint32_t clump,
                          std::uint32_t mode) override {
        Check(clump == 0x700 && mode == 0, "native clump mode");
        events.push_back("clump_raw");
    }
    std::uint32_t ForAllOptimizerAtomics(std::uint32_t clump,
        std::uint32_t (*callback)(std::uint32_t, std::uint32_t,
                                   pal4::IRwClumpOptimizerRuntime&),
        std::uint32_t context) override {
        Check(clump == 0x700 && context == 0,
              "clump atomic traversal");
        events.push_back("atomics");
        for (auto atomic : {0xA00u, 0xC00u}) {
            Check(callback(atomic, context, *this) == atomic,
                  "atomic callback result");
        }
        return 0x777;
    }

    std::map<std::uint32_t, std::uint32_t> words{
        {0xA18, 0xB00}, {0xC18, 0}, {0xD00, 0x12345678}};
    std::vector<std::string> events;
};

void OptimizationChain() {
    Runtime runtime;
    Check(pal4::RwClumpOptimizer::OptimizeClump(0, runtime) == 0 &&
          runtime.events.empty(), "empty clump gate");
    Check(pal4::RwClumpOptimizer::OptimizeClump(0x700, runtime) == 0x777 &&
          runtime.words[0xD00] == 0x1234566a &&
          runtime.events == std::vector<std::string>{
              "clump_raw", "atomics", "read", "meshes", "set_method",
              "read", "mesh_flag", "write", "atomic_raw", "read",
              "atomic_raw"},
          "clump, geometry and atomic optimization order");
}

void DirectCallbacks() {
    Runtime runtime;
    Check(pal4::RwClumpOptimizer::OptimizeAtomic(0xC00, 4, runtime) ==
              0xC00 &&
          runtime.events == std::vector<std::string>{"read", "atomic_raw"},
          "missing geometry still optimizes atomic");
    runtime.events.clear();
    Check(pal4::RwClumpOptimizer::SetMeshTriStripMethod(
              0xB00, 0xD00, 0, runtime) == 0xB00 &&
          runtime.words[0xD00] == 0x1234566a &&
          runtime.events == std::vector<std::string>{
              "set_method", "read", "mesh_flag", "write"},
          "mesh high bytes retained and low byte replaced");
}

}  // namespace

int main() {
    OptimizationChain();
    DirectCallbacks();
}
