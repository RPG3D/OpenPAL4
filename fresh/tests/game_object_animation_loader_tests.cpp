#include "pal4/game_object_animation_loader.h"

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) { std::cerr << message << '\n'; std::exit(1); }
}

class Runtime final : public pal4::IGameObjectAnimationLoadRuntime {
public:
    std::vector<std::string> calls;
    std::vector<std::string> suffixes{"walk.anm", "run.anm"};
    std::map<std::uint32_t, std::uint32_t> animations;
    bool script_ok = true;
    bool hierarchy_attached = false;
    std::uint32_t allocation = 0x1000u;
    std::uint32_t hierarchy = 0x700u;
    std::string failed_animation;
    int snapshot_count = 0;

    void InitializeAnimationData(
        pal4::GameObjectEmbeddedAnimationState&) override {}
    void CleanupAnimationData(
        pal4::GameObjectEmbeddedAnimationState&) override {}
    void CleanupAnimationDataRange(
        pal4::GameObjectEmbeddedAnimationState&) override {}
    std::uint32_t AnimationArrayCount(std::uint32_t address) override {
        Check(address == 0x200u, "reads old array count before elements");
        calls.emplace_back("old-count");
        return 3;
    }
    void DestructAnimationArray(std::uint32_t address,
                                std::uint32_t size,
                                std::uint32_t count) override {
        Check(address == 0x204u && size == 0x44u && count == 3,
              "destructs old 68-byte records");
        calls.emplace_back("old-destruct");
    }
    std::uint32_t FreeMemory(std::uint32_t address) override {
        Check(address == 0x200u, "frees old array header allocation");
        calls.emplace_back("old-free");
        return 0;
    }
    void FreeWrappedMemory(std::uint32_t address) override {
        Check(address == 0x44u || address == 0x55u,
              "frees old blend snapshot");
        calls.emplace_back("free-snapshot:" + std::to_string(address));
    }
    std::uint32_t FindHierarchy(std::uint32_t clump) override {
        Check(clump == 0x55u, "looks up hierarchy from new clump");
        calls.emplace_back("hierarchy");
        return hierarchy;
    }
    void ProcessAnimationData(
        pal4::GameObjectEmbeddedAnimationState&,
        std::uint32_t, std::int32_t) override {}
    bool LoadScript(pal4::GameObjectEmbeddedAnimationState&,
                    std::string_view path) override {
        Check(path == "root/actions.tab", "builds base plus suffix plus .tab");
        calls.emplace_back("script");
        return script_ok;
    }
    std::int32_t AnimationCount(
        pal4::GameObjectEmbeddedAnimationState&) override {
        calls.emplace_back("count");
        return static_cast<std::int32_t>(suffixes.size());
    }
    std::string_view AnimationResourceSuffix(
        pal4::GameObjectEmbeddedAnimationState&,
        std::int32_t index) override {
        calls.emplace_back("suffix:" + std::to_string(index));
        return suffixes.at(static_cast<std::size_t>(index));
    }
    std::uint32_t AllocateRecords(std::uint32_t bytes) override {
        Check(bytes == suffixes.size() * 0x44u + 4u,
              "allocates record stride times count plus header");
        calls.emplace_back("allocate");
        return allocation;
    }
    void ConstructRecords(std::uint32_t first,
                          std::uint32_t count,
                          std::uint32_t stride) override {
        Check(first == 0x1004u && count == suffixes.size() &&
                  stride == 0x44u,
              "constructs records after count header");
        calls.emplace_back("construct");
    }
    void SetRecordName(std::uint32_t record,
                       std::string_view name) override {
        const auto index = (record - 0x1004u) / 0x44u;
        Check(record == 0x1004u + index * 0x44u &&
                  index < suffixes.size() &&
                  name == (index == 0 ? "walk" : "run"),
              "record receives resource name without four-character suffix");
        calls.emplace_back("name:" + std::to_string(index));
    }
    void SetRecordBasePath(std::uint32_t record,
                           std::string_view path) override {
        Check((record == 0x1004u || record == 0x1048u) && path == "root/",
              "record stores base path");
        calls.emplace_back("base");
    }
    std::uint32_t ReadAnimation(std::string_view path) override {
        Check(path == "root/walk.anm" || path == "root/run.anm",
              "reads each base plus resource suffix");
        calls.emplace_back("read:" + std::string(path));
        if (path == failed_animation) return 0;
        return path == "root/walk.anm" ? 0x501u : 0x502u;
    }
    void SetRecordAnimation(std::uint32_t record,
                            std::uint32_t animation) override {
        animations[record] = animation;
        calls.emplace_back("store-animation");
    }
    std::uint32_t RecordAnimation(std::uint32_t record) override {
        Check(record == 0x1004u, "starts with first loaded record");
        calls.emplace_back("first-animation");
        return animations.at(record);
    }
    void LoadAnimationEvents(
        pal4::GameObjectEmbeddedAnimationState&,
        std::uint32_t record) override {
        Check(animations.at(record) != 0,
              "loads event data only after animation read");
        calls.emplace_back("events");
    }
    bool HasHierarchy(std::uint32_t clump) override {
        Check(clump == 0x55u, "checks hierarchy on new clump");
        calls.emplace_back("has-hierarchy");
        return hierarchy_attached;
    }
    void SetHierarchyFlags(std::uint32_t found,
                           std::uint32_t flags) override {
        Check(found == 0x700u && flags == 0x3000u,
              "sets animation hierarchy flags");
        calls.emplace_back("flags");
    }
    void AttachHierarchy(std::uint32_t found) override {
        Check(found == 0x700u, "attaches hierarchy");
        calls.emplace_back("attach");
    }
    void AttachSkinAtomics(std::uint32_t clump,
                           std::uint32_t found) override {
        Check(clump == 0x55u && found == 0x700u,
              "binds clump atomics to hierarchy");
        calls.emplace_back("atomics");
    }
    void InitializeHierarchyAnimationData(
        pal4::GameObjectEmbeddedAnimationState&,
        std::uint32_t found) override {
        Check(found == 0x700u, "initializes state hierarchy data");
        calls.emplace_back("init-hierarchy");
    }
    std::uint32_t HierarchyInterpolator(std::uint32_t found) override {
        Check(found == 0x700u, "reads hierarchy interpolator");
        calls.emplace_back("interpolator");
        return 0x900u;
    }
    void SetCurrentAnimation(std::uint32_t interpolator,
                             std::uint32_t animation) override {
        Check(interpolator == 0x900u && animation == 0x501u,
              "selects first record animation");
        calls.emplace_back("set-animation");
    }
    void SetCurrentTime(std::uint32_t interpolator,
                        float time) override {
        Check(interpolator == 0x900u && time == 0.0f,
              "rewinds current interpolator");
        calls.emplace_back("set-time");
    }
    void UpdateMatrices(std::uint32_t found) override {
        Check(found == 0x700u, "updates attached matrices");
        calls.emplace_back("matrices");
    }
    std::uint32_t HierarchyFrameCount(std::uint32_t found) override {
        Check(found == 0x700u, "reads hierarchy frame count");
        calls.emplace_back("frame-count");
        return 4;
    }
    std::uint32_t InterpolatorFrameData(
        std::uint32_t interpolator) override {
        Check(interpolator == 0x900u, "reads interpolator frame data");
        calls.emplace_back("frame-data");
        return 0xabcdu;
    }
    std::uint32_t CloneBlendSnapshot(
        std::uint32_t count, std::uint32_t data) override {
        Check(count == 4 && data == 0xabcdu,
              "clones current frame data with hierarchy count");
        calls.emplace_back("snapshot");
        return ++snapshot_count == 1 ? 0xa00u : 0xb00u;
    }
};

bool Load(pal4::GameObjectEmbeddedAnimationState& state,
          Runtime& runtime, std::uint32_t clump = 0x55u) {
    return pal4::GameObjectAnimationLoader::LoadData(
        state, clump, "root/", "actions", runtime);
}

void FailureBranches() {
    Runtime runtime;
    pal4::GameObjectEmbeddedAnimationState state;
    Check(!Load(state, runtime, 0) && runtime.calls.empty(),
          "null clump stops before script path construction");
    runtime.script_ok = false;
    Check(!Load(state, runtime) &&
              runtime.calls == std::vector<std::string>{"script"},
          "script failure stops before count or state mutation");
    runtime.calls.clear();
    runtime.script_ok = true;
    runtime.suffixes.clear();
    Check(!Load(state, runtime) &&
              runtime.calls == std::vector<std::string>{"script", "count"},
          "zero animation count stops before allocation");
    runtime.calls.clear();
    runtime.suffixes = {"walk.anm", "run.anm"};
    runtime.allocation = 0;
    Check(!Load(state, runtime) && state.words[2] == 0 &&
              runtime.calls == std::vector<std::string>{
                  "script", "count", "count", "allocate"},
          "allocation failure leaves array pointer null");

    runtime.calls.clear();
    runtime.allocation = 0x1000u;
    runtime.failed_animation = "root/walk.anm";
    Check(!Load(state, runtime) && state.words[2] == 0x1004u &&
              state.words[1] == 0 && state.words[17] == 0 &&
              runtime.animations.at(0x1004u) == 0 &&
              runtime.calls.back() == "store-animation",
          "read failure leaves constructed array but no clump binding");
    runtime.calls.clear();
    state.words[2] = 0;
    runtime.failed_animation = "root/run.anm";
    Check(!Load(state, runtime) && state.words[2] == 0x1004u &&
              state.words[1] == 0 &&
              runtime.animations.at(0x1004u) == 0x501u &&
              runtime.animations.at(0x1048u) == 0 &&
              std::count(runtime.calls.begin(), runtime.calls.end(),
                         "events") == 1,
          "second read failure preserves first record events without binding");
    runtime.calls.clear();
    runtime.failed_animation.clear();
    state.words[2] = 0;
    runtime.hierarchy = 0;
    Check(!Load(state, runtime) && state.words[1] == 0x55u &&
              state.words[17] == 1 && state.words[10] == 0 &&
              state.words[3] == 0 &&
              runtime.calls.back() == "hierarchy",
          "missing hierarchy returns after records and state assignment");
}

void SuccessfulLoad() {
    Runtime runtime;
    pal4::GameObjectEmbeddedAnimationState state;
    state.words[2] = 0x204u;
    state.words[4] = 0x44u;
    state.words[5] = 0x55u;
    state.words[10] = 9;
    Check(Load(state, runtime) && state.words[2] == 0x1004u &&
              state.words[3] == 0x1004u && state.words[1] == 0x55u &&
              state.words[10] == 0 && state.words[17] == 1 &&
              state.words[4] == 0xa00u && state.words[5] == 0xb00u &&
              runtime.animations.at(0x1048u) == 0x502u,
          "successful load replaces records and initializes hierarchy");
    const auto old_destruct = std::find(runtime.calls.begin(),
                                         runtime.calls.end(), "old-destruct");
    const auto allocate = std::find(runtime.calls.begin(),
                                    runtime.calls.end(), "allocate");
    const auto flags = std::find(runtime.calls.begin(),
                                 runtime.calls.end(), "flags");
    const auto attach = std::find(runtime.calls.begin(),
                                  runtime.calls.end(), "attach");
    const auto atomics = std::find(runtime.calls.begin(),
                                   runtime.calls.end(), "atomics");
    const auto initialize = std::find(runtime.calls.begin(),
                                      runtime.calls.end(), "init-hierarchy");
    const auto matrices = std::find(runtime.calls.begin(),
                                    runtime.calls.end(), "matrices");
    const auto free_snapshot = std::find(runtime.calls.begin(),
                                         runtime.calls.end(), "free-snapshot:68");
    Check(old_destruct < allocate && flags < attach && attach < atomics &&
              atomics < initialize && matrices < free_snapshot &&
              runtime.snapshot_count == 2,
          "old array and snapshots are released around hierarchy setup");

    Runtime attached;
    attached.hierarchy_attached = true;
    pal4::GameObjectEmbeddedAnimationState attached_state;
    Check(Load(attached_state, attached) &&
              std::find(attached.calls.begin(), attached.calls.end(),
                        "flags") == attached.calls.end() &&
              std::find(attached.calls.begin(), attached.calls.end(),
                        "set-animation") != attached.calls.end(),
          "existing hierarchy skips attachment but selects animation");
}

}  // namespace

int main() {
    FailureBranches();
    SuccessfulLoad();
    std::cout << "game_object_animation_loader_tests passed\n";
}
