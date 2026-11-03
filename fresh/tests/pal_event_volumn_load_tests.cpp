#include "pal4/pal_event_volumn_manager.h"

#include <cstdint>
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

struct Runtime final : pal4::IPalEventVolumnSceneLoadRuntime,
                       pal4::ISceneEvfRecordRuntime,
                       pal4::ISceneEvfStreamRuntime,
                       pal4::ISceneEvfRecordTypeRuntime,
                       pal4::ISceneEvfGeometryRuntime,
                       pal4::ISceneEvfAtomicRuntime {
    std::vector<std::string> events;
    std::map<std::uint32_t, pal4::SceneEvfRecordState> records;
    std::map<std::uint32_t, std::uint32_t> memory;
    std::uint32_t context = 0x100u;
    std::uint32_t count = 0u;
    std::uint32_t allocations = 0u;
    std::uint32_t fail_on_allocation = 0u;
    std::uint32_t stream_words = 0u;
    std::string path;

    std::uint32_t OpenStream(std::string_view value, std::uint32_t mode,
                             std::uint32_t flags) override {
        Check(mode == 2u && flags == 1u, "stream mode and flags");
        path = value;
        events.push_back("open");
        return context;
    }
    std::uint32_t ReadRecordCount(std::uint32_t stream) override {
        Check(stream == context, "count stream");
        events.push_back("count");
        return count;
    }
    std::uint32_t AllocateRecordBytes(std::uint32_t bytes) override {
        Check(bytes == 172u, "record size");
        events.push_back("allocate-record");
        if (++allocations == fail_on_allocation) return 0u;
        return 0x2000u + allocations * 0x100u;
    }
    pal4::SceneEvfRecordState& RecordAt(std::uint32_t address) override {
        return records[address];
    }
    void InsertRecord(pal4::PalEventVolumnState& state,
                      std::uint32_t address) override {
        events.push_back("insert:" + std::to_string(address));
        memory[state.words[2u]] = address;
        state.words[2u] += 4u;
    }
    void CloseStream(std::uint32_t stream, std::uint32_t flags) override {
        Check(stream == context && flags == 0u, "stream close arguments");
        events.push_back("close");
    }
    int FinishTemporaryPath() override {
        events.push_back("finish");
        return 37;
    }

    void InitializeEmptyString(pal4::SceneEvfRecordState&,
                               std::uint32_t) override {}
    std::uint32_t Allocate(std::uint32_t bytes) override {
        Check(bytes == 12u, "record list head size");
        return 0x9000u + allocations * 0x100u;
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        memory[address] = value;
    }

    pal4::EffectPropertyState& PropertyAt(std::uint32_t) override {
        Check(false, "zero properties expected");
        static pal4::EffectPropertyState unused;
        return unused;
    }
    void FreePropertyBuffer(std::uint32_t) override {}
    void ReadName(std::uint32_t, pal4::SceneEvfRecordState&,
                  std::uint32_t offset) override {
        Check(offset == 104u, "EVF name offset");
        events.push_back("read-name");
    }
    std::uint32_t ReadWord(std::uint32_t stream) override {
        Check(stream == context, "record stream");
        ++stream_words;
        return stream_words % 4u == 1u ? 9u : 0u;
    }
    void ReadEffectFields(std::uint32_t, pal4::SceneEvfRecordState& state) override {
        events.push_back("read-fields");
        state.words[1u] = 6u;
    }
    void ReadProperty(std::uint32_t, pal4::EffectPropertyState&) override {
        Check(false, "zero properties expected");
    }
    void WriteProperty(std::uint32_t,
                       const pal4::EffectPropertyState&) override {}
    std::uint32_t AllocatePropertyBytes(std::uint32_t) override { return 0u; }
    bool FindClumpChunk(std::uint32_t, std::uint32_t chunk) override {
        Check(chunk == 16u, "clump chunk id");
        return false;
    }
    std::uint32_t ClumpWorld(std::uint32_t) override { return 0u; }
    void RemoveClump(std::uint32_t, std::uint32_t) override {}
    void DestroyClump(std::uint32_t) override {}
    std::uint32_t ReadClump(std::uint32_t) override { return 0u; }
    std::vector<std::uint32_t> Atomics(std::uint32_t) override { return {}; }

    void DestroyOld(std::uint32_t, std::uint32_t) override {}
    std::uint32_t FindFactory(std::uint32_t, std::uint32_t) override { return 0u; }
    std::uint32_t Create(std::uint32_t) override { return 0u; }
    void BindOwnerField(std::uint32_t, std::uint32_t) override {}

    void Free(std::uint32_t) override {}
    std::vector<pal4::EffectPropertyState> ReadProperties(
        std::uint32_t, std::uint32_t) override { return {}; }
    std::array<std::uint8_t, 4u> ReadColor(std::uint32_t) override { return {}; }
    float PlaneFromPoints(const pal4::EffectPropertyState&,
                          const pal4::EffectPropertyState&,
                          const pal4::EffectPropertyState&,
                          std::array<float, 3u>&) override { return 0.0f; }
    void WriteIndices(std::uint32_t,
                      const std::vector<std::uint16_t>&) override {}
    void WriteVertices(std::uint32_t,
                       const std::vector<pal4::SceneEvfVertex>&) override {}
    void WriteTriangles(std::uint32_t,
                        const std::vector<pal4::SceneEvfTriangle>&) override {}
    void WarnInsufficientVertices() override {}
    void WarnInsufficientPolygonVertices() override {}
    void WarnInsufficientVolumeVertices() override {}
    void ClearPropertyRange(std::uint32_t, std::uint32_t) override {}

    std::uint32_t IdentityRenderCallback() override { return 0u; }
    std::uint32_t DefaultRenderCallback() override { return 0u; }
    void WriteCallback(std::uint32_t, std::uint32_t,
                       std::uint32_t) override {}
};

struct Cleanup final : pal4::IPalEventVolumnCleanupRuntime {
    Runtime& runtime;
    explicit Cleanup(Runtime& value) : runtime(value) {}
    pal4::PalEventVolumnState& ObjectAt(std::uint32_t) override {
        static pal4::PalEventVolumnState unused;
        return unused;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return runtime.memory.at(address);
    }
    void DestroyRecord(std::uint32_t address) override {
        runtime.events.push_back("destroy:" + std::to_string(address));
    }
    void ClearTree(pal4::PalEventVolumnState&,
                   pal4::PalEventVolumnSharedTree&) override {}
    int Free(std::uint32_t) override { return 0; }
};

}  // namespace

int main() {
    Runtime runtime;
    Cleanup cleanup(runtime);
    pal4::PalEventVolumnLoadServices services{
        runtime, cleanup, runtime, runtime, runtime, runtime, runtime};
    pal4::PalEventVolumnState state;
    state.words[1u] = 0x4000u;
    state.words[2u] = 0x4004u;
    state.words[4u] = 0xdeadbeefu;
    state.words[5u] = 1u;
    runtime.memory[0x4000u] = 0x5000u;
    runtime.count = 2u;
    Check(pal4::PalEventVolumnManager::LoadSceneEvfRecords(
              state, 0x3000u, "root\\", "maps", "town", services) == 37,
          "temporary path result");
    Check(runtime.path == "root\\maps\\town\\town.evf", "EVF path");
    Check(state.words[5u] == 2u && state.words[2u] == 0x4008u &&
              state.words[4u] == 0u && runtime.records.size() == 2u,
          "old records replaced by two initialized records");
    Check(runtime.records.at(0x2100u).words[18u] == 0x3000u &&
              runtime.records.at(0x2200u).words[18u] == 0x3000u,
          "both records keep manager owner");
    Check(runtime.events.front() == "destroy:20480" &&
              runtime.events[1u] == "open" &&
              runtime.events[runtime.events.size() - 2u] == "close" &&
              runtime.events.back() == "finish", "cleanup and close order");

    runtime.events.clear();
    runtime.context = 0u;
    Check(pal4::PalEventVolumnManager::LoadSceneEvfRecords(
              state, 0x3000u, "", "", "x", services) == 37 &&
              state.words[5u] == 0u &&
              runtime.events == std::vector<std::string>{
                  "destroy:8448", "destroy:8704", "open", "finish"},
          "failed open still clears previous records");

    runtime.events.clear();
    runtime.context = 0x100u;
    runtime.count = 0u;
    Check(pal4::PalEventVolumnManager::LoadSceneEvfRecords(
              state, 0x3000u, "", "", "x", services) == 37 &&
              runtime.events == std::vector<std::string>{
                  "open", "count", "close", "finish"},
          "empty stream closes without allocations");

    runtime.events.clear();
    runtime.count = 2u;
    runtime.fail_on_allocation = runtime.allocations + 1u;
    Check(pal4::PalEventVolumnManager::LoadSceneEvfRecords(
              state, 0x3000u, "", "", "x", services) == 37 &&
              state.words[5u] == 0u &&
              runtime.events == std::vector<std::string>{
                  "open", "count", "allocate-record", "close", "finish"},
          "allocation failure closes stream and stops loading");
}
