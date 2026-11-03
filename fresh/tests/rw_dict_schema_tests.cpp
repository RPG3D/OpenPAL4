#include "pal4/rw_dict_schema.h"

#include <algorithm>
#include <bit>
#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

class Runtime final : public pal4::IRwDictSchemaRuntime {
public:
    void DestroyList(std::uint32_t address) override { events.push_back("free"); freed = address; }
    void DestroyDict(std::uint32_t value) override {
        events.push_back("destroy"); destroyed = value;
    }
    std::uint32_t SetCurrentDict(std::uint32_t value) override {
        events.push_back("current"); current = value; return 0x77;
    }
    std::uint32_t OpenStream(std::uint32_t type, std::uint32_t mode,
                             const char* source) override {
        events.push_back("open");
        Check(type == 2 && mode == 1 && std::string(source) == "uv.dct",
              "stream arguments");
        return stream;
    }
    bool FindChunk(std::uint32_t value, std::uint32_t chunk) override {
        events.push_back("find");
        Check(value == stream && chunk == 43, "UVAnim chunk query");
        return has_chunk;
    }
    std::uint32_t ReadDict(std::uint32_t value) override {
        events.push_back("read"); Check(value == stream, "read stream");
        return read_result;
    }
    void CloseStream(std::uint32_t value) override {
        events.push_back("close"); Check(value == stream, "close stream");
    }
    std::uint32_t CreateList(std::uint32_t size, std::uint32_t hint) override {
        events.push_back("create"); Check(size == 4 && hint == 0, "list shape");
        return list_result;
    }
    std::uint32_t ForAllAtomics(std::uint32_t clump,
        std::uint32_t (*callback)(std::uint32_t, std::uint32_t,
                                   pal4::IRwDictSchemaRuntime&),
        std::uint32_t list) override {
        events.push_back("atomics");
        Check(clump == 0x100 && list == list_result, "clump traversal arguments");
        Check(callback(0x200, list, *this) == 0x200, "atomic callback result");
        return 0x11;
    }
    std::uint32_t ForAllGeometryMaterials(std::uint32_t geometry,
        std::uint32_t (*callback)(std::uint32_t, std::uint32_t,
                                   pal4::IRwDictSchemaRuntime&),
        std::uint32_t list) override {
        events.push_back("geometry");
        Check(geometry == 0x300 && list == list_result, "geometry traversal arguments");
        for (const auto material : materials) {
            Check(callback(material, list, *this) == material, "material callback result");
        }
        return 0x22;
    }
    std::uint32_t ForAllWorldMaterials(std::uint32_t world,
        std::uint32_t (*callback)(std::uint32_t, std::uint32_t,
                                   pal4::IRwDictSchemaRuntime&),
        std::uint32_t list) override {
        events.push_back("world");
        Check(world == 0x400 && list == list_result, "world traversal arguments");
        for (const auto material : materials) {
            Check(callback(material, list, *this) == material, "world callback result");
        }
        return 0x33;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        if (address == 0x218) return 0x300;
        return words.at(address);
    }
    bool HasUVAnim(std::uint32_t material) override { return material != 0x502; }
    std::uint32_t ListBegin(std::uint32_t) override { return 0x1000; }
    std::uint32_t ListEnd(std::uint32_t) override { return 0x1000 + count * 4; }
    std::uint32_t ListAppend(std::uint32_t) override {
        events.push_back("append"); return 0x1000 + count++ * 4;
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
    }
    void AdvanceUVAnim(std::uint32_t material, float elapsed) override {
        events.push_back("advance");
        advanced.push_back(material);
        elapsed_values.push_back(elapsed);
    }
    void ApplyUVAnim(std::uint32_t material) override {
        events.push_back("apply"); applied.push_back(material);
    }

    std::vector<std::string> events;
    std::vector<std::uint32_t> materials;
    std::vector<std::uint32_t> advanced;
    std::vector<std::uint32_t> applied;
    std::vector<float> elapsed_values;
    std::map<std::uint32_t, std::uint32_t> words;
    std::uint32_t stream{0x700};
    std::uint32_t read_result{0x800};
    std::uint32_t list_result{0x900};
    std::uint32_t freed{};
    std::uint32_t destroyed{};
    std::uint32_t current{};
    std::uint32_t count{};
    bool has_chunk{true};
};

void InitializationAndCleanup() {
    Runtime runtime;
    pal4::RwDictSchema schema;
    schema.dict = 5; schema.list = 6; schema.time_scale = 2;
    Check(&schema.Initialize() == &schema && schema.dict == 0 &&
          schema.list == 0 && schema.time_scale == 1, "initialize fields");
    schema.dict = 5; schema.list = 6;
    Check(schema.CleanupWrapper(runtime) == 0x77 && schema.dict == 0 &&
          schema.list == 0 && runtime.freed == 6 && runtime.destroyed == 5 &&
          runtime.events == std::vector<std::string>{"free", "destroy", "current"},
          "cleanup order and wrapper return");
    runtime.events.clear();
    Check(schema.Cleanup(runtime) == 0x77 && runtime.events ==
          std::vector<std::string>{"current"}, "empty cleanup resets current");
}

void Loading() {
    for (int mode = 0; mode < 4; ++mode) {
        Runtime runtime;
        pal4::RwDictSchema schema;
        schema.dict = 5; schema.list = 6;
        if (mode == 1) runtime.stream = 0;
        if (mode == 2) runtime.has_chunk = false;
        if (mode == 3) runtime.read_result = 0;
        const bool success = mode == 0;
        Check(schema.LoadDict("uv.dct", runtime) == success &&
              schema.dict == (success ? 0x800u : 0u) && schema.list == 0 &&
              runtime.current == schema.dict && runtime.freed == 6 &&
              runtime.destroyed == 5, "load success and failure branches");
        Check((mode == 1) == (runtime.events.end() ==
              std::find(runtime.events.begin(), runtime.events.end(), "close")),
              "close only an opened stream");
    }
}

void Traversal() {
    Runtime runtime;
    pal4::RwDictSchema schema;
    Check(!schema.SetClump(0x100, runtime) && !schema.SetWorld(0x400, runtime),
          "no dictionary gates both traversal paths");
    schema.dict = 0x800;
    runtime.materials = {0x501, 0x502, 0x501};
    Check(schema.SetClump(0x100, runtime) && schema.list == 0x900 &&
          runtime.count == 1 && runtime.words[0x1000] == 0x501,
          "clump collects unique animated materials");
    runtime.count = 0;
    Check(schema.SetWorld(0x400, runtime) && runtime.freed == 0x900 &&
          runtime.count == 1 && runtime.words[0x1000] == 0x501,
          "world recreates list and collects materials");
    runtime.list_result = 0;
    Check(schema.CreateWorldList(0x400, runtime) == 0 && schema.list == 0,
          "failed list creation skips traversal");
}

void UpdateAndConstants() {
    Runtime runtime;
    pal4::RwDictSchema schema;
    Check(schema.Update(2.0f, runtime) == 0 && runtime.events.empty(),
          "null list returns null without update");
    schema.list = 0x900;
    schema.time_scale = 1.5f;
    runtime.words[0x1000] = 0x501;
    runtime.words[0x1004] = 0x503;
    runtime.count = 2;
    Check(schema.Update(2.0f, runtime) == 0x1008 &&
          runtime.advanced == std::vector<std::uint32_t>{0x501, 0x503} &&
          runtime.applied == runtime.advanced &&
          runtime.elapsed_values == std::vector<float>{3.0f, 3.0f} &&
          runtime.events == std::vector<std::string>{"advance", "apply", "advance", "apply"},
          "update multiplies elapsed time and advances before apply");
    float value = 0;
    pal4::RwDictSchema::InitConstantsWrapper(value);
    Check(std::bit_cast<std::uint32_t>(value) == 0x4096cbe4u,
          "constant wrapper stores sum of IDA float operands");
}

}  // namespace

int main() {
    InitializationAndCleanup();
    Loading();
    Traversal();
    UpdateAndConstants();
}
