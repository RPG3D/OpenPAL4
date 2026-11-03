#include "pal4/rw_atomic_material_callbacks.h"

#include <bit>
#include <cstdlib>
#include <iostream>
#include <map>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(1);
    }
}

struct Call final {
    char kind;
    std::uint32_t a;
    std::uint32_t b;
    std::uint32_t c;
    std::uint32_t d;
    bool operator==(const Call&) const = default;
};

class TextureApi final : public pal4::IRwTextureImageFactoryRuntime {
public:
    std::map<std::uint32_t, std::uint32_t> words;
    std::uint32_t image = 0x8000u;
    std::uint32_t raster = 0x9000u;
    std::uint32_t texture = 0xa000u;
    std::uint32_t create_count = 0;
    float last_scale = 0;

    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    std::uint8_t ReadByte(std::uint32_t) override { return 0; }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override { words[address] = value; }
    std::uint32_t CreateImage(std::uint32_t width,
                              std::uint32_t height,
                              std::uint32_t depth) override {
        Check(width == 8u && height == 4u && depth == 32u,
              "rebuilt texture factory uses source raster dimensions");
        return image;
    }
    void AllocatePixels(std::uint32_t) override {}
    void SetImageFromRaster(std::uint32_t,
                            std::uint32_t) override {}
    std::uint32_t CreateRaster(std::uint32_t width,
                               std::uint32_t height,
                               std::uint32_t depth,
                               std::uint32_t) override {
        Check(width == 8u && height == 4u && depth == 32u,
              "rebuilt raster builder receives temporary image");
        ++create_count;
        return raster;
    }
    std::uint32_t RasterLevelCount(std::uint32_t) override { return 0; }
    void LockRaster(std::uint32_t, std::uint32_t,
                    std::uint32_t) override {}
    std::uint32_t ResampleImage(std::uint32_t,
                                std::uint32_t,
                                std::uint32_t) override { return 0; }
    std::uint32_t CreateNormalMapImage(std::uint32_t,
                                       bool, float) override { return 0; }
    void SetRasterFromImage(std::uint32_t,
                            std::uint32_t) override {}
    void DestroyImage(std::uint32_t) override {}
    void UnlockRaster(std::uint32_t) override {}
    std::uint32_t CreateTexture(std::uint32_t value) override {
        Check(value == raster, "generated texture uses rebuilt raster");
        return texture;
    }
};

class Runtime final : public pal4::IRwAtomicMaterialRuntime {
public:
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<Call> calls;
    TextureApi texture;
    std::uint32_t skin_type = 1u;
    bool dispatch_material = false;

    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        calls.push_back({'W', address, value, 0, 0});
        words[address] = value;
    }
    pal4::IRwTextureImageFactoryRuntime& TextureRuntime() override {
        return texture;
    }
    std::uint32_t SkinAtomicType(std::uint32_t) override {
        return skin_type;
    }
    void SetSkinAtomicType(std::uint32_t atomic,
                           std::uint32_t type) override {
        calls.push_back({'T', atomic, type, 0, 0});
        skin_type = type;
    }
    void ForAllMaterials(std::uint32_t geometry,
                         pal4::RwMaterialCallback callback,
                         std::uint32_t config) override {
        calls.push_back({'G', geometry,
                         static_cast<std::uint32_t>(callback), config, 0});
        if (!dispatch_material) return;
        if (callback == pal4::RwMaterialCallback::NormalMapBasic)
            (void)pal4::RwAtomicMaterialCallbacks::SetBasicMaterialTextures(
                0x1000u, config, *this);
        else if (callback == pal4::RwMaterialCallback::NormalMapExtended)
            (void)pal4::RwAtomicMaterialCallbacks::SetExtendedMaterialTextures(
                0x1000u, config, *this);
        else
            (void)pal4::RwAtomicMaterialCallbacks::SetEnvironmentMaterial(
                0x1000u, config, *this);
    }
    void InitializeNormalMapAtomic(std::uint32_t atomic) override {
        calls.push_back({'I', atomic, 0, 0, 0});
    }
    void SetNormalMapTexture(std::uint32_t material,
                             std::uint32_t normal) override {
        calls.push_back({'N', material, normal, 0, 0});
    }
    void SetNormalEnvTexture(std::uint32_t material,
                             std::uint32_t environment) override {
        calls.push_back({'E', material, environment, 0, 0});
    }
    void SetNormalEnvFrame(std::uint32_t material,
                           std::uint32_t frame) override {
        calls.push_back({'F', material, frame, 0, 0});
    }
    void SetNormalEnvCoefficient(std::uint32_t material,
                                 float coefficient) override {
        calls.push_back({'C', material,
                         std::bit_cast<std::uint32_t>(coefficient), 0, 0});
    }
    void ModulateNormalEnv(std::uint32_t material,
                           std::uint32_t enabled) override {
        calls.push_back({'M', material, enabled, 0, 0});
    }
    void SetupMatFxAtomic(std::uint32_t atomic) override {
        calls.push_back({'P', atomic, 0, 0, 0});
    }
    void SetMatFxEffects(std::uint32_t material,
                         std::uint32_t mode) override {
        calls.push_back({'X', material, mode, 0, 0});
    }
    void SetupMatFxEnvironment(
        std::uint32_t material, std::uint32_t normal,
        std::uint32_t frame, std::uint32_t unused,
        std::uint32_t coefficient_bits) override {
        Check(unused == 0, "environment setup passes zero fourth argument");
        calls.push_back({'V', material, normal,
                         frame, coefficient_bits});
    }
    void SetupMatFxDualTexture(
        std::uint32_t material, std::uint32_t texture_value,
        std::uint32_t blend_mode, std::uint32_t flags) override {
        calls.push_back({'D', material, texture_value,
                         blend_mode, flags});
    }
    void SetMatFxUvTransforms(
        std::uint32_t material, std::uint32_t first,
        std::uint32_t second) override {
        calls.push_back({'U', material, first, second, 0});
    }
};

}  // namespace

int main() {
    Runtime runtime;
    Check(pal4::RwAtomicMaterialCallbacks::SetPipeline(
              0x2000u, 0x1234u, runtime) == 0x2000u &&
              runtime.words[0x206cu] == 0x1234u &&
              runtime.calls == std::vector<Call>{
                  {'W', 0x206cu, 0x1234u, 0, 0}},
          "pipeline callback writes atomic +108 and returns atomic");
    runtime.calls.clear();

    runtime.words[0x3000u] = 0xa000u;
    Check(pal4::RwAtomicMaterialCallbacks::SetBasicMaterialTextures(
              0x1000u, 0x3000u, runtime) == 0x1000u &&
              runtime.calls == std::vector<Call>{
                  {'N', 0x1000u, 0xa000u, 0, 0},
                  {'E', 0x1000u, 0, 0, 0}} &&
              runtime.texture.create_count == 0u,
          "basic callback reuses cached normal map and clears environment");

    runtime.calls.clear();
    runtime.words[0x1000u] = 0x6000u;
    runtime.words[0x3000u] = 0;
    runtime.words[0x3004u] = std::bit_cast<std::uint32_t>(0.5f);
    runtime.texture.words[0x6000u] = 0x7000u;
    runtime.texture.words[0x700cu] = 8u;
    runtime.texture.words[0x7010u] = 4u;
    runtime.texture.words[0x8004u] = 8u;
    runtime.texture.words[0x8008u] = 4u;
    Check(pal4::RwAtomicMaterialCallbacks::SetBasicMaterialTextures(
              0x1000u, 0x3000u, runtime) == 0x1000u &&
              runtime.texture.create_count == 1u &&
              runtime.words[0x3000u] == 0xa000u &&
              runtime.calls == std::vector<Call>{
                  {'W', 0x3000u, 0xa000u, 0, 0},
                  {'N', 0x1000u, 0xa000u, 0, 0},
                  {'E', 0x1000u, 0, 0, 0}},
          "cache miss invokes rebuilt texture and raster creation");
    runtime.calls.clear();
    runtime.words[0x3000u] = 0;
    runtime.texture.image = 0;
    Check(pal4::RwAtomicMaterialCallbacks::SetBasicMaterialTextures(
              0x1000u, 0x3000u, runtime) == 0x1000u &&
              runtime.calls == std::vector<Call>{
                  {'W', 0x3000u, 0, 0, 0}},
          "failed normal texture creation does not mutate material");
    runtime.texture.image = 0x8000u;

    runtime.calls.clear();
    runtime.words[0x3008u] = 1u;
    runtime.words[0x2018u] = 0x4000u;
    runtime.words[0x3000u] = 0xa000u;
    runtime.dispatch_material = true;
    Check(pal4::RwAtomicMaterialCallbacks::ApplyBasicNormalMap(
              0x2000u, 0x3000u, runtime) == 0x2000u &&
              runtime.calls == std::vector<Call>{
                  {'T', 0x2000u, 2u, 0, 0},
                  {'G', 0x4000u, 0u, 0x3000u, 0},
                  {'N', 0x1000u, 0xa000u, 0, 0},
                  {'E', 0x1000u, 0, 0, 0},
                  {'I', 0x2000u, 0, 0, 0}},
          "atomic normal-map path fixes skin type, visits material and initializes");
    runtime.calls.clear();
    runtime.words[0x2018u] = 0;
    Check(pal4::RwAtomicMaterialCallbacks::ApplyBasicNormalMap(
              0x2000u, 0x3000u, runtime) == 0x2000u &&
              runtime.calls == std::vector<Call>{
                  {'I', 0x2000u, 0, 0, 0}},
          "already-correct skin and absent geometry still initialize atomic");

    runtime.calls.clear();
    runtime.words[0x5010u] = 0;
    runtime.words[0x5018u] = 0;
    Check(pal4::RwAtomicMaterialCallbacks::SetExtendedMaterialTextures(
              0x1000u, 0x5000u, runtime) == 0x1000u &&
              runtime.calls.empty(),
          "extended callback without source or cache leaves material alone");
    runtime.words[0x5010u] = 0x6000u;
    runtime.words[0x5014u] = std::bit_cast<std::uint32_t>(0.75f);
    Check(pal4::RwAtomicMaterialCallbacks::SetExtendedMaterialTextures(
              0x1000u, 0x5000u, runtime) == 0x1000u &&
              runtime.words[0x5018u] == 0xa000u &&
              runtime.texture.create_count == 2u,
          "extended callback caches a generated normal texture");

    runtime.calls.clear();
    runtime.words[0x5000u] = 0x77u;
    runtime.words[0x5004u] = 0x88u;
    runtime.words[0x5008u] = std::bit_cast<std::uint32_t>(1.5f);
    runtime.words[0x500cu] = 1u;
    Check(pal4::RwAtomicMaterialCallbacks::SetExtendedMaterialTextures(
              0x1000u, 0x5000u, runtime) == 0x1000u &&
              runtime.calls == std::vector<Call>{
                  {'N', 0x1000u, 0xa000u, 0, 0},
                  {'E', 0x1000u, 0x88u, 0, 0},
                  {'F', 0x1000u, 0x77u, 0, 0},
                  {'C', 0x1000u,
                   std::bit_cast<std::uint32_t>(1.5f), 0, 0},
                  {'M', 0x1000u, 1u, 0, 0}},
          "extended callback applies cached normal and optional environment state");
    runtime.calls.clear();
    runtime.words[0x5004u] = 0;
    runtime.words[0x5008u] = 0;
    Check(pal4::RwAtomicMaterialCallbacks::SetExtendedMaterialTextures(
              0x1000u, 0x5000u, runtime) == 0x1000u &&
              runtime.calls == std::vector<Call>{
                  {'N', 0x1000u, 0xa000u, 0, 0},
                  {'F', 0x1000u, 0x77u, 0, 0},
                  {'M', 0x1000u, 1u, 0, 0}},
          "zero optional environment texture and coefficient are skipped");

    runtime.calls.clear();
    runtime.skin_type = 1u;
    runtime.words[0x501cu] = 1u;
    runtime.words[0x2018u] = 0x4000u;
    runtime.dispatch_material = false;
    Check(pal4::RwAtomicMaterialCallbacks::ApplyExtendedNormalMap(
              0x2000u, 0x5000u, runtime) == 0x2000u &&
              runtime.calls == std::vector<Call>{
                  {'T', 0x2000u, 2u, 0, 0},
                  {'G', 0x4000u, 1u, 0x5000u, 0},
                  {'I', 0x2000u, 0, 0, 0}},
          "extended atomic callback uses +28 skin flag and material traversal");

    runtime.calls.clear();
    runtime.words[0x5004u] = 0x88u;
    runtime.words[0x5008u] = std::bit_cast<std::uint32_t>(0.25f);
    Check(pal4::RwAtomicMaterialCallbacks::SetEnvironmentMaterial(
              0x1000u, 0x5000u, runtime) == 0x1000u &&
              runtime.calls == std::vector<Call>{
                  {'X', 0x1000u, 2u, 0, 0},
                  {'V', 0x1000u, 0x88u, 0x77u,
                   std::bit_cast<std::uint32_t>(0.25f)}},
          "environment material sets effect two and forwards all parameters");
    runtime.calls.clear();
    Check(pal4::RwAtomicMaterialCallbacks::ApplyEnvironmentMap(
              0x2000u, 0x5000u, runtime) == 0x2000u &&
              runtime.calls == std::vector<Call>{
                  {'P', 0x2000u, 0, 0, 0},
                  {'G', 0x4000u, 2u, 0x5000u, 0}},
          "environment atomic setup precedes material traversal");

    runtime.calls.clear();
    runtime.words[0x7000u] = 0;
    runtime.words[0x7004u] = 0;
    Check(pal4::RwAtomicMaterialCallbacks::SetMatFxMaterial(
              0x1000u, 0x7000u, runtime) == 0x1000u &&
              runtime.calls.empty(),
          "mode zero without secondary texture has no effect");
    runtime.words[0x7004u] = 0x99u;
    runtime.words[0x7008u] = 3u;
    runtime.words[0x700cu] = 4u;
    Check(pal4::RwAtomicMaterialCallbacks::SetMatFxMaterial(
              0x1000u, 0x7000u, runtime) == 0x1000u &&
              runtime.calls == std::vector<Call>{
                  {'X', 0x1000u, 4u, 0, 0},
                  {'D', 0x1000u, 0x99u, 3u, 4u}},
          "mode zero configures dual texture effect four");
    runtime.calls.clear();
    runtime.words[0x7000u] = 1u;
    runtime.words[0x7010u] = 0;
    Check(pal4::RwAtomicMaterialCallbacks::SetMatFxMaterial(
              0x1000u, 0x7000u, runtime) == 0x1000u &&
              runtime.calls.empty(),
          "mode one requires first UV matrix");
    runtime.words[0x7010u] = 0x111u;
    Check(pal4::RwAtomicMaterialCallbacks::SetMatFxMaterial(
              0x1000u, 0x7000u, runtime) == 0x1000u &&
              runtime.calls == std::vector<Call>{
                  {'X', 0x1000u, 5u, 0, 0},
                  {'U', 0x1000u, 0x111u, 0, 0}},
          "mode one configures UV transform effect five");
    runtime.calls.clear();
    runtime.words[0x7000u] = 2u;
    runtime.words[0x7014u] = 0;
    Check(pal4::RwAtomicMaterialCallbacks::SetMatFxMaterial(
              0x1000u, 0x7000u, runtime) == 0x1000u &&
              runtime.calls.empty(),
          "mode two requires texture and both UV matrices");
    runtime.words[0x7014u] = 0x222u;
    Check(pal4::RwAtomicMaterialCallbacks::SetMatFxMaterial(
              0x1000u, 0x7000u, runtime) == 0x1000u &&
              runtime.calls == std::vector<Call>{
                  {'X', 0x1000u, 6u, 0, 0},
                  {'D', 0x1000u, 0x99u, 3u, 4u},
                  {'U', 0x1000u, 0x111u, 0x222u, 0}},
          "mode two configures combined dual texture and UV effect six");
    runtime.calls.clear();
    runtime.words[0x7000u] = 3u;
    Check(pal4::RwAtomicMaterialCallbacks::SetMatFxMaterial(
              0x1000u, 0x7000u, runtime) == 0x1000u &&
              runtime.calls.empty(),
          "unknown MatFX mode leaves material unchanged");

    std::cout << "rw_atomic_material_callbacks_tests passed\n";
}
