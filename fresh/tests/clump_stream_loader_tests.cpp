#include "pal4/clump_stream_loader.h"

#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IClumpStreamRuntime {
    std::uint32_t context = 0x3000u;
    std::uint32_t clump = 0x4000u;
    bool chunk_found = true;
    std::vector<std::string> events;
    void PrepareImage(const std::uint32_t image) override {
        Check(image == 0x2000u, "prepares optional image argument");
        events.emplace_back("prepare");
    }
    [[nodiscard]] std::uint32_t CreateContext(
        const std::uint32_t type, const std::uint32_t mode,
        const std::uint32_t source) override {
        Check(type == 2u && mode == 1u && source == 0x1000u,
              "creates type-two read context from source");
        events.emplace_back("create");
        return context;
    }
    [[nodiscard]] bool FindChunk(const std::uint32_t value,
                                 const std::uint32_t chunk,
                                 const std::uint32_t version,
                                 const std::uint32_t length) override {
        Check(value == context && chunk == 16u && version == 0u &&
                  length == 0u,
              "finds clump chunk 16 with null outputs");
        events.emplace_back("find");
        return chunk_found;
    }
    [[nodiscard]] std::uint32_t ReadClump(
        const std::uint32_t value) override {
        Check(value == context, "reads clump from created context");
        events.emplace_back("read-clump");
        return clump;
    }
    void LogLoadedClump(const std::uint32_t source,
                        const std::uint32_t line) override {
        Check(source == 0x1000u && line == 241u,
              "load log uses original source and native line");
        events.emplace_back("log");
    }
    void CloseContext(const std::uint32_t value,
                      const std::uint32_t output) override {
        Check(value == context && output == 0u,
              "closes context without output data");
        events.emplace_back("close");
    }
};

}  // namespace

int main() {
    Runtime runtime;
    Check(pal4::ClumpStreamLoader::Load(0x1000u, 0x2000u, runtime) ==
              0x4000u &&
              runtime.events == std::vector<std::string>({
                  "prepare", "create", "find", "read-clump", "log", "close"}),
          "successful load retains native call order");
    runtime.events.clear();
    runtime.chunk_found = false;
    Check(pal4::ClumpStreamLoader::Load(0x1000u, 0u, runtime) == 0u &&
              runtime.events == std::vector<std::string>({
                  "create", "find", "close"}),
          "missing clump chunk still closes context");
    runtime.events.clear();
    runtime.context = 0u;
    Check(pal4::ClumpStreamLoader::Load(0x1000u, 0u, runtime) == 0u &&
              runtime.events == std::vector<std::string>({"create"}),
          "failed context creation skips subsequent services");
    runtime.events.clear();
    runtime.context = 0x3000u;
    runtime.chunk_found = true;
    runtime.clump = 0u;
    Check(pal4::ClumpStreamLoader::Load(0x1000u, 0u, runtime) == 0u &&
              runtime.events == std::vector<std::string>({
                  "create", "find", "read-clump", "log", "close"}),
          "chunk hit logs even when stream read returns no clump");
    pal4::ClumpStreamRuntimeStub stub;
    Check(pal4::ClumpStreamLoader::Load(1u, 0u, stub) == 0u,
          "RenderWare stream stub links");
    return EXIT_SUCCESS;
}
