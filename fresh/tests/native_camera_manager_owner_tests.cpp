#include "pal4/native_camera_manager_owner.h"
#include "pal4/native_game_save_archive_reader.h"
#include "pal4/native_game_save_archive_buffer.h"

#include <cstdint>
#include <bit>
#include <iostream>
#include <stdexcept>
#include <string_view>
#include <vector>
#include <fstream>
#include <process.h>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

void ControllerLifetime() {
    pal4::NativeCameraManagerOwner owner;
    Check(owner.Construct() == owner.Address() && owner.Constructed(),
          "camera manager owns its controller address");
    auto& state = owner.State();
    Check(state.At(0x4u) == 0u && state.At(0x8u) == 0u &&
              state.At(0xcu) == 0u && state.At(0x30u) == 0u &&
              state.At(0x34u) == 0u && state.At(0x38u) == 0u &&
              state.At(0x3cu) == 0u && state.At(0x40u) == 0u &&
              state.At(0x10u) == 0x5eaec0u &&
              state.At(0x14u) == 0x5eaea0u &&
              state.At(0x18u) == 0x5eaf50u &&
              state.At(0x1cu) == 0x5fc6c0u &&
              state.At(0x2cu) != 0u &&
              std::string_view(reinterpret_cast<const char*>(
                  static_cast<std::uintptr_t>(state.At(0x2cu)))) ==
                  "palCameraController set",
          "camera controller uses the IDA startup layout");
    bool rejected = false;
    try {
        (void)owner.Construct();
    } catch (const std::logic_error&) {
        rejected = true;
    }
    Check(rejected, "camera manager rejects double construction");
    owner.Destroy();
    Check(!owner.Constructed() && state.At(0x2cu) == 0u &&
              state.At(0x30u) == 0u,
          "empty camera controller is released before SDK shutdown");
    owner.Destroy();
}

class Resources final : public pal4::INativeCameraObjectResources {
public:
    int fail_index = -1;
    int initialized = 0;
    int matrix_updates = 0;
    std::uint32_t last_solve_eye = 0u;
    std::uint32_t last_far_clip_camera = 0u;
    float last_far_clip = -1.0f;
    std::vector<int> destroyed;

    pal4::CameraRenderRect RenderRect() override {
        return {0, 0, 640, 480};
    }
    void LogInitialization(bool) override {}
    bool Initialize(pal4::CameraSystemObjectState& object,
                    pal4::CameraDataState& data,
                    const pal4::CameraSetupParameters& parameters) override {
        constexpr std::array<std::string_view, 4> names{
            "MainCamera", "ScriptCamera", "CombatCamera", "UiCamera"};
        const int index = initialized++;
        Check(index < 4 && parameters.name == names[index] &&
                  parameters.viewport == pal4::CameraRect{0, 0, 640, 480} &&
                  object.camera == reinterpret_cast<std::uintptr_t>(&data),
              "camera owner forwards the original initializer arguments");
        data.At(0u) = 0x1000u + static_cast<std::uint32_t>(index);
        return index != fail_index;
    }
    void Destroy(pal4::CameraSystemObjectState& object,
                 pal4::CameraDataState& data) noexcept override {
        destroyed.push_back(static_cast<int>(data.At(0u) - 0x1000u));
        object.camera = 0u;
        data.At(0u) = 0u;
    }
    void SetFarClip(std::uint32_t camera, float distance) override {
        last_far_clip_camera = camera;
        last_far_clip = distance;
    }
    float MinimumDistance() override { return 1.0f; }
    float MaximumDistance() override { return 1000.0f; }
    std::uint32_t UpdateMatrix(pal4::CameraDataState&,
                               std::uint32_t solve_eye) override {
        ++matrix_updates;
        last_solve_eye = solve_eye;
        return 1u;
    }
};

class SceneRuntime final : public pal4::IBaseObjectSceneParameterRuntime {
public:
    std::uint32_t clump{};
    std::array<float, 3> ClumpPosition(std::uint32_t value) override {
        clump = value;
        return {10.0f, 20.0f, 30.0f};
    }
};

void CameraObjectLifetime() {
    pal4::NativeCameraManagerOwner owner;
    (void)owner.Construct();
    Resources resources;
    Check(owner.InitializeCameras(resources) && owner.CamerasReady() &&
              resources.initialized == 4 &&
              owner.State().At(0x30u) == owner.State().At(0x34u) &&
              owner.State().At(0x34u) != owner.State().At(0x38u) &&
              owner.State().At(0x38u) != owner.State().At(0x3cu) &&
              owner.State().At(0x3cu) != owner.State().At(0x40u) &&
              owner.State().At(0xbcu) == owner.Address() + 0x44u &&
              owner.Data(0u).At(0x2bcu) == 0x5ec270u,
          "startup initializer owns four named camera objects and data");
    Check(owner.FindByName("MainCamera") == owner.State().At(0x34u) &&
              owner.FindByName("UiCamera") == owner.State().At(0x40u) &&
              owner.ActivateByName("ScriptCamera") &&
              owner.State().At(0x30u) == owner.State().At(0x38u) &&
              owner.ActivateByName("MainCamera") &&
              owner.State().At(0x30u) == owner.State().At(0x34u),
          "camera owner reuses fixed-name lookup and active selection");
    Check(owner.SetActiveFarClip(0.0f) &&
              resources.last_far_clip_camera == owner.Data(0u).At(0u) &&
              resources.last_far_clip == 0.0f,
          "scene far clip forwards the original zero value");
    pal4::BaseObjectState actor;
    actor.Write(0x14u, 0x12340000u);
    actor.Write(0xecu, std::bit_cast<std::uint32_t>(5.0f));
    SceneRuntime scene;
    Check(owner.BindMainCameraToActor(actor, scene) &&
              scene.clump == 0x12340000u &&
              resources.matrix_updates == 1 &&
              resources.last_solve_eye == 1u &&
              owner.Data(0u).At(0x34u) ==
                  reinterpret_cast<std::uintptr_t>(&actor) + 0x1ecu &&
              owner.Data(0u).At(0x74u) ==
                  std::bit_cast<std::uint32_t>(10.0f) &&
              owner.Data(0u).At(0x78u) ==
                  std::bit_cast<std::uint32_t>(29.0f) &&
              owner.Data(0u).At(0x7cu) ==
                  std::bit_cast<std::uint32_t>(30.0f),
          "main camera binds player scene parameter and updates look-at");
    owner.ClearMainCameraSceneParameter();
    Check(owner.Data(0u).At(0x34u) == 0u,
          "camera releases the player scene parameter before player teardown");
    const auto matrix_updates_before_yaw = resources.matrix_updates;
    Check(owner.SetMainCameraYaw(-10.0f, 1u) == 1u &&
              owner.Data(0u).At(0x3cu) ==
                  std::bit_cast<std::uint32_t>(350.0f) &&
              resources.matrix_updates == matrix_updates_before_yaw + 1 &&
              resources.last_solve_eye == 1u,
          "MainCamera tail yaw reuses angle normalization and matrix update");
    const auto matrix_updates_before_drag = resources.matrix_updates;
    Check(owner.SetMainCameraPitch(-20.0f, 1u) == 1u &&
              owner.Data(0u).At(0x40u) ==
                  std::bit_cast<std::uint32_t>(340.0f) &&
              owner.AdjustMainCameraAngles(25.0f, 30.0f, 1u) == 1u &&
              owner.Data(0u).At(0x3cu) ==
                  std::bit_cast<std::uint32_t>(15.0f) &&
              owner.Data(0u).At(0x40u) ==
                  std::bit_cast<std::uint32_t>(10.0f) &&
              resources.matrix_updates == matrix_updates_before_drag + 2 &&
              resources.last_solve_eye == 1u,
          "MainCamera drag reuses pitch and combined-angle setters");
    for (std::uint32_t mode = 0u; mode < 6u; ++mode) {
        Check(owner.SetModeByIndex(mode) &&
                  (owner.CameraFollowValue() != 0.0f) ==
                      (mode == 0u || mode == 5u),
              "follow gate reads the selected original mode table entry");
    }
    const auto selected_mode = owner.State().At(0xbcu);
    owner.State().At(0xbcu) = 0u;
    Check(owner.CameraFollowValue() == 0.0f,
          "missing mode table disables the original follow gate");
    owner.State().At(0xbcu) = selected_mode + 4u;
    bool bad_mode_rejected = false;
    try { (void)owner.CameraFollowValue(); }
    catch (const std::invalid_argument&) { bad_mode_rejected = true; }
    Check(bad_mode_rejected, "follow gate rejects an unowned mode table entry");
    owner.State().At(0xbcu) = selected_mode;
    bool rejected = false;
    try {
        (void)owner.InitializeCameras(resources);
    } catch (const std::logic_error&) {
        rejected = true;
    }
    Check(rejected, "camera resources cannot initialize twice");
    owner.Destroy();
    Check(resources.destroyed == std::vector<int>{3, 2, 1, 0} &&
              !owner.CamerasReady() && owner.State().At(0x34u) == 0u &&
              owner.State().At(0xbcu) == 0u,
          "four cameras release in reverse order before controller cleanup");
}

void CameraArchiveInput() {
    const auto root = std::filesystem::path("H:/PAL4/objs/camera-input") / std::to_string(_getpid());
    std::filesystem::create_directories(root / "save");
    pal4::NativeGameSaveArchiveReader input;
    auto write = [&](const pal4::BinaryXmlNode& node, std::uint8_t major = 1u, bool truncate = false) {
        input.Close();
        pal4::NativeGameSaveArchiveBuffer buffer;
        auto xml = pal4::BinaryXmlConfig::Serialize(node);
        Check(xml.has_value(), "serialize camera fixture XML");
        (*xml)[0] = major;
        // Padding satisfies archive size checks and verifies nonzero entry cursor.
        std::array<std::byte, sizeof(pal4::GameSaveHeader)> padding{};
        Check(buffer.WriteBytes(padding) && buffer.WriteBytes(std::as_bytes(
            std::span(xml->data(), truncate ? 7u : xml->size()))) &&
            buffer.WriteMarker("AFTER_CAMERA"), "write continuous camera fixture");
        { std::ofstream file(root / "save/0.dat", std::ios::binary);
          file.write(reinterpret_cast<const char*>(buffer.Bytes().data()), buffer.Bytes().size());
          Check(static_cast<bool>(file), "save camera input fixture"); }
        Check(input.OpenSlot(root, 0) && input.ReadBytes(padding), "open camera at nonzero archive cursor");
    };
    Resources resources;
    pal4::NativeCameraManagerOwner owner;
    (void)owner.Construct();
    pal4::BinaryXmlNode node{"CAM_MGR"};
    node.attributes = {{"current-camera-mode", 3u, std::string("2")},
        {"current-camera-PIT", 2u, -15.0f}, {"current-camera-YAW", 1u, std::int32_t{370}}};
    write(node);
    bool rejected = false;
    try { (void)owner.LoadConfig(input); } catch (const std::logic_error&) { rejected = true; }
    Check(rejected && input.Position() == sizeof(pal4::GameSaveHeader),
          "uninitialized cameras rejected before archive consumption");
    Check(owner.InitializeCameras(resources), "initialize cameras for archive input");
    owner.ConfigureSceneModes();
    const auto before = resources.matrix_updates;
    Check(owner.LoadConfig(input) && owner.State().At(0xc0u) == 2u &&
        owner.State().At(0xbcu) == owner.Address() + 0x44u + 40u &&
        owner.Data(0).At(0x3cu) == std::bit_cast<std::uint32_t>(10.0f) &&
        owner.Data(0).At(0x40u) == std::bit_cast<std::uint32_t>(345.0f) &&
        owner.Data(0).At(0x44u) == 0u && resources.matrix_updates == before + 7 &&
        resources.last_solve_eye == 1u && input.ReadMarker() == "AFTER_CAMERA",
        "camera archive applies mode and normalized angles through real shared setters");
    write(pal4::BinaryXmlNode{"CAM_MGR"});
    Check(owner.LoadConfig(input) && owner.State().At(0xc0u) == 0u &&
        owner.Data(0).At(0x3cu) == owner.State().At(0x44u) &&
        owner.Data(0).At(0x40u) == owner.State().At(0x48u),
        "missing camera attributes use mode0 and selected table defaults");
    // Original stores an invalid mode but SetModeByIndex preserves its old table.
    auto bad_mode = node;
    bad_mode.attributes[0] = {"current-camera-mode", 1u, std::int32_t{-1}};
    const auto selected = owner.State().At(0xbcu);
    write(bad_mode);
    Check(owner.LoadConfig(input) && owner.State().At(0xc0u) == 0xffffffffu &&
        owner.State().At(0xbcu) == selected, "invalid mode preserves the selected table as original");
    for (unsigned failure = 0; failure < 4u; ++failure) {
        write(failure == 0u ? pal4::BinaryXmlNode{"WRONG"} : node,
              failure == 1u ? 2u : (failure == 2u ? 0u : 1u), failure == 3u);
        const auto state = owner.State(); const auto camera = owner.Data(0u);
        const auto updates = resources.matrix_updates;
        Check(!owner.LoadConfig(input) && owner.State().words == state.words &&
            owner.Data(0u).words == camera.words && resources.matrix_updates == updates,
            "bad camera root/version/truncation leaves camera and mode unchanged");
    }
    input.Close(); rejected = false;
    try { (void)owner.LoadConfig(input); } catch (const std::logic_error&) { rejected = true; }
    Check(rejected, "closed camera archive rejected");
    owner.Destroy();
    std::filesystem::remove_all(root);
}

void CameraArchiveOutput() {
    const auto root=std::filesystem::path("H:/PAL4/objs/camera-output")/std::to_string(_getpid());
    std::filesystem::create_directories(root/"save");
    Resources resources;
    pal4::NativeCameraManagerOwner owner;
    (void)owner.Construct();
    pal4::NativeGameSaveArchiveBuffer output;
    bool rejected=false;
    try{(void)owner.SaveConfig(output);}catch(const std::logic_error&){rejected=true;}
    Check(rejected && output.Bytes().empty(),"uninitialized camera save rejects before changing output");
    Check(owner.InitializeCameras(resources) && owner.SetModeByIndex(2u) &&
              owner.SetMainCameraYaw(123.0f,1u) && owner.SetMainCameraPitch(345.0f,1u) &&
              owner.SetCameraYaw(owner.Object(1).camera,56.0f,1u) &&
              owner.SetCameraPitch(owner.Object(1).camera,78.0f,1u) &&
              owner.ActivateByName("ScriptCamera"),"prepare distinct Main and active Script cameras");
    const auto state=owner.State();
    const std::array cameras{owner.Data(0),owner.Data(1),owner.Data(2),owner.Data(3)};
    const auto updates=resources.matrix_updates;
    Check(output.WritePrefix("CAMERA_SAVE_TEST",{}) && owner.SaveConfig(output) &&
              output.WriteMarker("AFTER_CAMERA") && owner.SaveConfig(output),
        "camera owner appends repeated shared CAM_MGR documents to continuous archive");
    Check(owner.State().words==state.words && resources.matrix_updates==updates,
        "saving preserves active camera, mode table and matrix update count");
    for(unsigned i=0;i<4u;++i)Check(owner.Data(i).words==cameras[i].words,
        "saving preserves every camera image");
    {
        std::ofstream file(root/"save/0.dat",std::ios::binary);
        file.write(reinterpret_cast<const char*>(output.Bytes().data()),output.Bytes().size());
        Check(static_cast<bool>(file),"write camera archive under objs");
    }
    pal4::NativeGameSaveArchiveReader input;
    pal4::GameSaveHeader header;
    pal4::BinaryXmlConfig xml;
    Check(input.OpenSlot(root,0) && input.ReadMarker()=="CAMERA_SAVE_TEST" && input.ReadHeader(header) &&
              input.ReadConfig(xml) && xml.Root().name=="CAM_MGR" &&
              xml.MajorVersion()==1u && xml.MinorVersion()==1u && xml.Root().attributes.size()==3u,
        "native camera save writes original versioned CAM_MGR shape");
    const auto* mode=xml.Root().FindAttribute("current-camera-mode");
    const auto* yaw=xml.Root().FindAttribute("current-camera-YAW");
    const auto* pitch=xml.Root().FindAttribute("current-camera-PIT");
    Check(mode && yaw && pitch && mode->type==1u && mode->ToInt(0u)==2u &&
              yaw->type==2u && yaw->ToFloat(0.0f)==123.0f &&
              pitch->type==2u && pitch->ToFloat(0.0f)==345.0f && input.ReadMarker()=="AFTER_CAMERA",
        "save uses Main yaw/pitch rather than active Script values and preserves following archive marker");
    Check(owner.SetModeByIndex(0u) && owner.SetMainCameraYaw(5.0f,1u) &&
              owner.SetMainCameraPitch(6.0f,1u) && owner.LoadConfig(input) && !input.Remaining() &&
              owner.State().At(0xc0u)==2u &&
              owner.Data(0).At(0x3cu)==std::bit_cast<std::uint32_t>(123.0f) &&
              owner.Data(0).At(0x40u)==std::bit_cast<std::uint32_t>(345.0f),
        "existing native reader restores saved nondefault mode and Main angles from next document");
    owner.Destroy();
    const auto previous=output.Bytes();rejected=false;
    try{(void)owner.SaveConfig(output);}catch(const std::logic_error&){rejected=true;}
    Check(rejected && output.Bytes()==previous,"retired camera owner rejects save without appending bytes");
    input.Close();std::filesystem::remove_all(root);
    std::cout<<"Camera archive output: Main vs active Script, nonmutating continuous write, version/types and native roundtrip pass\n";
}

void SceneCameraConfiguration() {
    pal4::NativeCameraManagerOwner owner;
    (void)owner.Construct();
    Resources resources;
    Check(owner.InitializeCameras(resources) && owner.SetModeByIndex(2u) &&
              owner.ActivateByName("ScriptCamera"), "prepare nondefault scene cameras");
    (void)owner.SetCameraTarget(owner.Object(1u).camera, 5.0f, 6.0f, 7.0f, 1u);
    (void)owner.SetCameraAngles(owner.Object(1u).camera, {120.0f, 20.0f, 30.0f}, 1u);
    Check(owner.AddSceneClump(0x1234u) && owner.AddSceneClump(0x5678u),
          "scene cameras retain borrowed obstacles before configuration");
    auto* script = reinterpret_cast<pal4::CameraSystemObjectState*>(
        static_cast<std::uintptr_t>(owner.State().At(0x38u)));
    script->tasks[2] = 1u;
    const auto manager_before = owner.State();
    const auto camera_before = owner.Data(0u);
    const auto updates_before = resources.matrix_updates;
    bool rejected = false;
    try { owner.ConfigureSceneModes(); }
    catch (const std::logic_error&) { rejected = true; }
    Check(rejected && owner.State().words == manager_before.words &&
              owner.Data(0u).words == camera_before.words &&
              resources.matrix_updates == updates_before,
          "live camera tasks reject configuration before changing camera state");
    script->tasks[2] = 0u;
    const auto combat = owner.Data(2u);
    const auto ui = owner.Data(3u);
    const auto buffer = owner.State().At(4u);
    const auto capacity = owner.State().At(12u);
    owner.ConfigureSceneModes();
    Check(owner.State().At(0x30u) == owner.State().At(0x34u) &&
              owner.State().At(0xc0u) == 0u &&
              owner.State().At(0xbcu) == owner.Address() + 0x44u &&
              owner.SceneClumpCount() == 0u && owner.State().At(4u) == buffer &&
              owner.State().At(12u) == capacity &&
              owner.Data(2u).words == combat.words && owner.Data(3u).words == ui.words,
          "scene configuration selects Main mode0, retires obstacle references and preserves other cameras");
    for (const auto i : {0u, 1u})
        Check(owner.Data(i).At(0x3cu) == owner.State().At(0x44u) &&
                  owner.Data(i).At(0x40u) == owner.State().At(0x48u) &&
                  owner.Data(i).At(0x44u) == 0u &&
                  owner.Data(i).At(0x4cu) == owner.State().At(0x4cu) &&
                  owner.Data(i).At(0x74u) == 0u &&
                  owner.Data(i).At(0x78u) == 0u && owner.Data(i).At(0x7cu) == 0u &&
                  owner.Data(i).At(0x1a8u) == owner.State().At(0x50u) &&
                  (owner.Object(i).tasks[3] & 0xffu) == 1u,
              "Main and Script camera parameters and task-ready state use original mode0 configuration");
    owner.ConfigureSceneModes();
    owner.Destroy();
}

void CameraSetupFailure() {
    pal4::NativeCameraManagerOwner owner;
    (void)owner.Construct();
    Resources failing;
    failing.fail_index = 1;
    Check(!owner.InitializeCameras(failing) && !owner.CamerasReady() &&
              failing.initialized == 4 &&
              failing.destroyed == std::vector<int>{3, 2, 1, 0} &&
              owner.State().At(0x30u) == 0u,
          "earlier camera setup failure rolls back every object");
    Resources retry;
    Check(owner.InitializeCameras(retry) && owner.CamerasReady(),
          "camera manager can initialize after a cleaned failure");
    owner.Destroy();
}

void ScenePropertyReset() {
    pal4::NativeCameraManagerOwner owner;
    (void)owner.Construct();
    bool rejected = false;
    try { owner.ResetSceneProperties(); }
    catch (const std::logic_error&) { rejected = true; }
    Check(rejected, "uninitialized camera property reset is rejected");
    Resources resources;
    Check(owner.InitializeCameras(resources) &&
              owner.ActivateByName("ScriptCamera"),
          "select script camera before property reset");
    auto& state = owner.State();
    // An unbound populated collection must never silently lose its root.
    state.At(0x20u) = 0x1234u;
    state.At(0x24u) = 1u;
    const auto before = state;
    rejected = false;
    try { owner.ResetSceneProperties(); }
    catch (const std::logic_error&) { rejected = true; }
    Check(rejected && state.words == before.words,
          "unbound named cameras reject reset before active camera or tree mutation");
    state.At(0x20u) = state.At(0x24u) = 0u;
    Check(owner.AddSceneClump(0x5678u) && owner.SetModeByIndex(2u),
          "prepare unrelated scene camera state");
    const auto configured = state;
    const auto main_data = owner.Data(0u);
    const auto script_data = owner.Data(1u);
    const auto updates = resources.matrix_updates;
    for (unsigned repeat = 0u; repeat < 2u; ++repeat) {
        owner.ResetSceneProperties();
        auto expected = configured;
        expected.At(0x30u) = expected.At(0x34u);
        Check(state.words == expected.words && owner.SceneClumpCount() == 1u &&
                  owner.Data(0u).words == main_data.words &&
                  owner.Data(1u).words == script_data.words &&
                  resources.matrix_updates == updates && resources.destroyed.empty(),
              "property reset restores MainCamera without mode, obstacle or fixed-camera cleanup");
    }
    Check(owner.RemoveSceneClump(0x5678u), "release reset fixture obstacle");
    owner.Destroy();
}

}  // namespace

int main() {
    try {
        ControllerLifetime();
        CameraObjectLifetime();
        CameraSetupFailure();
        CameraArchiveInput();
        CameraArchiveOutput();
        SceneCameraConfiguration();
        ScenePropertyReset();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
