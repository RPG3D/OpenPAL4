#include "pal4/game_manager_initializer.h"

#include <array>

namespace pal4 {
namespace {

struct ManagerSpec {
    std::uint32_t size;
    std::uint32_t vtable;
};

constexpr std::array<ManagerSpec, 15> kManagerSpecs{{
    {12u, 0x842540u}, {12u, 0x842500u}, {12u, 0x8424c0u},
    {12u, 0x842480u}, {12u, 0x842440u}, {48u, 0x8423c0u},
    {12u, 0x842380u}, {16u, 0x842340u}, {12u, 0x842300u},
    {12u, 0x8422c0u}, {12u, 0x842280u}, {12u, 0x842240u},
    {12u, 0x842200u}, {16u, 0x8421c0u}, {12u, 0x842180u},
}};

void WriteByte(ManagerObjectImage& image, const std::size_t offset,
               const std::uint8_t value) {
    image.bytes[offset] = value;
    image.written[offset] = true;
}

void WriteDword(ManagerObjectImage& image, const std::size_t offset,
                const std::uint32_t value) {
    for (std::size_t i = 0; i < 4; ++i) {
        WriteByte(image, offset + i,
                  static_cast<std::uint8_t>(value >> (8u * i)));
    }
}

ManagerObjectImage BuildImage(const std::uint32_t key,
                              const std::uint8_t stack_byte) {
    const auto& spec = kManagerSpecs[key - 1u];
    ManagerObjectImage image{};
    image.size = spec.size;
    image.final_vtable = spec.vtable;
    image.base_vtable = key == 6u ? 0x842400u : spec.vtable;
    if (key == 6u) image.InstallBaseVtable();
    WriteDword(image, 0u, spec.vtable);
    WriteByte(image, 4u, 0u);
    WriteByte(image, 5u, 0u);
    if (key != 14u) {
        WriteDword(image, 8u, 0u);
    }
    if (key == 8u || key == 14u) {
        WriteDword(image, 12u, 0u);
    }
    if (key == 6u) {
        WriteDword(image, 12u, 0u);
        WriteDword(image, 16u, 0u);
        WriteDword(image, 24u, 0u);
        image.InitializeExtendedTail(stack_byte);
    }
    return image;
}

}  // namespace

bool ManagerObjectImage::HasWrite(const std::size_t offset) const {
    return offset < written.size() && written[offset];
}

std::uint32_t ManagerObjectImage::Dword(const std::size_t offset) const {
    std::uint32_t result = 0u;
    if (offset + 4u > bytes.size()) {
        return result;
    }
    for (std::size_t i = 0; i < 4; ++i) {
        result |= static_cast<std::uint32_t>(bytes[offset + i]) << (8u * i);
    }
    return result;
}

std::uint8_t ManagerObjectImage::IsInitialized() const {
    return bytes[4];
}

std::uint8_t ManagerObjectImage::IsEnabled() const {
    return bytes[5];
}

void ManagerObjectImage::InstallBaseVtable() {
    base_vtable = 0x842400u;
    WriteDword(*this, 0u, base_vtable);
}

ManagerObjectImage& ManagerObjectImage::InitializeExtendedTail(
    const std::uint8_t source_byte) {
    WriteByte(*this, 32u, source_byte);
    WriteDword(*this, 36u, 0u);
    WriteDword(*this, 40u, 0u);
    WriteDword(*this, 44u, 0u);
    return *this;
}

void GameManagerInitRuntimeStub::ResetExisting(
    ScreenEffectDispatcherState& /*state*/) {}
std::uint32_t GameManagerInitRuntimeStub::AllocateCameraRecord() {
    return 0u;
}
bool GameManagerInitRuntimeStub::InitializeCamera(std::uint32_t /*record*/) {
    return false;
}
void GameManagerInitRuntimeStub::DestroyCameraRecord(
    std::uint32_t /*record*/) {}
std::uint32_t GameManagerInitRuntimeStub::AllocateObject(
    std::uint32_t /*size*/) { return 0u; }
void GameManagerInitRuntimeStub::InitializeObject(
    std::uint32_t /*address*/, const ManagerObjectImage& /*image*/) {}
void GameManagerInitRuntimeStub::MakePair(
    std::uint32_t /*key*/, std::uint32_t /*address*/) {}
void GameManagerInitRuntimeStub::InsertManager(
    std::uint32_t /*key*/, std::uint32_t /*address*/) {}
void GameManagerInitRuntimeStub::SetNodeData(std::uint32_t /*key*/) {}

bool GameManagerInitializer::Initialize(
    ScreenEffectDispatcherState& state,
    const std::uint8_t uninitialized_stack_byte,
    IGameManagerInitRuntime& runtime) {
    runtime.ResetExisting(state);
    const auto camera = runtime.AllocateCameraRecord();
    state.frame_state = camera;
    if (!runtime.InitializeCamera(camera)) {
        runtime.DestroyCameraRecord(camera);
        state.frame_state = 0u;
        return false;
    }

    for (std::uint32_t key = 1u; key <= kManagerSpecs.size(); ++key) {
        const auto image = BuildImage(key, uninitialized_stack_byte);
        const auto object = runtime.AllocateObject(image.size);
        if (object != 0u) {
            runtime.InitializeObject(object, image);
        }
        if (key < 14u) {
            runtime.MakePair(key, object);
        }
        runtime.InsertManager(key, object);
        runtime.SetNodeData(key);
    }
    state.active = 0;
    state.enabled = 1u;
    return true;
}

}  // namespace pal4
