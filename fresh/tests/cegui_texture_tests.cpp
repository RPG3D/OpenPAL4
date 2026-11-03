#include "pal4/cegui_texture.h"

#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

class Runtime final : public pal4::CeguiTextureRuntimeStub {
public:
    pal4::CeguiTextureState* state = nullptr;
    std::vector<std::string> calls;
    std::vector<std::uint32_t> freed;
    std::vector<std::uint32_t> read_addresses;
    std::uint32_t free_result = 0x7777u;
    std::uint32_t lookup_result = 0x8888u;
    std::uint32_t create_result = 0x9000u;
    std::uint32_t update_result = 0xaaaa0001u;
    std::uint32_t image_width = 13u;
    std::uint32_t image_height = 17u;
    std::uint32_t alpha_calls = 0u;
    bool temporary_image = false;

    std::uint32_t DestroyPluginObject(std::uint32_t address) override {
        if (address == 0xa000u) {
            Check(temporary_image && state->ReadDword(12u) == 0u,
                  "temporary mask image is separate from cached colour image");
            calls.emplace_back("destroy_mask_image");
            return 0xfaceu;
        } else {
            Check(address == 0x2000u &&
                      state->ReadDword(0u) ==
                          pal4::CeguiTextureState::kVtable &&
                      state->ReadDword(12u) == address,
                  "plugin destruction observes derived vtable and live pointer");
            calls.emplace_back("destroy_plugin");
            return 0u;
        }
    }
    std::uint32_t Free(std::uint32_t address) override {
        Check(state->ReadDword(0u) ==
                  (address == 0x9999u ? 0x12345678u
                                       : pal4::CeguiTextureState::kVtable),
              "free observes the correct destructor vtable phase");
        freed.push_back(address);
        calls.emplace_back("free");
        return free_result;
    }
    std::uint32_t BaseTextureVtable() override {
        calls.emplace_back("base_vtable");
        return 0x12345678u;
    }
    std::uint32_t ReadDword(std::uint32_t address) override {
        calls.emplace_back("read_dword");
        read_addresses.push_back(address);
        return pal4::CeguiTextureRuntimeStub::ReadDword(address);
    }
    std::uint16_t ReadWord(std::uint32_t address) override {
        calls.emplace_back("read_word");
        return pal4::CeguiTextureRuntimeStub::ReadWord(address);
    }
    std::uint32_t InitializeTextureDictionary() override {
        calls.emplace_back("dictionary");
        return 0x3000u;
    }
    std::uint32_t BuildUtf8(std::uint32_t string_address) override {
        Check(string_address == 0x4000u, "UTF-8 receives CEGUI String");
        calls.emplace_back("utf8");
        return 0x5000u;
    }
    std::uint32_t LookupTexture(std::uint32_t dictionary,
                                std::uint32_t utf8_address) override {
        Check(dictionary == 0x3000u && utf8_address == 0x5000u,
              "lookup receives dictionary and UTF-8 buffer");
        calls.emplace_back("lookup");
        return lookup_result;
    }
    std::uint32_t CreateTexture(std::uint32_t dictionary,
                                std::uint32_t width, std::uint32_t height,
                                std::uint32_t color,
                                std::uint32_t format) override {
        Check(dictionary == 0x3000u && width == 13u && height == 17u &&
                  color == 0xabcdef01u && format == 1280u,
              "create forwards dimensions, colour and fixed format");
        calls.emplace_back("create");
        return create_result;
    }
    std::uint32_t LockAndCopyTexture(
        std::uint32_t dictionary, std::uint32_t texture,
        std::uint32_t source, std::uint32_t x, std::uint32_t y,
        std::uint32_t width, std::uint32_t height) override {
        Check(dictionary == 0x3000u && texture == 0x9000u &&
                  source == 0x6000u && x == 2u && y == 3u &&
                  width == 5u && height == 7u,
              "update forwards texture and reordered source/rectangle");
        calls.emplace_back("update");
        return update_result;
    }
    std::uint32_t CreateImage(std::uint32_t width,
                              std::uint32_t height,
                              std::uint32_t depth) override {
        Check(width == image_width && height == image_height && depth == 32u,
              "image creation uses raster dimensions and 32-bit depth");
        calls.emplace_back("create_image");
        return 0xa000u;
    }
    void AllocateImagePixels(std::uint32_t image) override {
        Check(image == 0xa000u &&
                  state->ReadDword(12u) ==
                      (temporary_image ? 0u : image),
              "pixel allocation sees the expected image ownership");
        calls.emplace_back("allocate_pixels");
    }
    void CopyRasterToImage(std::uint32_t image,
                           std::uint32_t raster) override {
        Check(image == 0xa000u && raster == 0xb000u,
              "raster copy receives new image and raster");
        calls.emplace_back("copy_raster");
    }
    void ConstructColour(std::uint32_t& output,
                         std::uint32_t argb) override {
        Check(argb == 0xa1b2c3d4u, "colour constructor sees sampled ARGB");
        calls.emplace_back("colour");
        output = argb;
    }
    float AlphaFromArgb(std::uint32_t argb) override {
        ++alpha_calls;
        calls.emplace_back("alpha");
        return pal4::CeguiTextureRuntimeStub::AlphaFromArgb(argb);
    }
    void ProcessImagePath(std::uint32_t image,
                          std::uint32_t utf8_address) override {
        Check(image == 0xa000u && utf8_address == 0x5000u,
              "image processor receives temporary image and UTF-8 path");
        calls.emplace_back("process_image");
    }
};

void ConstructorWritesOnlyObservedFields() {
    pal4::CeguiTextureState state;
    for (std::uint32_t offset = 0u; offset < state.kBytes; ++offset) {
        state.WriteByte(offset, 0xa5u);
    }
    Check(&pal4::CeguiTexture::Construct(state, 0x12345678u) == &state,
          "constructor returns object identity");
    Check(state.ReadDword(0u) == state.kVtable &&
              state.ReadDword(4u) == 0x12345678u &&
              state.ReadDword(8u) == 0u &&
              state.ReadDword(12u) == 0u &&
              state.ReadDword(16u) == 0u &&
              state.ReadByte(20u) == 0x78u &&
              state.ReadDword(24u) == 0u &&
              state.ReadDword(28u) == 0u &&
              state.ReadDword(32u) == 0u,
          "constructor reproduces observed field values");
    for (std::uint32_t offset = 21u; offset < 24u; ++offset) {
        Check(state.ReadByte(offset) == 0xa5u,
              "constructor preserves unobserved padding");
    }
}

void CleanupDestroysPluginAndResetsOwnedFields() {
    pal4::CeguiTextureState state;
    Runtime runtime;
    runtime.state = &state;
    state.WriteDword(0u, 0u);
    state.WriteDword(8u, 0x1111u);
    state.WriteDword(12u, 0x2000u);
    state.WriteDword(16u, 0x3333u);
    state.WriteDword(24u, 0x4444u);
    state.WriteDword(28u, 0x5555u);
    state.WriteDword(32u, 0x6666u);
    Check(pal4::CeguiTexture::Cleanup(state, runtime) == 0x7777u,
          "cleanup returns buffer free result");
    Check(runtime.calls == std::vector<std::string>{
              "destroy_plugin", "free", "base_vtable"} &&
              runtime.freed == std::vector<std::uint32_t>{0x4444u},
          "cleanup uses observed call order and buffer address");
    Check(state.ReadDword(0u) == 0x12345678u &&
              state.ReadDword(8u) == 0x1111u &&
              state.ReadDword(12u) == 0u &&
              state.ReadDword(16u) == 0u &&
              state.ReadDword(24u) == 0u &&
              state.ReadDword(28u) == 0u &&
              state.ReadDword(32u) == 0u,
          "cleanup clears owned fields and preserves texture handle");
}

void CleanupWithoutPluginStillFreesNullBuffer() {
    pal4::CeguiTextureState state;
    Runtime runtime;
    runtime.state = &state;
    Check(pal4::CeguiTexture::Cleanup(state, runtime) == 0x7777u &&
              runtime.calls == std::vector<std::string>{
                  "free", "base_vtable"} &&
              runtime.freed == std::vector<std::uint32_t>{0u},
          "null plugin skips destruction but null buffer reaches free");
}

void DestructorUsesOnlyLowFlagBit() {
    for (const auto flags : {0u, 2u, 1u, 3u}) {
        pal4::CeguiTextureState state;
        Runtime runtime;
        runtime.state = &state;
        Check(&pal4::CeguiTexture::Destroy(state,
                  static_cast<std::uint8_t>(flags), 0x9999u, runtime) ==
                  &state,
              "destructor returns object identity");
        const auto should_free = (flags & 1u) != 0u;
        Check(runtime.freed == (should_free
                  ? std::vector<std::uint32_t>{0u, 0x9999u}
                  : std::vector<std::uint32_t>{0u}),
              "destructor frees object only when low bit is set");
        Check(state.ReadDword(0u) == 0x12345678u,
              "destructor finishes at base vtable");
    }
}

void NestedWordAccessorsFollowTwoPointers() {
    pal4::CeguiTextureState state;
    Runtime runtime;
    Check(pal4::CeguiTexture::NestedWord0C(state, runtime) == 0u &&
              pal4::CeguiTexture::NestedWord10(state, runtime) == 0u &&
              runtime.calls.empty(),
          "null texture returns zero without memory reads");
    state.WriteDword(8u, 0x6000u);
    runtime.WriteDword(0x6000u, 0x7000u);
    runtime.WriteWord(0x700cu, 0xabcdu);
    runtime.WriteWord(0x7010u, 0x1234u);
    Check(pal4::CeguiTexture::NestedWord0C(state, runtime) == 0xabcdu &&
              pal4::CeguiTexture::NestedWord10(state, runtime) == 0x1234u &&
              runtime.calls == std::vector<std::string>{
                  "read_dword", "read_word", "read_dword", "read_word"},
          "accessors read two distinct 16-bit nested fields");
}

void LookupStoresResultIncludingNull() {
    pal4::CeguiTextureState state;
    Runtime runtime;
    state.WriteDword(8u, 0xaaaa5555u);
    Check(pal4::CeguiTexture::LoadByName(state, 0x4000u, 0xdeadbeefu,
                                          runtime) == 0x8888u &&
              state.ReadDword(8u) == 0x8888u &&
              runtime.calls == std::vector<std::string>{
                  "dictionary", "utf8", "lookup"},
          "lookup converts name then stores dictionary result");
    runtime.lookup_result = 0u;
    runtime.calls.clear();
    Check(pal4::CeguiTexture::LoadByName(state, 0x4000u, 0u,
                                          runtime) == 0u &&
              state.ReadDword(8u) == 0u &&
              runtime.calls == std::vector<std::string>{
                  "dictionary", "utf8", "lookup"},
          "missing texture clears previous handle");
}

void CreateAndUpdateForwardDictionaryCalls() {
    pal4::CeguiTextureState state;
    Runtime runtime;
    runtime.state = &state;
    Check(pal4::CeguiTexture::CreateWithDimensions(
              state, 13u, 17u, 0xabcdef01u, runtime) == 0x9000u &&
              state.ReadDword(8u) == 0x9000u &&
              runtime.calls == std::vector<std::string>{
                  "dictionary", "create"},
          "texture creation stores dictionary result");
    runtime.calls.clear();
    Check(pal4::CeguiTexture::Update(state, 2u, 3u, 5u, 7u,
                                      0x6000u, runtime) == 0xaaaa0001u &&
              runtime.calls == std::vector<std::string>{
                  "dictionary", "update"},
          "texture update forwards reordered arguments and result");
}

void ReadColourCreatesImageOnceAndUsesPixelStride() {
    pal4::CeguiTextureState state;
    Runtime runtime;
    runtime.state = &state;
    state.WriteDword(8u, 0x9000u);
    runtime.WriteDword(0x9000u, 0xb000u);
    runtime.WriteDword(0xb00cu, 13u);
    runtime.WriteDword(0xb010u, 17u);
    runtime.WriteDword(0xa010u, 52u);
    runtime.WriteDword(0xa014u, 0xc000u);
    runtime.WriteDword(0xc000u + 2u * 52u + 4u * 3u, 0xa1b2c3d4u);
    std::uint32_t colour = 0u;
    Check(&pal4::CeguiTexture::ReadColour(state, colour, 3u, 2u,
                                           runtime) == &colour &&
              colour == 0xa1b2c3d4u && state.ReadDword(12u) == 0xa000u,
          "first colour read creates and caches image");
    Check(runtime.calls == std::vector<std::string>{
              "read_dword", "read_dword", "read_dword", "create_image",
              "allocate_pixels", "copy_raster", "read_dword", "read_dword",
              "read_dword", "colour"},
          "first colour read preserves image construction order");
    Check(runtime.read_addresses == std::vector<std::uint32_t>{
              0x9000u, 0xb010u, 0xb00cu, 0xa010u, 0xa014u,
              0xc000u + 2u * 52u + 4u * 3u},
          "first colour read follows raw raster and pixel read order");
    runtime.calls.clear();
    runtime.read_addresses.clear();
    colour = 0u;
    Check(&pal4::CeguiTexture::ReadColour(state, colour, 3u, 2u,
                                           runtime) == &colour &&
              colour == 0xa1b2c3d4u &&
              runtime.calls == std::vector<std::string>{
                  "read_dword", "read_dword", "read_dword", "colour"},
          "cached colour read skips raster conversion");
    Check(runtime.read_addresses == std::vector<std::uint32_t>{
              0xa010u, 0xa014u, 0xc000u + 2u * 52u + 4u * 3u},
          "cached colour read uses stored image stride and pixels");
}

void AlphaMaskBuildsOnceAndTestsThreshold() {
    pal4::CeguiTextureState state;
    Runtime runtime;
    runtime.state = &state;
    runtime.image_width = 3u;
    runtime.image_height = 2u;
    runtime.temporary_image = true;
    state.WriteDword(8u, 0x9000u);
    runtime.WriteDword(0x9000u, 0xb000u);
    runtime.WriteDword(0xb00cu, 3u);
    runtime.WriteDword(0xb010u, 2u);
    runtime.WriteWord(0xb00cu, 3u);
    runtime.WriteWord(0xb010u, 2u);
    runtime.WriteDword(0xa010u, 16u);
    runtime.WriteDword(0xa014u, 0xc000u);
    runtime.WriteDword(0xc000u, 0x00000000u);
    runtime.WriteDword(0xc004u, 0x19000000u);
    runtime.WriteDword(0xc008u, 0x1a000000u);
    runtime.WriteDword(0xc010u, 0xff000000u);
    runtime.WriteDword(0xc014u, 0x10000000u);
    runtime.WriteDword(0xc018u, 0x80000000u);

    Check(pal4::CeguiTexture::HitTestAlpha(state, 0u, 0u, runtime) &&
              state.MaskBitCount() == 6u &&
              state.MaskWordCapacity() >= 1u &&
              runtime.alpha_calls == 6u,
          "first hit builds six pixel bits and identifies transparent pixel");
    Check(runtime.calls.front() == "read_dword" &&
              runtime.calls[3] == "create_image" &&
              runtime.calls[4] == "allocate_pixels" &&
              runtime.calls[5] == "copy_raster" &&
              runtime.calls[runtime.calls.size() - 3u] ==
                  "destroy_mask_image",
          "mask builder allocates, fills and releases temporary image");
    const auto prior_calls = runtime.calls.size();
    Check(pal4::CeguiTexture::HitTestAlpha(state, 1u, 0u, runtime) &&
              !pal4::CeguiTexture::HitTestAlpha(state, 2u, 0u, runtime) &&
              !pal4::CeguiTexture::HitTestAlpha(state, 0u, 1u, runtime) &&
              pal4::CeguiTexture::HitTestAlpha(state, 1u, 1u, runtime) &&
              !pal4::CeguiTexture::HitTestAlpha(state, 2u, 1u, runtime) &&
              runtime.alpha_calls == 6u &&
              runtime.calls.size() == prior_calls + 10u,
          "cached mask preserves row-major bits and skips image rebuilding");
    static_cast<void>(pal4::CeguiTexture::Cleanup(state, runtime));
    Check(state.MaskBitCount() == 0u && state.MaskWordCapacity() == 0u,
          "texture cleanup releases semantic bit storage");
}

void ProcessImageByNameUsesTemporaryRasterImage() {
    pal4::CeguiTextureState state;
    Runtime runtime;
    runtime.state = &state;
    runtime.temporary_image = true;
    state.WriteDword(8u, 0x9000u);
    runtime.WriteDword(0x9000u, 0xb000u);
    runtime.WriteDword(0xb00cu, 13u);
    runtime.WriteDword(0xb010u, 17u);
    Check(pal4::CeguiTexture::ProcessImageByName(
              state, 0x4000u, runtime) == 0xfaceu &&
              state.ReadDword(12u) == 0u &&
              runtime.calls == std::vector<std::string>{
                  "read_dword", "read_dword", "read_dword",
                  "create_image", "allocate_pixels", "copy_raster",
                  "utf8", "process_image", "destroy_mask_image"},
          "named image path converts raster, processes path, and returns destroy result");
    Check(runtime.read_addresses == std::vector<std::uint32_t>{
              0x9000u, 0xb010u, 0xb00cu},
          "named image path reads raster dimensions in instruction order");
}

}  // namespace

int main() {
    ConstructorWritesOnlyObservedFields();
    CleanupDestroysPluginAndResetsOwnedFields();
    CleanupWithoutPluginStillFreesNullBuffer();
    DestructorUsesOnlyLowFlagBit();
    NestedWordAccessorsFollowTwoPointers();
    LookupStoresResultIncludingNull();
    CreateAndUpdateForwardDictionaryCalls();
    ReadColourCreatesImageOnceAndUsesPixelStride();
    AlphaMaskBuildsOnceAndTestsThreshold();
    ProcessImageByNameUsesTemporaryRasterImage();
    std::cout << "10 CEGUI texture test cases passed\n";
}
