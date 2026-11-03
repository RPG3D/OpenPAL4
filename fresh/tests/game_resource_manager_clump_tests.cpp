#include "support/rw_dict_schema_fake.h"
#include "pal4/game_resource_manager.h"

#include <algorithm>
#include <bit>
#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace {

void Check(bool ok, const char* message) {
    if (!ok) { std::cerr << "FAIL: " << message << '\n'; std::exit(1); }
}

class Runtime final : public pal4::GameResourceManagerRuntimeStub {
public:
    std::uint32_t Allocate(std::uint32_t size) override {
        sizes.push_back(size);
        return sizes.size() == fail_at ? 0 : next++;
    }
    std::uint32_t Free(std::uint32_t address) override {
        events.push_back("free"); freed.push_back(address); return address + 1;
    }
    void StoreString(std::uint32_t address, std::string_view value) override {
        strings[address] = std::string(value);
    }
    std::string ReadString(std::uint32_t address) override { return strings[address]; }
    void CleanupCamera() override { events.push_back("shadow"); }
    std::uint32_t CreateLight(std::uint32_t type) override {
        Check(type == 1, "shadow light type"); return light_result;
    }
    std::uint32_t CreateFrame() override { return frame_result; }
    std::uint32_t LightFrame(std::uint32_t light) override {
        Check(light == 0x702, "shadow light frame"); return attached_frame;
    }
    void SetLightFrame(std::uint32_t light, std::uint32_t frame) override {
        Check(light == 0x702, "shadow light attach");
        attached_frame = frame;
    }
    void AddFrameChild(std::uint32_t parent, std::uint32_t child) override {
        Check(parent == 0x801 && child == 0x802, "shadow frame hierarchy");
        events.push_back("add_child");
    }
    void DestroyFrame(std::uint32_t frame) override {
        Check(frame == 0x802, "shadow frame destroy");
        events.push_back("destroy_frame");
    }
    void DestroyLight(std::uint32_t light) override {
        Check(light == 0x702, "shadow light destroy");
        events.push_back("destroy_light");
    }
    void DestroyIm3D(std::uint32_t value) override { im3d.push_back(value); }
    std::uint32_t ClumpWorld(std::uint32_t value) override {
        Check(value == 0x700 || value == 0x701, "world query clump"); return world;
    }
    void RemoveClump(std::uint32_t parent, std::uint32_t value) override {
        Check(parent == world && (value == 0x700 || value == 0x701),
              "world removal");
        events.push_back("remove_clump");
    }
    void DestroyClump(std::uint32_t value) override {
        Check(value == 0x700 || value == 0x701, "destroy clump");
        events.push_back("destroy_clump");
    }
    void RemoveTextureByName(std::uint32_t context, std::string_view path) override {
        Check(context == 0x900 &&
              (path == "res/model.dff" || path == "res/model_shadow.dff"),
              "texture cleanup path");
        events.push_back("remove_texture");
    }
    void ImageReallocateAndCopy(std::uint32_t address) override {
        Check(strings[address] == "res/", "image directory"); events.push_back("image");
    }
    std::uint32_t LoadWorldClump(std::string_view path, std::uint32_t mode,
                                 std::uint32_t context) override {
        Check((path == "res/model.dff" || path == "res/model_shadow.dff") &&
              mode == 0 && context == 0x900,
              "load arguments");
        events.push_back("load"); return load_result;
    }
    std::uint32_t SetClumpMaterialFlagsRaw(std::uint32_t clump,
                                           std::uint32_t value) override {
        Check(clump == 0x700 && value == 1, "flag mode and clump");
        events.push_back("flags"); return 0xFA;
    }
    std::uint32_t InitCamera(std::uint32_t* camera, std::uint32_t context,
                             std::uint32_t shadow, std::uint32_t light,
                             std::uint32_t* first,
                             std::uint32_t* second) override {
        events.push_back("camera_init");
        Check(context == 0x900 && shadow == 0x701 && light == 0x702 &&
              first == camera + 38 && second == camera + 42,
              "camera parameters and state offsets");
        return camera_result;
    }
    void RenderCameraScene(std::uint32_t* camera, float clear,
                            std::uint32_t* manager) override {
        events.push_back("camera_render");
        Check(camera == manager + 14 &&
              std::bit_cast<std::uint32_t>(clear) == 0x3dcccccdu,
              "camera render clear constant and state");
    }
    void ProcessAtomicBounds(std::uint32_t clump, std::uint32_t mode,
                             std::uint32_t* bbox) override {
        Check(clump == 0x700 && mode == bounds_calls.size() &&
              bbox == manager + (mode == 0 ? 60 : 66),
              "clump bounds traversal order");
        if (bounds_calls.empty()) events.push_back("bounds");
        bounds_calls.push_back(mode);
    }
    std::uint32_t ProcessAtomicBounds2(std::uint32_t clump,
        std::uint32_t first, std::uint32_t second,
        std::uint32_t* output) override {
        Check(clump == 0x700 && second == 1 &&
              output == manager + (first == 1 ? 56 : 52),
              "clump auxiliary bounds traversal");
        bounds_calls.push_back(first + 10);
        return clump;
    }
    std::uint32_t ReadGeometryWord(std::uint32_t address) override {
        Check(address == 0xA18 || address == 0xB08,
              "geometry flag read address");
        return address == 0xA18 ? 0xB00 : 0x100;
    }
    void WriteGeometryWord(std::uint32_t address,
                           std::uint32_t value) override {
        Check(address == 0xB08 && value == 0x145,
              "geometry flag update");
    }
    std::uint32_t ForAllGeometryFlagAtomics(std::uint32_t clump,
        std::uint32_t (*callback)(std::uint32_t, std::uint32_t,
                                   pal4::IRwGeometryFlagsRuntime&),
        std::uint32_t context) override {
        Check(clump == 0x700 && context == 0,
              "geometry flag traversal arguments");
        events.push_back("wrapper");
        Check(callback(0xA00, context, *this) == 0xA00,
              "geometry flag callback result");
        return clump;
    }
    void OptimizeClumpRaw(std::uint32_t clump,
                          std::uint32_t mode) override {
        Check(clump == 0x700 && mode == 0, "clump optimizer mode");
        events.push_back("optimize");
    }
    std::uint32_t ForAllOptimizerAtomics(std::uint32_t clump,
        std::uint32_t (*callback)(std::uint32_t, std::uint32_t,
                                   pal4::IRwClumpOptimizerRuntime&),
        std::uint32_t context) override {
        Check(clump == 0x700 && context == 0 && callback != nullptr,
              "optimizer traversal follows raw call");
        return clump;
    }
    bool CheckTexturePath(std::uint32_t value) override {
        Check(value == 0x700, "texture path clump"); return check_path;
    }
    std::uint32_t FirstMaterialAtomic(std::uint32_t value) override {
        Check(value == 0x700, "atomic clump"); return atomic;
    }
    std::string TexturePath(std::uint32_t value) override {
        Check(value == atomic, "texture path atomic"); return "tex/cloth.png";
    }
    std::string SplitFilename(std::string_view path) override {
        Check(path == "tex/cloth.png", "split path"); return "cloth";
    }
    std::uint32_t OpenDataStream(std::uint32_t type, std::uint32_t mode,
                                  std::string_view path) override {
        Check(type == 2 && mode == 1, "resource stream mode");
        if (path == "res/model_cloth.eff") {
            events.push_back("effect"); return 0;
        }
        Check(path == "res/extSize.scl", "scale stream path");
        events.push_back("scale"); return 0;
    }
    std::uint32_t ClumpFrame(std::uint32_t value) override {
        Check(value == 0x700 || value == 0x701, "clump frame");
        return value == 0x700 ? 0x800 : 0x801;
    }
    void FrameSetIdentity(std::uint32_t value) override {
        Check(value == 0x800, "frame identity"); events.push_back("identity");
    }
    void FrameRotate(std::uint32_t frame, std::uint32_t axis, float degrees,
                     std::uint32_t combine) override {
        Check((frame == 0x800 && combine == 2) ||
              (frame == 0x802 && combine <= 1), "rotate arguments");
        rotations.push_back({axis, degrees});
    }
    void FrameUpdateObjects(std::uint32_t value) override {
        if (manager) Check(manager[9]==0u, "resource stays unready until frame update completes");
        Check(value == 0x800, "frame update"); events.push_back("frame_update");
    }

    std::map<std::uint32_t, std::string> strings;
    std::vector<std::string> events;
    std::vector<std::uint32_t> sizes, freed, im3d;
    std::vector<std::uint32_t> bounds_calls;
    std::uint32_t* manager{};
    std::vector<std::pair<std::uint32_t, float>> rotations;
    std::uint32_t next{0x100}, load_result{0x700}, world{0x600}, atomic{0x500};
    std::uint32_t light_result{0x702}, frame_result{0x802}, attached_frame{};
    std::uint32_t camera_result{0xC00};
    std::size_t fail_at{};
    bool check_path{true};
};

class Schema final : public pal4::test::RwDictSchemaFake {
public:
    std::uint32_t SetCurrentDict(std::uint32_t value) override {
        current.push_back(value); return 0;
    }
    std::uint32_t OpenStream(std::uint32_t type, std::uint32_t mode,
                             const char* source) override {
        Check(type == 2 && mode == 1 && std::string(source) == "res/model.uva",
              "dictionary source"); return 0xA10;
    }
    bool FindChunk(std::uint32_t stream, std::uint32_t chunk) override {
        Check(stream == 0xA10 && chunk == 43, "dictionary chunk"); return true;
    }
    std::uint32_t ReadDict(std::uint32_t value) override {
        Check(value == 0xA10, "dictionary read"); return 0xA00;
    }
    std::uint32_t CreateList(std::uint32_t size, std::uint32_t hint) override {
        Check(size == 4 && hint == 0, "list allocation"); return 0xB00;
    }
    std::uint32_t ForAllAtomics(std::uint32_t clump,
        std::uint32_t (*)(std::uint32_t, std::uint32_t,
                          pal4::IRwDictSchemaRuntime&),
        std::uint32_t list) override {
        Check(clump == 0x700 && list == 0xB00, "dictionary clump traversal");
        return clump;
    }
    void DestroyList(std::uint32_t value) override { freed.push_back(value); }
    void DestroyDict(std::uint32_t value) override { destroyed.push_back(value); }
    std::vector<std::uint32_t> current, freed, destroyed;
};

void FailureBranches() {
    for (std::size_t fail = 1; fail <= 5; ++fail) {
        Runtime runtime;
        Schema schema;
        pal4::GameResourceManager manager;
        (void)manager.Construct();
        runtime.fail_at = fail;
        Check(manager.LoadClump("hero", "res/", "model", 0x900, 7, 8,
                                false, runtime, schema) == nullptr,
              "allocation failure result");
        Check(runtime.sizes.size() == (fail <= 3 ? 3u : 5u) &&
              runtime.events == std::vector<std::string>{"shadow"} &&
              manager.words[5] == 0 && manager.words[9] == 0,
              "allocation failure stops at original gate");
    }
    Runtime runtime;
    Schema schema;
    pal4::GameResourceManager manager;
    (void)manager.Construct();
    runtime.load_result = 0;
    Check(manager.LoadClump("hero", "res/", "model", 0x900, 7, 8,
                            false, runtime, schema) == nullptr &&
          manager.words[37] == 0xA00 && manager.words[5] == 0 &&
          runtime.freed.empty(), "world clump failure after dictionary load");
}

void SuccessAndCleanup() {
    Runtime runtime;
    Schema schema;
    pal4::GameResourceManager manager;
    (void)manager.Construct();
    runtime.manager = manager.words.data();
    Check(manager.LoadClump("hero", "res/", "model", 0x900, 7, 8,
                            true, runtime, schema) == &manager &&
          manager.words[5] == 0x700 && manager.words[6] == 7 &&
          manager.words[8] == 8 && manager.words[9] == 1 &&
          manager.words[37] == 0xA00 && manager.words[38] == 0xB00,
          "success manager fields");
    Check(runtime.sizes == std::vector<std::uint32_t>{5, 5, 6, 14, 14, 6, 20, 16} &&
          runtime.bounds_calls == std::vector<std::uint32_t>{0, 1, 11, 10} &&
          runtime.strings[manager.words[4]] == "cloth" &&
          runtime.freed == std::vector<std::uint32_t>{0x106, 0x107, 0x103, 0x104} &&
          runtime.rotations == std::vector<std::pair<std::uint32_t, float>>{
              {0x84b87cu, 0.0f}, {0x84b870u, 360.0f},
              {0x84b888u, 360.0f}},
          "paths, temporary lifetime and frame rotations");
    Check(runtime.events == std::vector<std::string>{
              "shadow", "image", "load", "flags", "bounds", "wrapper",
              "optimize", "effect", "free", "scale", "free", "free", "free", "identity",
              "frame_update"}, "success external call order");
    manager.words[74] = 0x740;
    manager.words[75] = 0x750;
    manager.words[90] = 0x44;
    Check(manager.CleanupClump(0x900, runtime, schema) == 0x44 &&
          manager.words[5] == 0 && manager.words[9] == 0 &&
          manager.words[37] == 0 && manager.words[38] == 0 &&
          manager.words[74] == 0 && manager.words[75] == 0,
          "cleanup clears owned state and returns config word 14");
    Check(schema.freed == std::vector<std::uint32_t>{0xB00} &&
          schema.destroyed == std::vector<std::uint32_t>{0xA00} &&
          runtime.im3d == std::vector<std::uint32_t>{0x740, 0x750} &&
          std::find(runtime.events.begin(), runtime.events.end(),
                    "remove_texture") != runtime.events.end(),
          "cleanup releases schema, Im3D and texture registration");
}

void OptionalPathBranches() {
    for (int branch = 0; branch < 2; ++branch) {
        Runtime runtime;
        Schema schema;
        pal4::GameResourceManager manager;
        (void)manager.Construct();
        runtime.manager = manager.words.data();
        runtime.check_path = branch != 0;
        runtime.atomic = 0;
        runtime.world = 0;
        Check(manager.LoadClump("hero", "res/", "model", 0x900, 7, 8,
                                false, runtime, schema) == &manager &&
              manager.words[4] == 0 &&
              std::find(runtime.events.begin(), runtime.events.end(),
                        "effect") == runtime.events.end(),
              "disabled texture/effect paths leave name empty");
        (void)manager.CleanupClump(0x900, runtime, schema);
        Check(std::find(runtime.events.begin(), runtime.events.end(),
                        "remove_clump") == runtime.events.end() &&
              std::find(runtime.events.begin(), runtime.events.end(),
                        "destroy_clump") != runtime.events.end(),
              "clump without world is destroyed without removal");
    }
}

void ShadowBranches() {
    {
        Runtime runtime;
        pal4::GameResourceManager manager;
        (void)manager.Construct();
        Check(!manager.LoadShadowClump(0x900, runtime) && runtime.events.empty(),
              "shadow load requires ready main clump");
    }
    for (int branch = 0; branch < 4; ++branch) {
        Runtime runtime;
        pal4::GameResourceManager manager;
        (void)manager.Construct();
        manager.words[2] = 0x201;
        manager.words[3] = 0x202;
        manager.words[9] = 1;
        runtime.strings[0x201] = "res/";
        runtime.strings[0x202] = "model";
        runtime.load_result = branch == 0 ? 0u : 0x701u;
        runtime.light_result = branch == 1 ? 0u : 0x702u;
        runtime.frame_result = branch == 2 ? 0u : 0x802u;
        Check(manager.LoadShadowClump(0x900, runtime) == (branch == 3) &&
              runtime.sizes == std::vector<std::uint32_t>{21} &&
              runtime.strings[0x100] == "res/model_shadow.dff" &&
              runtime.freed == std::vector<std::uint32_t>{0x100},
              "shadow path and load outcome");
        Check(manager.words[12] == (branch == 0 ? 0u : 0x701u) &&
              manager.words[13] == (branch == 3 ? 0x702u : 0u),
              "shadow clump and light failure state");
        if (branch == 2) {
            Check(std::find(runtime.events.begin(), runtime.events.end(),
                            "destroy_light") != runtime.events.end(),
                  "failed frame destroys light");
        }
        if (branch == 3) {
            Check(runtime.rotations ==
                  std::vector<std::pair<std::uint32_t, float>>{
                      {0x8c6ff0u, 86.900002f}, {0x8c6ffcu, 10.0f},
                      {0x8c7008u, 0.0f}} &&
                  runtime.attached_frame == 0x802,
                  "shadow light frame rotation and attachment");
            manager.words[11] = 1;
            Check(manager.CleanupShadowClump(0x900, runtime) &&
                  manager.words[11] == 0 && manager.words[12] == 0 &&
                  manager.words[13] == 0 && runtime.attached_frame == 0 &&
                  std::find(runtime.events.begin(), runtime.events.end(),
                            "destroy_frame") != runtime.events.end() &&
                  std::find(runtime.events.begin(), runtime.events.end(),
                            "remove_texture") != runtime.events.end(),
                  "shadow cleanup tears down light, frame, clump and texture");
        }
    }
}

void CameraAndFlags() {
    Runtime runtime;
    pal4::GameResourceManager manager;
    (void)manager.Construct();
    manager.words[11] = 0x55;
    Check(manager.RenderCamera(1, 0x900, runtime) == 0 &&
          manager.words[11] == 0x55 && runtime.events.empty(),
          "missing shadow clump gates camera without state changes");
    manager.words[12] = 0x701;
    Check(manager.RenderCamera(1, 0x900, runtime) == 0 &&
          manager.words[11] == 0x55 && runtime.events.empty(),
          "missing light gates camera");
    manager.words[13] = 0x702;
    Check(manager.RenderCamera(0, 0x900, runtime) == 0 &&
          manager.words[11] == 0 &&
          runtime.events == std::vector<std::string>{"shadow"},
          "disabled camera records flag and cleans camera");
    runtime.events.clear();
    runtime.camera_result = 0;
    Check(manager.RenderCamera(1, 0x900, runtime) == 0 &&
          runtime.events == std::vector<std::string>{"shadow", "camera_init"},
          "failed camera initialization skips render");
    runtime.events.clear();
    runtime.camera_result = 0xC00;
    Check(manager.RenderCamera(1, 0x900, runtime) == 0xC00 &&
          runtime.events == std::vector<std::string>{
              "shadow", "camera_init", "camera_render"},
          "successful camera initializes then renders");
    Check(manager.CleanupCamera(runtime) &&
          runtime.events.back() == "shadow", "cleanup camera forwards and succeeds");

    runtime.events.clear();
    Check(manager.SetClumpMaterialFlags(1, runtime) == 0 &&
          runtime.events.empty(), "material flags skip absent clump");
    manager.words[5] = 0x700;
    Check(manager.SetClumpMaterialFlags(1, runtime) == 0xFA &&
          runtime.events == std::vector<std::string>{"flags"},
          "material flags forward clump and result");
}

}  // namespace

int main() {
    FailureBranches();
    SuccessAndCleanup();
    OptionalPathBranches();
    ShadowBranches();
    CameraAndFlags();
}
