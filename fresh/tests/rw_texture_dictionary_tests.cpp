#include "pal4/rw_texture_dictionary.h"
#include "pal4/legacy_crc32.h"

#include <algorithm>
#include <array>
#include <cstring>
#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) { std::cerr << message << '\n'; std::exit(1); }
}

class Runtime final : public pal4::IRwTextureDictionaryContentRuntime {
public:
    std::uint32_t created = 0;
    std::uint32_t destroyed = 0;
    std::vector<std::string> calls;
    std::vector<std::uint32_t> textures;
    std::map<std::uint32_t, std::int32_t> references;
    std::map<std::string, std::uint32_t> named_textures;
    std::string assigned_name;
    std::uint32_t read_result = 0;
    std::uint32_t add_result = 0;
    std::uint32_t raster_result = 0;
    std::uint32_t texture_result = 0;
    std::uint8_t filter = 0;
    std::vector<std::uint8_t> raster_pixels = std::vector<std::uint8_t>(32, 0xee);
    bool lock_succeeds = true;
    std::int32_t raster_depth_bits = 16;
    std::int32_t raster_stride = 8;
    std::int32_t expected_raster_height = 3;
    std::int32_t raster_width = 2;
    std::int32_t raster_height = 3;

    std::uint32_t CreateDictionary() override {
        calls.emplace_back("create");
        return created;
    }
    void ForEachTexture(
        std::uint32_t dictionary,
        const std::function<std::uint32_t(std::uint32_t)>& visit) override {
        Check(dictionary == 0x1234u, "traverses the owned dictionary");
        calls.emplace_back("traverse");
        for (const auto texture : textures)
            Check(visit(texture) == texture, "callback returns its texture");
    }
    std::uint32_t DestroyDictionary(std::uint32_t dictionary) override {
        Check(dictionary == 0x1234u, "destroys the owned dictionary");
        calls.emplace_back("destroy");
        return destroyed;
    }
    std::int32_t TextureReferences(std::uint32_t texture) override {
        return references.at(texture);
    }
    void SetTextureReferences(std::uint32_t texture,
                              std::int32_t count) override {
        calls.emplace_back("reference");
        references.at(texture) = count;
    }
    void CopyName(std::array<std::uint8_t, 256>& buffer,
                  const char* name, std::uint32_t count) override {
        Check(count == 256 && name != nullptr, "copies the source name into 256 bytes");
        calls.emplace_back("copy");
        const auto length = std::min<std::size_t>(std::strlen(name), count);
        std::memcpy(buffer.data(), name, length);
    }
    void NormalizeName(std::array<std::uint8_t, 256>& buffer) override {
        calls.emplace_back("normalize");
        for (auto& byte : buffer)
            if (byte >= 'A' && byte <= 'Z')
                byte = static_cast<std::uint8_t>(byte - 'A' + 'a');
    }
    std::uint32_t NameLength(
        const std::array<std::uint8_t, 256>& buffer) override {
        calls.emplace_back("length");
        return static_cast<std::uint32_t>(
            std::find(buffer.begin(), buffer.end(), 0) - buffer.begin());
    }
    std::uint32_t FindNamedTexture(std::uint32_t dictionary,
                                   const char* name) override {
        Check(dictionary == 0x1234u, "lookup uses owned dictionary");
        calls.emplace_back("find");
        const auto found = named_textures.find(name);
        return found == named_textures.end() ? 0 : found->second;
    }
    void PrepareEmptyErrorString() override { calls.emplace_back("prepare"); }
    void SetCurrentDictionary(std::uint32_t dictionary) override {
        Check(dictionary == 0x1234u, "sets owned dictionary current");
        calls.emplace_back("current");
    }
    std::uint32_t ReadTexture(const char* name) override {
        Check(std::strcmp(name, "Stone") == 0, "reads the source texture name");
        calls.emplace_back("read");
        return read_result;
    }
    void SetTextureFilter(std::uint32_t texture,
                          std::uint8_t value) override {
        Check(texture == 0x55u, "sets the loaded texture filter");
        calls.emplace_back("filter");
        filter = value;
    }
    void SetTextureName(std::uint32_t texture,
                        const char* name) override {
        Check(texture == 0x55u, "names the loaded texture");
        calls.emplace_back("name");
        assigned_name = name;
    }
    std::uint32_t AddTextureToDictionary(
        std::uint32_t dictionary, std::uint32_t texture) override {
        Check(dictionary == 0x1234u && texture == 0x55u,
              "adds the loaded texture to owned dictionary");
        calls.emplace_back("add");
        return add_result;
    }
    void RemoveTextureFromDictionary(std::uint32_t texture) override {
        Check(texture == 0x44u, "removes the found texture");
        calls.emplace_back("remove");
    }
    std::uint32_t CreateRaster(std::int32_t width, std::int32_t height,
                               std::uint32_t depth,
                               std::uint32_t flags) override {
        Check(width == 2 && height == expected_raster_height &&
                  depth == 8 && flags == 12,
              "raster forwards dimensions and ORs depth with four");
        calls.emplace_back("raster");
        return raster_result;
    }
    std::uint32_t CreateTexture(std::uint32_t raster) override {
        Check(raster == raster_result, "texture receives created raster, including null");
        calls.emplace_back("texture");
        return texture_result;
    }
    std::uint32_t TextureRaster(std::uint32_t texture) override {
        Check(texture == 0x55u, "reads raster from created texture");
        calls.emplace_back("texture-raster");
        return 0x66u;
    }
    std::int32_t RasterWidth(std::uint32_t raster) override {
        Check(raster == 0x66u, "reads raster width");
        calls.emplace_back("width");
        return raster_width;
    }
    std::int32_t RasterHeight(std::uint32_t raster) override {
        Check(raster == 0x66u, "reads raster height");
        calls.emplace_back("height");
        return raster_height;
    }
    std::uint8_t* LockRaster(std::uint32_t raster, std::int32_t level,
                             std::int32_t mode) override {
        Check(raster == 0x66u && level == 0 && mode == 1,
              "locks raster at mip zero for writing");
        calls.emplace_back("lock");
        return lock_succeeds ? raster_pixels.data() : nullptr;
    }
    std::int32_t RasterStride(std::uint32_t raster) override {
        Check(raster == 0x66u, "reads raster stride");
        calls.emplace_back("stride");
        return raster_stride;
    }
    std::int32_t RasterDepthBits(std::uint32_t raster) override {
        Check(raster == 0x66u, "reads raster depth");
        calls.emplace_back("depth");
        return raster_depth_bits;
    }
    void UnlockRaster(std::uint32_t raster) override {
        Check(raster == 0x66u, "unlocks created raster");
        calls.emplace_back("unlock");
    }
};

void ConstructionAndReferenceCount() {
    pal4::RwTextureDictionaryState state{4, 8, 9};
    Check(&pal4::RwTextureDictionary::Construct(state) == &state &&
              state.vtable == 0x84b940u && state.dictionary == 0 &&
              state.enabled == 1,
          "constructor sets all three original words and returns this");
    state.dictionary = 0x1234u;
    state.enabled = 7;
    pal4::RwTextureDictionary::Destruct(state);
    Check(state.vtable == 0x84b940u && state.dictionary == 0x1234u &&
              state.enabled == 7,
          "destructor only restores the vtable");

    Runtime runtime;
    for (const auto count : {-1, 0, 1, 2, 3}) {
        runtime.references[0x55u] = count;
        Check(pal4::RwTextureDictionary::DecrementTextureReferences(
                  0x55u, runtime) == 0x55u &&
                  runtime.references[0x55u] == (count > 1 ? 1 : count),
              "texture callback clamps only signed counts above one");
    }
}

void CleanupAndInitialize() {
    Runtime runtime;
    pal4::RwTextureDictionaryState state{0, 0, 9};
    Check(pal4::RwTextureDictionary::Cleanup(state, runtime) == 0 &&
              state.enabled == 1 && runtime.calls.empty(),
          "empty cleanup only resets the enabled word");

    state.dictionary = 0x1234u;
    state.enabled = 8;
    runtime.textures = {10u, 20u, 30u};
    runtime.references = {{10u, 3}, {20u, 1}, {30u, -1}};
    runtime.destroyed = 0x99u;
    Check(pal4::RwTextureDictionary::Cleanup(state, runtime) == 0x99u &&
              state.dictionary == 0 && state.enabled == 1 &&
              runtime.references[10u] == 1 &&
              runtime.references[20u] == 1 &&
              runtime.references[30u] == -1 &&
              runtime.calls == std::vector<std::string>{
                  "traverse", "reference", "destroy"},
          "cleanup traverses, clamps, destroys and resets in order");

    runtime.calls.clear();
    runtime.created = 0x4567u;
    Check(pal4::RwTextureDictionary::Initialize(state, runtime) &&
              state.dictionary == 0x4567u &&
              runtime.calls == std::vector<std::string>{"create"},
          "initialize creates a dictionary after empty cleanup");

    runtime.calls.clear();
    state.dictionary = 0x1234u;
    state.enabled = 0;
    runtime.references[10u] = 3;
    runtime.created = 0;
    Check(!pal4::RwTextureDictionary::Initialize(state, runtime) &&
              state.dictionary == 0 && state.enabled == 1 &&
              runtime.calls == std::vector<std::string>{
                  "traverse", "reference", "destroy", "create"},
          "initialize cleans the previous dictionary before failed create");
}

void FindAndAddTexture() {
    Runtime runtime;
    pal4::LegacyCrc32 crc;
    pal4::RwTextureDictionaryState state{0, 0, 1};
    Check(pal4::RwTextureDictionary::FindTextureByCrc(
              state, "Stone", runtime, crc) == 0 && runtime.calls.empty() &&
              !crc.Initialized(),
          "lookup without dictionary does not hash or access name");
    Check(pal4::RwTextureDictionary::AddTexture(
              state, "Stone", runtime, crc) == 0 && runtime.calls.empty(),
          "add without dictionary short circuits");
    state.dictionary = 0x1234u;
    Check(pal4::RwTextureDictionary::AddTexture(
              state, nullptr, runtime, crc) == 0 &&
              pal4::RwTextureDictionary::AddTexture(
                  state, "", runtime, crc) == 0 && runtime.calls.empty(),
          "add rejects null and empty names before lookup");

    constexpr std::array<std::uint8_t, 5> normalized{
        's', 't', 'o', 'n', 'e'};
    pal4::LegacyCrc32 expected_crc;
    const auto name = std::to_string(expected_crc.ComputeAndStore(
        normalized.data(), normalized.size()));
    runtime.named_textures[name] = 0x44u;
    runtime.references[0x44u] = 2;
    Check(pal4::RwTextureDictionary::FindTextureByCrc(
              state, "Stone", runtime, crc) == 0x44u &&
              crc.LastResult() == expected_crc.LastResult() &&
              runtime.references[0x44u] == 3 &&
              runtime.calls == std::vector<std::string>{
                  "copy", "normalize", "length", "find", "reference"},
          "lookup hashes normalized name and increments found reference");
    runtime.calls.clear();
    Check(pal4::RwTextureDictionary::AddTexture(
              state, "Stone", runtime, crc) == 0x44u &&
              runtime.references[0x44u] == 4 &&
              runtime.calls == std::vector<std::string>{
                  "copy", "normalize", "length", "find", "reference"},
          "add returns a found texture without trying to read it");

    runtime.named_textures.clear();
    runtime.calls.clear();
    Check(pal4::RwTextureDictionary::AddTexture(
              state, "Stone", runtime, crc) == 0 &&
              runtime.calls == std::vector<std::string>{
                  "copy", "normalize", "length", "find", "prepare",
                  "current", "read"},
          "read failure stops before texture mutation");
    runtime.calls.clear();
    runtime.read_result = 0x55u;
    runtime.add_result = 0x99u;
    Check(pal4::RwTextureDictionary::AddTexture(
              state, "Stone", runtime, crc) == 0x99u &&
              runtime.filter == 2 && runtime.assigned_name == name &&
              runtime.calls == std::vector<std::string>{
                  "copy", "normalize", "length", "find", "prepare",
                  "current", "read", "filter", "copy", "normalize",
                  "length", "name", "add"},
          "add rereads normalized CRC name and returns RenderWare result");

    runtime.calls.clear();
    state.dictionary = 0;
    Check(!pal4::RwTextureDictionary::RemoveTextureByCrc(
              state, "Stone", runtime, crc) &&
              !pal4::RwTextureDictionary::SetCurrent(state, runtime) &&
              runtime.calls.empty(),
          "remove and set-current skip absent dictionaries");
    state.dictionary = 0x1234u;
    Check(pal4::RwTextureDictionary::SetCurrent(state, runtime) &&
              runtime.calls == std::vector<std::string>{"current"},
          "set-current forwards the owned dictionary");
    runtime.calls.clear();
    Check(!pal4::RwTextureDictionary::RemoveTextureByCrc(
              state, "Stone", runtime, crc) &&
              runtime.calls == std::vector<std::string>{
                  "copy", "normalize", "length", "find"},
          "remove miss does not modify a texture");
    runtime.named_textures[name] = 0x44u;
    runtime.references[0x44u] = 5;
    runtime.calls.clear();
    Check(pal4::RwTextureDictionary::RemoveTextureByCrc(
              state, "Stone", runtime, crc) &&
              runtime.references[0x44u] == 1 &&
              runtime.calls == std::vector<std::string>{
                  "copy", "normalize", "length", "find", "reference",
                  "remove"},
          "remove clamps references before native removal");
    runtime.references[0x44u] = -1;
    runtime.calls.clear();
    Check(pal4::RwTextureDictionary::RemoveTextureByCrc(
              state, "Stone", runtime, crc) &&
              runtime.references[0x44u] == -1 &&
              runtime.calls.back() == "remove" &&
              runtime.calls.size() == 5,
          "remove preserves signed counts at or below one");
}

void AddTextureFromPixels() {
    Runtime runtime;
    pal4::RwTextureDictionaryState state{0, 0, 7};
    const std::array<std::uint8_t, 12> pixels{
        1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12};
    Check(pal4::RwTextureDictionary::AddTextureFromPixels(
              state, pixels.data(), 2, 3, 8, runtime) == 0 &&
              runtime.calls.empty() && state.enabled == 7,
          "pixel add short circuits without dictionary");
    state.dictionary = 0x1234u;
    Check(pal4::RwTextureDictionary::AddTextureFromPixels(
              state, pixels.data(), 2, 3, 8, runtime) == 0 &&
              runtime.calls == std::vector<std::string>{"raster"} &&
              state.enabled == 7,
          "raster failure leaves sequence untouched");
    runtime.calls.clear();
    runtime.raster_result = 0x66u;
    Check(pal4::RwTextureDictionary::AddTextureFromPixels(
              state, pixels.data(), 2, 3, 8, runtime) == 0 &&
              runtime.calls == std::vector<std::string>{"raster", "texture"} &&
              state.enabled == 7,
          "texture failure leaves sequence untouched");
    runtime.calls.clear();
    runtime.texture_result = 0x55u;
    runtime.add_result = 0x99u;
    Check(pal4::RwTextureDictionary::AddTextureFromPixels(
              state, pixels.data(), 2, 3, 8, runtime) == 0x99u &&
              state.enabled == 8 && runtime.filter == 1 &&
              runtime.assigned_name == "texture-autoname(8)" &&
              runtime.calls == std::vector<std::string>{
                  "raster", "texture", "filter", "texture-raster", "width",
                  "height", "lock", "depth", "stride", "unlock", "name", "add"} &&
              runtime.raster_pixels[0] == 1 &&
              runtime.raster_pixels[1] == 2 &&
              runtime.raster_pixels[8] == 5,
          "pixel add copies data, increments sequence and adds named texture");
}

void FillTexture() {
    Runtime runtime;
    pal4::RwTextureDictionaryState state{0, 0, 1};
    Check(pal4::RwTextureDictionary::CreateAndFillTexture(
              state, 2, 3, 8, 0x12345678u, runtime) == 0 &&
              runtime.calls.empty(),
          "fill skips an absent dictionary");
    state.dictionary = 0x1234u;
    Check(pal4::RwTextureDictionary::CreateAndFillTexture(
              state, 2, 3, 8, 0x12345678u, runtime) == 0 &&
              runtime.calls == std::vector<std::string>{"raster", "texture"},
          "fill calls texture creation even for a null raster");
    runtime.calls.clear();
    runtime.raster_result = 0x66u;
    runtime.texture_result = 0x55u;
    runtime.lock_succeeds = false;
    Check(pal4::RwTextureDictionary::CreateAndFillTexture(
              state, 2, 3, 8, 0x12345678u, runtime) == 0 &&
              runtime.calls == std::vector<std::string>{
                  "raster", "texture", "texture-raster", "filter", "lock"} &&
              state.enabled == 1,
          "lock failure returns without unlocking or naming");

    runtime.calls.clear();
    runtime.lock_succeeds = true;
    runtime.add_result = 0x99u;
    Check(pal4::RwTextureDictionary::CreateAndFillTexture(
              state, 2, 3, 8, 0x12345678u, runtime) == 0x99u &&
              state.enabled == 2 && runtime.filter == 1 &&
              runtime.assigned_name == "texture-autoname(2)" &&
              runtime.calls == std::vector<std::string>{
                  "raster", "texture", "texture-raster", "filter", "lock",
                  "depth", "stride", "unlock", "name", "add"},
          "fill locks, fills, unlocks, names and adds");
    for (std::size_t row = 0; row < 3; ++row) {
        const auto offset = row * 8;
        Check(runtime.raster_pixels[offset] == 0x78u &&
                  runtime.raster_pixels[offset + 1] == 0x56u &&
                  runtime.raster_pixels[offset + 2] == 0x78u &&
                  runtime.raster_pixels[offset + 3] == 0x56u &&
                  runtime.raster_pixels[offset + 4] == 0xeeu,
              "16-bit color fill honors pixel size and row stride");
    }
    runtime.raster_depth_bits = 32;
    runtime.raster_stride = 8;
    runtime.expected_raster_height = 2;
    runtime.raster_pixels.assign(32, 0);
    runtime.calls.clear();
    Check(pal4::RwTextureDictionary::CreateAndFillTexture(
              state, 2, 2, 8, 0x12345678u, runtime) == 0x99u &&
              runtime.raster_pixels[0] == 0x78u &&
              runtime.raster_pixels[1] == 0x56u &&
              runtime.raster_pixels[2] == 0x34u &&
              runtime.raster_pixels[3] == 0x12u &&
              runtime.raster_pixels[8] == 0x78u,
          "32-bit fill repeats all four color bytes by row");
}

}  // namespace

int main() {
    ConstructionAndReferenceCount();
    CleanupAndInitialize();
    FindAndAddTexture();
    AddTextureFromPixels();
    FillTexture();
    std::cout << "rw_texture_dictionary_tests passed\n";
}
