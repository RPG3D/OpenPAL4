#include "pal4/scene_evf_record.h"

#include <array>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace {

void Check(bool good, const char* message) {
    if (!good) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct StreamRuntime final : pal4::ISceneEvfStreamRuntime {
    std::vector<std::uint32_t> input;
    std::size_t cursor = 0u;
    std::uint32_t type = 0u;
    std::uint32_t next_property = 0u;
    std::uint32_t next_allocation = 0x5000u;
    bool has_clump = false;
    std::map<std::uint32_t, pal4::EffectPropertyState> properties;
    std::vector<std::uint32_t> allocation_sizes;
    std::vector<std::uint32_t> freed;
    std::vector<std::string> events;

    void ReadName(std::uint32_t stream, pal4::SceneEvfRecordState& state,
                  std::uint32_t offset) override {
        Check(stream == 0x100u && offset == 104u, "name stream and offset");
        events.push_back("name");
        state.words[26u] = 0x123u;
    }
    std::uint32_t ReadWord(std::uint32_t stream) override {
        Check(stream == 0x100u && cursor < input.size(), "word read order");
        events.push_back("word");
        return input[cursor++];
    }
    void ReadEffectFields(std::uint32_t stream,
                          pal4::SceneEvfRecordState& state) override {
        Check(stream == 0x100u, "effect field stream");
        events.push_back("fields");
        state.words[1u] = type;
        state.words[18u] = 0x777u;
    }
    void ReadProperty(std::uint32_t stream,
                      pal4::EffectPropertyState& property) override {
        Check(stream == 0x100u, "property stream");
        events.push_back("read-property");
        property.words[0u] = ++next_property;
        property.words[1u] = 100u + next_property;
        property.words[2u] = 200u + next_property;
        property.words[16u] = 0xabu;
    }
    pal4::EffectPropertyState& PropertyAt(std::uint32_t address) override {
        return properties.at(address);
    }
    void WriteProperty(std::uint32_t address,
                       const pal4::EffectPropertyState& property) override {
        events.push_back("write-property");
        properties[address] = property;
    }
    std::uint32_t AllocatePropertyBytes(std::uint32_t bytes) override {
        allocation_sizes.push_back(bytes);
        events.push_back("allocate");
        const auto address = next_allocation;
        next_allocation += 0x1000u;
        return address;
    }
    void FreePropertyBuffer(std::uint32_t address) override {
        events.push_back("free-property");
        freed.push_back(address);
    }
    bool FindClumpChunk(std::uint32_t stream,
                        std::uint32_t chunk_id) override {
        Check(stream == 0x100u && chunk_id == 16u,
              "RenderWare clump chunk id");
        events.push_back("find-clump");
        return has_clump;
    }
    std::uint32_t ClumpWorld(std::uint32_t clump) override {
        Check(clump == 0x600u, "old clump world lookup");
        events.push_back("clump-world");
        return 0x700u;
    }
    void RemoveClump(std::uint32_t world,
                     std::uint32_t clump) override {
        Check(world == 0x700u && clump == 0x600u,
              "old clump removal");
        events.push_back("remove-clump");
    }
    void DestroyClump(std::uint32_t clump) override {
        Check(clump == 0x600u, "old clump destruction");
        events.push_back("destroy-clump");
    }
    std::uint32_t ReadClump(std::uint32_t stream) override {
        Check(stream == 0x100u, "clump stream");
        events.push_back("read-clump");
        return 0x800u;
    }
    std::vector<std::uint32_t> Atomics(std::uint32_t clump) override {
        Check(clump == 0x800u, "new clump atomics");
        events.push_back("atomics");
        return {0x900u, 0xa00u};
    }
};

struct TypeRuntime final : pal4::ISceneEvfRecordTypeRuntime {
    std::uint32_t requested_type = 0xffffffffu;
    void DestroyOld(std::uint32_t, std::uint32_t) override {}
    std::uint32_t FindFactory(std::uint32_t owner,
                              std::uint32_t type) override {
        Check(owner == 0x777u, "stream fields precede type setup");
        requested_type = type;
        return 0u;
    }
    std::uint32_t Create(std::uint32_t) override { return 0u; }
    void BindOwnerField(std::uint32_t, std::uint32_t) override {}
};

struct GeometryRuntime final : pal4::ISceneEvfGeometryRuntime {
    StreamRuntime& stream;
    std::uint32_t next_allocation = 0x10000u;
    std::vector<std::string> events;
    std::vector<pal4::SceneEvfVertex> vertices;
    std::vector<std::uint16_t> indices;
    std::vector<pal4::SceneEvfTriangle> triangles;
    explicit GeometryRuntime(StreamRuntime& source) : stream(source) {}
    void Free(std::uint32_t address) override {
        events.push_back("free:" + std::to_string(address));
    }
    std::uint32_t Allocate(std::uint32_t bytes) override {
        events.push_back("allocate:" + std::to_string(bytes));
        const auto address = next_allocation;
        next_allocation += 0x1000u;
        return address;
    }
    std::vector<pal4::EffectPropertyState> ReadProperties(
        std::uint32_t begin, std::uint32_t count) override {
        events.push_back("geometry-properties");
        std::vector<pal4::EffectPropertyState> result;
        for (std::uint32_t i = 0u; i < count; ++i)
            result.push_back(stream.properties.at(begin + 80u * i));
        return result;
    }
    std::array<std::uint8_t, 4u> ReadColor(std::uint32_t) override {
        return {0x11u, 0x22u, 0x33u, 0x44u};
    }
    float PlaneFromPoints(const pal4::EffectPropertyState&,
                          const pal4::EffectPropertyState&,
                          const pal4::EffectPropertyState&,
                          std::array<float, 3u>& normal) override {
        normal = {1.0f, 2.0f, 3.0f};
        return 4.0f;
    }
    void WriteIndices(std::uint32_t,
                      const std::vector<std::uint16_t>& value) override {
        indices = value;
    }
    void WriteVertices(std::uint32_t,
                       const std::vector<pal4::SceneEvfVertex>& value) override {
        vertices = value;
    }
    void WriteTriangles(
        std::uint32_t,
        const std::vector<pal4::SceneEvfTriangle>& value) override {
        triangles = value;
    }
    void WarnInsufficientVertices() override { events.push_back("warn-strip"); }
    void WarnInsufficientPolygonVertices() override {
        events.push_back("warn-polygon");
    }
    void WarnInsufficientVolumeVertices() override {
        events.push_back("warn-volume");
    }
    void ClearPropertyRange(std::uint32_t,
                            std::uint32_t) override {
        events.push_back("clear-properties");
    }
};

struct AtomicRuntime final : pal4::ISceneEvfAtomicRuntime {
    std::vector<std::uint32_t> atomics;
    std::uint32_t IdentityRenderCallback() override { return 0x7b4bd0u; }
    std::uint32_t DefaultRenderCallback() override { return 0u; }
    void WriteCallback(std::uint32_t atomic, std::uint32_t offset,
                       std::uint32_t callback) override {
        Check(offset == 72u && callback == 0x7b4bd0u,
              "atomic identity callback");
        atomics.push_back(atomic);
    }
};

}  // namespace

int main() {
    StreamRuntime range_runtime;
    range_runtime.properties[0x3000u].words[0u] = 11u;
    range_runtime.properties[0x3050u].words[0u] = 22u;
    Check(pal4::SceneEvfRecord::CopyPropertyRange(
              0x3000u, 0x30a0u, 0x4000u, range_runtime) == 0x40a0u &&
              range_runtime.properties[0x4000u].words[0u] == 11u &&
              range_runtime.properties[0x4050u].words[0u] == 22u,
          "copy helper preserves order and returns destination end");
    const auto written = range_runtime.events.size();
    Check(pal4::SceneEvfRecord::CopyPropertyRange(
              0x3000u, 0x30a0u, 0u, range_runtime) == 160u &&
              range_runtime.events.size() == written + 1u &&
              range_runtime.properties[80u].words[0u] == 22u &&
              pal4::SceneEvfRecord::CopyPropertyRange(
                  0x3000u, 0x3000u, 0x6000u, range_runtime) == 0x6000u,
          "null destination skips only the first write as pointer advances");
    pal4::EffectPropertyState fill;
    fill.words[0u] = 33u;
    Check(pal4::SceneEvfRecord::FillPropertyRange(
              0x5000u, 2u, fill, range_runtime) == 0x50a0u &&
              range_runtime.properties[0x5000u].words[0u] == 33u &&
              range_runtime.properties[0x5050u].words[0u] == 33u &&
              pal4::SceneEvfRecord::FillPropertyRange(
                  0x6000u, 0u, fill, range_runtime) == 0u,
          "fill helper repeats record and zero count returns zero");
    Check(pal4::SceneEvfRecord::CopyPropertyRangeUnchecked(
              0x3000u, 0x30a0u, 0x6000u, range_runtime) == 0x60a0u &&
              range_runtime.properties[0x6000u].words[0u] == 11u &&
              range_runtime.properties[0x6050u].words[0u] == 22u &&
              pal4::SceneEvfRecord::CopyPropertyRangeUnchecked(
                  0x3000u, 0x3000u, 0x7000u, range_runtime) == 0x7000u,
          "unchecked copy advances through records and handles empty range");

    pal4::SceneEvfRecordState state;
    StreamRuntime stream;
    TypeRuntime type;
    GeometryRuntime geometry(stream);
    AtomicRuntime atomic;
    state.words[21u] = 0x3000u;
    state.words[22u] = 0x3050u;
    Check(!pal4::SceneEvfRecord::ReadFromStream(
              state, 0x2000u, 0u, stream, type, geometry, atomic) &&
              state.words[22u] == 0x3050u && stream.events.empty(),
          "null stream returns before touching record");

    state = {};
    stream.input = {3u, 3u, 0x11u, 0x22u};
    stream.type = 3u;
    stream.has_clump = true;
    state.words[17u] = 0x600u;
    Check(pal4::SceneEvfRecord::ReadFromStream(
              state, 0x2000u, 0x100u, stream, type, geometry, atomic) &&
              stream.cursor == 4u && type.requested_type == 3u &&
              state.words[0u] == 3u && state.words[36u] == 0x11u &&
              state.words[37u] == 0x22u && state.words[17u] == 0x800u &&
              stream.allocation_sizes ==
                  std::vector<std::uint32_t>{80u, 160u, 320u} &&
              stream.freed ==
                  std::vector<std::uint32_t>{0u, 0x5000u, 0x6000u} &&
              state.words[21u] == 0x7000u &&
              state.words[22u] == 0x7000u + 3u * 80u &&
              state.words[23u] == 0x7000u + 4u * 80u &&
              stream.properties.at(0x7000u).words[0u] == 1u &&
              stream.properties.at(0x7050u).words[0u] == 2u &&
              stream.properties.at(0x70a0u).words[0u] == 3u &&
              atomic.atomics == std::vector<std::uint32_t>{0x900u, 0xa00u} &&
              geometry.events.empty(),
          "growth, property copy, clump replacement and unknown mode");
    Check(stream.events[0u] == "name" && stream.events[1u] == "word" &&
              stream.events[2u] == "fields" &&
              stream.events[3u] == "word" &&
              stream.events[stream.events.size() - 5u] == "clump-world" &&
              stream.events[stream.events.size() - 4u] == "remove-clump" &&
              stream.events[stream.events.size() - 3u] == "destroy-clump" &&
              stream.events[stream.events.size() - 2u] == "read-clump" &&
              stream.events.back() == "atomics",
          "stream and RenderWare operation order");

    for (std::uint32_t mode : {0u, 1u, 2u}) {
        pal4::SceneEvfRecordState record;
        StreamRuntime source;
        TypeRuntime type_source;
        GeometryRuntime geometry_source(source);
        AtomicRuntime atomic_source;
        const auto count = mode == 0u ? 2u : mode == 1u ? 4u : 8u;
        source.input = {mode, count, 1u, 1u};
        source.type = mode;
        Check(pal4::SceneEvfRecord::ReadFromStream(
                  record, 0x2000u, 0x100u, source, type_source,
                  geometry_source, atomic_source) &&
                  record.words[32u] == count &&
                  geometry_source.vertices.size() == count &&
                  !geometry_source.indices.empty() &&
                  geometry_source.triangles.size() ==
                      geometry_source.indices.size() / 3u,
              "three geometry modes call reconstructed generators");
    }

    pal4::SceneEvfRecordState reused;
    StreamRuntime reuse_stream;
    TypeRuntime reuse_type;
    GeometryRuntime reuse_geometry(reuse_stream);
    AtomicRuntime reuse_atomic;
    reused.words[21u] = 0x3000u;
    reused.words[22u] = 0x30a0u;
    reused.words[23u] = 0x30f0u;
    reused.words[29u] = 0x900u;
    reused.words[31u] = 0xa00u;
    reused.words[41u] = 0xb00u;
    reuse_stream.properties[0x3000u].words[16u] = 0x99u;
    reuse_stream.properties[0x3050u].words[16u] = 0x88u;
    reuse_stream.input = {0u, 1u, 0u, 0u};
    Check(pal4::SceneEvfRecord::ReadFromStream(
              reused, 0x2000u, 0x100u, reuse_stream, reuse_type,
              reuse_geometry, reuse_atomic) &&
              reused.words[21u] == 0x3000u &&
              reused.words[22u] == 0x3000u &&
              reused.words[23u] == 0x30f0u &&
              reuse_stream.allocation_sizes.empty() &&
              reuse_stream.properties.at(0x3050u).words[16u] == 0u &&
              reuse_geometry.events == std::vector<std::string>{
                  "free:2304", "free:2560", "free:2816",
                  "warn-strip", "clear-properties"},
          "old records clear and existing capacity is reused before short strip");
}
