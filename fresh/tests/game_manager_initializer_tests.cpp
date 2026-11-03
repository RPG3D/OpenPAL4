#include "pal4/game_manager_initializer.h"

#include <array>
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

class Runtime final : public pal4::IGameManagerInitRuntime {
public:
    void ResetExisting(pal4::ScreenEffectDispatcherState& state) override {
        events.push_back("reset");
        state.enabled = 0u;
        state.active = -4;
    }
    std::uint32_t AllocateCameraRecord() override {
        events.push_back("camera allocate");
        return camera_address;
    }
    bool InitializeCamera(const std::uint32_t record) override {
        Check(record == camera_address, "camera initializer receives record");
        events.push_back("camera initialize");
        return camera_success;
    }
    void DestroyCameraRecord(const std::uint32_t record) override {
        Check(record == camera_address, "failed camera record is destroyed");
        events.push_back("camera destroy");
    }
    std::uint32_t AllocateObject(const std::uint32_t size) override {
        const auto key = static_cast<std::uint32_t>(objects.size() + 1u);
        objects.push_back({key, size, {}, 0u});
        events.push_back("allocate " + std::to_string(key));
        return key == failed_key ? 0u : 0x1000u + key * 0x100u;
    }
    void InitializeObject(const std::uint32_t address,
                          const pal4::ManagerObjectImage& image) override {
        Check(address == 0x1000u + objects.size() * 0x100u,
              "object writes target allocated address");
        objects.back().image = image;
        ++objects.back().initializations;
        events.push_back("initialize " + std::to_string(objects.size()));
    }
    void MakePair(const std::uint32_t key,
                  const std::uint32_t address) override {
        Check(key == objects.size() && key < 14u &&
                  address == (key == failed_key ? 0u : 0x1000u + key * 0x100u),
              "pair construction uses current key and pointer");
        events.push_back("pair " + std::to_string(key));
    }
    void InsertManager(const std::uint32_t key,
                       const std::uint32_t address) override {
        Check(key == objects.size() &&
                  address == (key == failed_key ? 0u : 0x1000u + key * 0x100u),
              "tree insertion uses current key and pointer");
        events.push_back("insert " + std::to_string(key));
    }
    void SetNodeData(const std::uint32_t key) override {
        Check(key == objects.size(), "node-data call uses insertion result");
        events.push_back("node " + std::to_string(key));
    }

    struct Object {
        std::uint32_t key;
        std::uint32_t size;
        pal4::ManagerObjectImage image;
        std::uint32_t initializations;
    };
    std::uint32_t camera_address{0x5000u};
    bool camera_success{true};
    std::uint32_t failed_key{};
    std::vector<Object> objects;
    std::vector<std::string> events;
};

pal4::ScreenEffectDispatcherState InitialState() {
    pal4::ScreenEffectDispatcherState state{};
    state.enabled = 1u;
    state.active = 7;
    state.current_key = 29;
    state.current_handler = 0x2200u;
    state.frame_state = 0x3300u;
    state.tree_field = 0x4400u;
    state.tree_sentinel = 0x5500u;
    return state;
}

void CameraFailureStopsBeforeManagers() {
    auto state = InitialState();
    Runtime runtime;
    runtime.camera_success = false;
    Check(!pal4::GameManagerInitializer::Initialize(state, 0xabu, runtime),
          "failed camera returns false");
    Check(runtime.events == std::vector<std::string>({
              "reset", "camera allocate", "camera initialize", "camera destroy"}),
          "camera failure cleans up without manager allocation");
    Check(state.frame_state == 0u && state.enabled == 0u &&
              state.active == -4 && state.current_key == 29,
          "failure only applies reset effects and clears camera field");
    runtime = Runtime{};
    runtime.camera_address = 0u;
    runtime.camera_success = false;
    state = InitialState();
    Check(!pal4::GameManagerInitializer::Initialize(state, 0u, runtime),
          "null camera record also follows camera failure branch");
    Check(runtime.events.back() == "camera destroy",
          "camera destructor call is made even for null record");
}

void FullInitializationPreservesOrderAndLayouts() {
    auto state = InitialState();
    Runtime runtime;
    Check(pal4::GameManagerInitializer::Initialize(state, 0xabu, runtime),
          "camera success completes manager initialization");
    Check(runtime.objects.size() == 15u && state.frame_state == 0x5000u &&
              state.enabled == 1u && state.active == 0 &&
              state.current_key == 29 && state.current_handler == 0x2200u &&
              state.tree_field == 0x4400u && state.tree_sentinel == 0x5500u,
          "all entries are inserted and unrelated state is preserved");
    std::vector<std::string> expected{
        "reset", "camera allocate", "camera initialize"};
    for (std::uint32_t key = 1; key <= 15u; ++key) {
        const auto text = std::to_string(key);
        expected.push_back("allocate " + text);
        expected.push_back("initialize " + text);
        if (key < 14u) expected.push_back("pair " + text);
        expected.push_back("insert " + text);
        expected.push_back("node " + text);
    }
    Check(runtime.events == expected, "15 insertions preserve exact call order");

    constexpr std::array<std::uint32_t, 15> vtables{
        0x842540u, 0x842500u, 0x8424c0u, 0x842480u, 0x842440u,
        0x8423c0u, 0x842380u, 0x842340u, 0x842300u, 0x8422c0u,
        0x842280u, 0x842240u, 0x842200u, 0x8421c0u, 0x842180u};
    for (std::size_t index = 0; index < runtime.objects.size(); ++index) {
        const auto key = index + 1u;
        const auto& object = runtime.objects[index];
        const auto& image = object.image;
        Check(object.key == key && object.initializations == 1u &&
                  object.size == (key == 6u ? 48u :
                                  (key == 8u || key == 14u ? 16u : 12u)),
              "allocation size matches each key");
        Check(image.size == object.size &&
                  image.Dword(0u) == vtables[index] &&
                  image.final_vtable == vtables[index] &&
                  image.base_vtable == (key == 6u ? 0x842400u : vtables[index]) &&
                  image.HasWrite(4u) && image.HasWrite(5u) &&
                  image.bytes[4] == 0u && image.bytes[5] == 0u,
              "vtable and common flag writes match IDA");
        Check(image.IsInitialized() == 0u && image.IsEnabled() == 0u,
              "manager virtual flag accessors read bytes at +4 and +5");
        Check(image.HasWrite(8u) == (key != 14u) &&
                  image.HasWrite(12u) == (key == 6u || key == 8u || key == 14u),
              "unwritten dwords are distinguished from zeroed dwords");
    }
    const auto& extended = runtime.objects[5].image;
    Check(extended.HasWrite(16u) && extended.HasWrite(24u) &&
              !extended.HasWrite(20u) && !extended.HasWrite(28u) &&
              extended.HasWrite(32u) && extended.bytes[32] == 0xabu &&
              !extended.HasWrite(33u) && extended.HasWrite(36u) &&
              extended.HasWrite(40u) && extended.HasWrite(44u),
          "key 6 preserves sparse 48-byte writes and explicit stack byte");
    auto mutable_image = runtime.objects[0].image;
    mutable_image.bytes[4] = 0x7fu;
    mutable_image.bytes[5] = 0xa5u;
    Check(mutable_image.IsInitialized() == 0x7fu &&
              mutable_image.IsEnabled() == 0xa5u,
          "accessors return full byte values, not normalized booleans");
}

void AllocationFailuresStillInsertNull() {
    for (std::uint32_t failed = 1u; failed <= 15u; ++failed) {
        auto state = InitialState();
        Runtime runtime;
        runtime.failed_key = failed;
        Check(pal4::GameManagerInitializer::Initialize(state, 0u, runtime),
              "object allocation failure does not fail outer initializer");
        Check(runtime.objects.size() == 15u &&
                  runtime.objects[failed - 1u].initializations == 0u &&
                  runtime.events.back() == "node 15",
              "null entry is inserted and later entries still run");
    }
}

void LinkableStubFailsAtCameraBoundary() {
    auto state = InitialState();
    pal4::GameManagerInitRuntimeStub stub;
    Check(!pal4::GameManagerInitializer::Initialize(state, 0u, stub) &&
              state.frame_state == 0u,
          "empty runtime links and exits through camera failure");
}

void ExtendedTailWritesOnlyObservedBytes() {
    pal4::ManagerObjectImage image{};
    image.bytes.fill(0xccu);
    Check(&image.InitializeExtendedTail(0x5au) == &image,
          "extended-tail helper returns its destination");
    for (std::size_t offset = 0; offset < image.bytes.size(); ++offset) {
        const bool expected_write = offset == 32u || offset >= 36u;
        Check(image.HasWrite(offset) == expected_write,
              "extended-tail write mask matches helper instructions");
        Check(image.bytes[offset] ==
                  (offset == 32u ? 0x5au :
                   expected_write ? 0u : 0xccu),
              "extended-tail helper preserves every unwritten byte");
    }
}

void BaseConstructorWritesOnlyVtable() {
    pal4::ManagerObjectImage image{};
    image.bytes.fill(0xccu);
    image.InstallBaseVtable();
    Check(image.base_vtable == 0x842400u &&
              image.Dword(0u) == 0x842400u,
          "base manager constructor installs observed vtable");
    for (std::size_t offset = 0u; offset < image.bytes.size(); ++offset)
        Check(image.HasWrite(offset) == (offset < 4u) &&
                  image.bytes[offset] ==
                      (offset < 4u ?
                           static_cast<std::uint8_t>(0x842400u >> (offset * 8u)) :
                           0xccu),
              "base constructor changes only the first DWORD");
}

}  // namespace

int main() {
    CameraFailureStopsBeforeManagers();
    FullInitializationPreservesOrderAndLayouts();
    AllocationFailuresStillInsertNull();
    LinkableStubFailsAtCameraBoundary();
    ExtendedTailWritesOnlyObservedBytes();
    BaseConstructorWritesOnlyVtable();
    std::cout << "6 game manager initializer test cases passed\n";
}
