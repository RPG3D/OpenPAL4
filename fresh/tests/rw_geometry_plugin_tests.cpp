#include "pal4/rw_geometry_plugin.h"

#include <cstdlib>
#include <algorithm>
#include <cstring>
#include <iostream>
#include <limits>
#include <map>
#include <string>
#include <utility>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

class Runtime final : public pal4::IRwGeometryPluginRuntime {
public:
    std::int32_t RegisterEnginePlugin(std::uint32_t bytes,
                                      std::uint32_t id) override {
        Check(bytes == 1 && id == 8449, "engine registration arguments");
        calls.push_back("engine");
        return engine_result;
    }
    std::int32_t RegisterGeometryPlugin(std::uint32_t bytes,
                                        std::uint32_t id) override {
        Check(bytes == 12 && id == 8449, "geometry registration arguments");
        calls.push_back("geometry");
        return geometry_result;
    }
    std::int32_t RegisterGeometryStream(std::uint32_t id) override {
        Check(id == 8449, "stream registration identifier");
        calls.push_back("stream");
        return stream_result;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.push_back("read");
        return words.at(address);
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        calls.push_back("write");
        words[address] = value;
    }
    void ForAllAtomics(std::uint32_t clump_address,
                       std::uint32_t atomic_callback,
                       pal4::RwAtomicProcessContext* context) override {
        Check(clump_address == 300, "RenderWare receives clump");
        if (atomic_callback == 0x65fd70u) {
            Check(context != nullptr && context->material_match == 0 &&
                      context->geometry_callback == 0x65fab0u &&
                      context->payload == 45,
                  "traversal context matches the three native words");
            context->material_match = 7;
        } else {
            Check(atomic_callback == 0x660060u && context == nullptr,
                  "restore traversal uses ProcessAtomic and null context");
        }
        calls.push_back("atomics");
    }
    void ResyncInterpolatedSphere(std::uint32_t atomic_address) override {
        Check(atomic_address == 300, "sphere resync receives atomic");
        calls.push_back("resync");
    }
    std::uint32_t GetObjectTransform(std::uint32_t frame_address) override {
        Check(frame_address == 800 || frame_address == 720,
              "transform receives atomic or camera frame");
        calls.push_back("transform");
        return frame_address == 800 ? 1000 : 900;
    }
    pal4::RwPluginVector3 CalculatePosition(
        std::uint32_t sphere_address, std::uint32_t transform_address) override {
        Check(sphere_address == 328 && transform_address == 1000,
              "sphere center uses the atomic transform");
        calls.push_back("position");
        return {2.5f, 2.5f, 0.0f};
    }
    void ForAllMaterials(std::uint32_t geometry_address,
                         std::uint32_t material_callback,
                         pal4::RwAtomicProcessContext& context) override {
        Check(geometry_address == 600 && material_callback == 0x65fea0u,
              "material enumeration arguments");
        calls.push_back("materials");
        context.material_match = material_result;
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        calls.push_back("byte");
        return bytes.at(address);
    }
    std::uint32_t CreateImage(std::uint32_t width, std::uint32_t height,
                              std::uint32_t depth) override {
        calls.push_back("create_image");
        image_arguments = {width, height, depth};
        words[20016] = 12;
        words[20020] = 22000;
        words[20024] = 22000;
        return image_create_result;
    }
    void AllocateImagePixels(std::uint32_t image_address) override {
        Check(image_address == 20000, "pixel allocation receives image");
        calls.push_back("allocate_pixels");
    }
    void SetImageFromRaster(std::uint32_t image_address,
                            std::uint32_t raster_address) override {
        Check(image_address == 20000 && raster_address == 21000,
              "image conversion receives source raster");
        calls.push_back("raster_to_image");
    }
    void DestroyImage(std::uint32_t image_address) override {
        Check(image_address == 20000, "image destruction receives image");
        calls.push_back("destroy_image");
    }
    std::uint32_t DefaultAtomicRenderCallback() override {
        calls.push_back("default");
        return 0x1234u;
    }
    std::uint32_t AllocateBytes(std::uint32_t bytes) override {
        calls.push_back("alloc");
        allocation_sizes.push_back(bytes);
        const auto result = next_allocation;
        next_allocation += 256;
        return result;
    }
    void FreeBytes(std::uint32_t address) override {
        calls.push_back("free");
        freed.push_back(address);
    }
    void InvokeAtomicRenderCallback(std::uint32_t callback_address,
                                    std::uint32_t atomic_address) override {
        calls.push_back("invoke");
        callbacks.push_back({callback_address, atomic_address});
    }
    std::optional<bool> CompressedTextureHasAlpha(
        std::uint32_t texture) override {
        Check(texture == 800u, "compressed texture query address");
        if (compressed_alpha) calls.push_back("compressed");
        return compressed_alpha;
    }

    std::int32_t engine_result = 4;
    std::int32_t geometry_result = 16;
    std::int32_t stream_result = 16;
    std::vector<std::string> calls;
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::uint32_t> allocation_sizes;
    std::vector<std::uint32_t> freed;
    std::vector<std::pair<std::uint32_t, std::uint32_t>> callbacks;
    std::uint32_t next_allocation = 1000;
    std::uint32_t material_result = 1;
    std::uint32_t image_create_result = 20000;
    std::optional<bool> compressed_alpha;
    std::vector<std::uint32_t> image_arguments;
};

void Registration() {
    pal4::RwGeometryPluginState state;
    Runtime runtime;
    Check(pal4::RwGeometryPlugin::Register(state, runtime), "successful registration");
    Check(state.engine_offset == 4 && state.geometry_offset == 16,
          "registered offsets retained");
    Check(runtime.calls == std::vector<std::string>({"engine", "geometry", "stream"}),
          "registration order");

    runtime.calls.clear();
    runtime.engine_result = -1;
    Check(!pal4::RwGeometryPlugin::Register(state, runtime), "engine failure");
    Check(runtime.calls == std::vector<std::string>({"engine"}),
          "engine failure short circuits");
    runtime.engine_result = 0;
    runtime.geometry_result = -1;
    runtime.calls.clear();
    Check(!pal4::RwGeometryPlugin::Register(state, runtime), "geometry failure");
    Check(runtime.calls == std::vector<std::string>({"engine", "geometry"}),
          "geometry failure short circuits");
    runtime.geometry_result = 16;
    runtime.stream_result = 12;
    Check(!pal4::RwGeometryPlugin::Register(state, runtime), "stream offset mismatch");
}

void ClearAndCopy() {
    Runtime runtime;
    pal4::RwGeometryPluginState state{4, 16};
    runtime.words = {{116, 1}, {120, 2}, {124, 3}};
    Check(pal4::RwGeometryPlugin::ConstructData(state, 100, runtime) == 100,
          "constructor returns object");
    Check(runtime.words[116] == 0 && runtime.words[120] == 0 &&
              runtime.words[124] == 0, "constructor clears three fields");
    runtime.words[116] = 8;
    Check(pal4::RwGeometryPlugin::DestructData(state, 100, runtime) == 100 &&
              runtime.words[116] == 0, "destructor clears fields");
    state.geometry_offset = 0;
    runtime.calls.clear();
    Check(pal4::RwGeometryPlugin::ConstructData(state, 100, runtime) == 100 &&
              pal4::RwGeometryPlugin::DestructData(state, 100, runtime) == 100,
          "zero-offset lifecycle still returns object");
    Check(runtime.calls.empty(), "zero offset does not clear");

    state.geometry_offset = 16;
    runtime.words = {{216, 11}, {220, 22}, {224, 33}};
    runtime.calls.clear();
    Check(pal4::RwGeometryPlugin::CopyData(state, 100, 200, runtime) == 100,
          "copy returns destination");
    Check(runtime.words[116] == 11 && runtime.words[120] == 22 &&
              runtime.words[124] == 33, "copy transfers three fields");
    Check(runtime.calls == std::vector<std::string>({"read", "write", "read", "write", "read", "write"}),
          "copy reads and writes in field order");
}

void ProcessClump() {
    Runtime runtime;
    pal4::RwGeometryPluginState state{4, 16};
    Check(pal4::RwGeometryPlugin::ProcessClumpAtomics(state, 300, 45, runtime) == 7,
          "traversal returns callback-updated material result");
    Check(runtime.calls == std::vector<std::string>({"atomics"}),
          "traversal calls RenderWare once");
    state.geometry_offset = 0;
    runtime.calls.clear();
    Check(pal4::RwGeometryPlugin::ProcessClumpAtomics(state, 300, 45, runtime) == 0 &&
              runtime.calls.empty(), "geometry offset gate");
    state.geometry_offset = 16;
    state.engine_offset = -1;
    Check(pal4::RwGeometryPlugin::ProcessClumpAtomics(state, 300, 45, runtime) == 0 &&
              runtime.calls.empty(), "engine offset gate");
}

void ProcessAtomic() {
    Runtime runtime;
    pal4::RwGeometryPluginState state{4, 16};
    runtime.words[320] = 10;
    runtime.words[324] = 500;
    runtime.words[376] = 0;
    runtime.words[304] = 800;
    runtime.words[0x950cd0u] = 700;
    runtime.words[700] = 704;
    runtime.words[708] = 720;
    runtime.words[948] = 0;
    runtime.words[952] = 0;
    runtime.words[956] = 0;
    Check(pal4::RwGeometryPlugin::GetAtomicGeometryData(state, 300, runtime) == 10 &&
              pal4::RwGeometryPlugin::GetAtomicGeometryDataArray(state, 300, runtime) == 500,
          "both getters read their observed plugin fields");
    state.geometry_offset = 0;
    runtime.calls.clear();
    Check(pal4::RwGeometryPlugin::GetAtomicGeometryData(state, 300, runtime) == 0 &&
              pal4::RwGeometryPlugin::GetAtomicGeometryDataArray(state, 300, runtime) == 0 &&
              runtime.calls.empty(), "getters gate on geometry offset");
    state.geometry_offset = 16;
    state.engine_offset = 0;
    Check(pal4::RwGeometryPlugin::GetAtomicGeometryData(state, 300, runtime) == 0 &&
              pal4::RwGeometryPlugin::GetAtomicGeometryDataArray(state, 300, runtime) == 0 &&
              runtime.calls.empty(), "getters gate on engine offset");
    state.engine_offset = 4;
    runtime.calls.clear();
    Check(pal4::RwGeometryPlugin::CalculateDistanceToCamera(300, runtime) == 12.5f,
          "distance helper computes squared world distance");
    Check(runtime.calls.front() == "read" && runtime.calls.back() == "read",
          "distance reads atomic and camera state");
    runtime.words[376] = 2;
    runtime.calls.clear();
    Check(pal4::RwGeometryPlugin::CalculateDistanceToCamera(300, runtime) == 12.5f &&
              runtime.calls[1] == "resync",
          "interpolated sphere is resynchronized before its position is read");
    runtime.words[376] = 0;
    runtime.words[504] = 0;
    runtime.words[508] = 0;
    runtime.words[512] = 0;
    Check(pal4::RwGeometryPlugin::ProcessAtomicGeometry(state, 300, runtime) == 300,
          "first insertion returns atomic");
    Check(runtime.allocation_sizes == std::vector<std::uint32_t>({8}) &&
              runtime.words[504] == 1000 && runtime.words[508] == 1008 &&
              runtime.words[512] == 1008 && runtime.words[1000] == 300,
          "empty array allocates one record");
    float distance = 0;
    auto bits = runtime.words[1004];
    std::memcpy(&distance, &bits, sizeof(distance));
    Check(distance == 12.5f, "distance stored as float bits");

    Check(pal4::RwGeometryPlugin::ProcessAtomicGeometry(state, 300, runtime) == 300,
          "second insertion returns atomic");
    Check(runtime.allocation_sizes == std::vector<std::uint32_t>({8, 16}) &&
              runtime.freed == std::vector<std::uint32_t>({0, 1000}) &&
              runtime.words[1256] == 300 && runtime.words[1264] == 300,
          "full one-element array grows to two and preserves old record");

    runtime.words[512] = 1280;
    runtime.calls.clear();
    Check(pal4::RwGeometryPlugin::ProcessAtomicGeometry(state, 300, runtime) == 300 &&
              runtime.words[508] == 1280,
          "spare capacity appends without allocation");
    Check(runtime.allocation_sizes.size() == 2,
          "spare capacity reuses the allocation");

    Check(pal4::RwGeometryPlugin::ProcessAtomicGeometry(state, 300, runtime) == 300 &&
              runtime.allocation_sizes ==
                  std::vector<std::uint32_t>({8, 16, 48}) &&
              runtime.freed.back() == 1256 &&
              runtime.words[504] == 1512 && runtime.words[508] == 1544 &&
              runtime.words[512] == 1560 &&
              runtime.words[1512] == 300 && runtime.words[1520] == 300 &&
              runtime.words[1528] == 300 && runtime.words[1536] == 300,
          "three-record array doubles and preserves all records");

    runtime.calls.clear();
    state.engine_offset = 0;
    Check(pal4::RwGeometryPlugin::ProcessAtomicGeometry(state, 300, runtime) == 300 &&
              runtime.calls.empty(), "disabled plugin bypasses processing");
    state.engine_offset = 4;
    runtime.words[320] = 0;
    runtime.calls.clear();
    Check(pal4::RwGeometryPlugin::ProcessAtomicGeometry(state, 300, runtime) == 300 &&
              runtime.calls == std::vector<std::string>({"read"}),
          "missing geometry data bypasses distance and array");
    runtime.words[320] = 10;
    runtime.words[324] = 0;
    runtime.calls.clear();
    Check(pal4::RwGeometryPlugin::ProcessAtomicGeometry(state, 300, runtime) == 300 &&
              runtime.calls.front() == "read" &&
              runtime.calls.back() == "read" &&
              runtime.allocation_sizes.size() == 3,
          "missing pair array is checked after distance");
}

void ProcessMaterial() {
    Runtime runtime;
    pal4::RwGeometryPluginState state{4, 100};
    pal4::RwAtomicProcessContext context{0, 0x65fab0u, 9};
    runtime.words[372] = 0x8888;
    runtime.words[324] = 600;
    runtime.words[608] = 0;
    runtime.words[404] = 0;
    Check(pal4::RwGeometryPlugin::ProcessAtomicMaterial(state, 300, context,
                                                         runtime) == 300 &&
              context.material_match == 1 &&
              runtime.words[400] == 0x8888 && runtime.words[404] == 1 &&
              runtime.words[408] == 9 && runtime.words[372] == 0x65fab0u,
          "material match saves callback and installs geometry processing");
    runtime.words[372] = 0x7777;
    runtime.calls.clear();
    Check(pal4::RwGeometryPlugin::ProcessAtomicMaterial(state, 300, context,
                                                         runtime) == 300 &&
              runtime.words[372] == 0x7777,
          "existing geometry data prevents callback replacement");
    runtime.words[404] = 0;
    runtime.words[372] = 0x65fab0u;
    Check(pal4::RwGeometryPlugin::ProcessAtomicMaterial(state, 300, context,
                                                         runtime) == 300 &&
              runtime.words[372] == 0x65fab0u,
          "matching callback bypasses plugin writes");
    runtime.words[372] = 0x7777;
    runtime.material_result = 0;
    Check(pal4::RwGeometryPlugin::ProcessAtomicMaterial(state, 300, context,
                                                         runtime) == 300 &&
              runtime.words[372] == 0x7777,
          "unmatched material bypasses callback replacement");
    runtime.material_result = 1;
    context.geometry_callback = 0;
    Check(pal4::RwGeometryPlugin::ProcessAtomicMaterial(state, 300, context,
                                                         runtime) == 300 &&
              runtime.words[372] == 0x1234u,
          "zero callback resolves to RenderWare default");
}

void AtomicSetters() {
    Runtime runtime;
    pal4::RwGeometryPluginState state{4, 16};
    Check(pal4::RwGeometryPlugin::SetAtomicGeometryData(state, 300, 11, runtime) == 16 &&
              pal4::RwGeometryPlugin::SetAtomicGeometryFlag(state, 300, 22, runtime) == 16 &&
              pal4::RwGeometryPlugin::SetAtomicGeometryPayload(state, 300, 33, runtime) == 16 &&
              runtime.words[316] == 11 && runtime.words[320] == 22 &&
              runtime.words[324] == 33,
          "setters write three observed plugin fields");
    runtime.calls.clear();
    state.engine_offset = 0;
    Check(pal4::RwGeometryPlugin::SetAtomicGeometryData(state, 300, 44, runtime) == 16 &&
              pal4::RwGeometryPlugin::SetAtomicGeometryFlag(state, 300, 44, runtime) == 16 &&
              pal4::RwGeometryPlugin::SetAtomicGeometryPayload(state, 300, 44, runtime) == 16 &&
              runtime.calls.empty(), "engine gate suppresses writes but retains result");
    state.engine_offset = 4;
    state.geometry_offset = -1;
    Check(pal4::RwGeometryPlugin::SetAtomicGeometryData(state, 300, 44, runtime) == -1 &&
              pal4::RwGeometryPlugin::SetAtomicGeometryFlag(state, 300, 44, runtime) == -1 &&
              pal4::RwGeometryPlugin::SetAtomicGeometryPayload(state, 300, 44, runtime) == -1 &&
              runtime.calls.empty(), "geometry gate returns its negative offset");
}

void MaterialAlpha() {
    Runtime runtime;
    pal4::RwAtomicProcessContext context{0, 0x65fab0u, 0};
    runtime.words[324] = 600;
    runtime.words[608] = 0;
    Check(!pal4::RwGeometryPlugin::CheckMaterialAlpha(600, runtime),
          "geometry without color flag has no vertex alpha");
    runtime.words[608] = 8;
    runtime.words[620] = 0;
    Check(!pal4::RwGeometryPlugin::CheckMaterialAlpha(600, runtime),
          "empty color array has no vertex alpha");
    runtime.words[620] = 2;
    runtime.words[648] = 10000;
    runtime.words[10000] = 0xffffffffu;
    runtime.words[10004] = 0x7fffffffu;
    Check(pal4::RwGeometryPlugin::CheckMaterialAlpha(600, runtime),
          "vertex alpha scans backward and finds a translucent color");
    runtime.calls.clear();
    Check(pal4::RwGeometryPlugin::GetAtomicGeometryMaterial(300, context,
                                                             runtime) == 300 &&
              context.material_match == 1 &&
              runtime.calls.end() ==
                  std::find(runtime.calls.begin(), runtime.calls.end(), "materials"),
          "geometry alpha skips material enumeration");
    runtime.words[10004] = 0xffffffffu;
    runtime.calls.clear();
    Check(!pal4::RwGeometryPlugin::CheckMaterialAlpha(600, runtime),
          "opaque colors do not match");
    Check(pal4::RwGeometryPlugin::GetAtomicGeometryMaterial(300, context,
                                                             runtime) == 300 &&
              context.material_match == 1 &&
              runtime.calls.back() == "materials",
          "opaque geometry delegates to material enumeration");

    runtime.words[704] = 0xff000000u;
    runtime.words[700] = 800;
    runtime.words[800] = 21000;
    runtime.bytes[21035] = 1;
    runtime.words[21012] = 2;
    runtime.words[21016] = 1;
    runtime.bytes[22003] = 0xff;
    runtime.bytes[22007] = 0x7f;
    context.material_match = 0;
    Check(pal4::RwGeometryPlugin::ProcessMaterial(700, context, runtime) == 700 &&
              context.material_match == 1 && runtime.calls.back() == "destroy_image",
          "opaque material checks texture image alpha");
    runtime.words[704] = 0x7f000000u;
    context.material_match = 0;
    runtime.calls.clear();
    Check(pal4::RwGeometryPlugin::ProcessMaterial(700, context, runtime) == 700 &&
              context.material_match == 1 && runtime.calls.size() == 1,
          "translucent material alpha matches without image scan");
    runtime.words[704] = 0xff000000u;
    runtime.words[700] = 0;
    context.material_match = 0;
    Check(pal4::RwGeometryPlugin::ProcessMaterial(700, context, runtime) == 700 &&
              context.material_match == 0,
          "opaque untextured material leaves result clear");
    runtime.words[700] = 800;
    context.material_match = 1;
    runtime.calls.clear();
    Check(pal4::RwGeometryPlugin::ProcessMaterial(700, context, runtime) == 700 &&
              context.material_match == 1 &&
              std::find(runtime.calls.begin(), runtime.calls.end(), "create_image") ==
                  runtime.calls.end(),
          "existing match skips texture image scan");
}

void ImageAlpha() {
    Runtime runtime;
    runtime.words[800] = 0;
    Check(!pal4::RwGeometryPlugin::ProcessImageAlpha(800, runtime),
          "missing raster bypasses image creation");
    runtime.words[800] = 21000;
    runtime.compressed_alpha = true;
    Check(pal4::RwGeometryPlugin::ProcessImageAlpha(800, runtime) &&
              runtime.calls.back() == "compressed",
          "compressed Alpha bypasses unsupported image conversion");
    runtime.compressed_alpha = false;
    Check(!pal4::RwGeometryPlugin::ProcessImageAlpha(800, runtime) &&
              runtime.calls.back() == "compressed",
          "opaque compressed texture bypasses image conversion");
    runtime.compressed_alpha.reset();
    runtime.bytes[21035] = 2;
    Check(!pal4::RwGeometryPlugin::ProcessImageAlpha(800, runtime) &&
              std::find(runtime.calls.begin(), runtime.calls.end(), "create_image") ==
                  runtime.calls.end(),
          "unsupported format bypasses RenderWare image calls");

    runtime.bytes[21035] = 1;
    runtime.words[21012] = 2;
    runtime.words[21016] = 2;
    runtime.bytes[22003] = 0xff;
    runtime.bytes[22007] = 0xff;
    runtime.bytes[22015] = 0xff;
    runtime.bytes[22019] = 0x7f;
    runtime.calls.clear();
    Check(pal4::RwGeometryPlugin::ProcessImageAlpha(800, runtime) &&
              runtime.image_arguments ==
                  std::vector<std::uint32_t>({2, 2, 32}) &&
              runtime.calls.back() == "destroy_image",
          "linear scan respects row stride and releases image on match");
    runtime.bytes[22019] = 0xff;
    runtime.calls.clear();
    Check(!pal4::RwGeometryPlugin::ProcessImageAlpha(800, runtime) &&
              runtime.calls.back() == "destroy_image",
          "opaque image releases image after full scan");
    runtime.image_create_result = 0;
    runtime.calls.clear();
    Check(!pal4::RwGeometryPlugin::ProcessImageAlpha(800, runtime) &&
              runtime.calls.back() == "create_image",
          "failed image creation skips pixel conversion and destruction");
    runtime.image_create_result = 20000;

    runtime.bytes[21035] = 0x21;
    runtime.words[21020] = 2;
    runtime.bytes[22003] = 0xff;
    runtime.bytes[22007] = 0xff;
    runtime.bytes[22011] = 0xff;
    runtime.bytes[22015] = 0x80;
    runtime.calls.clear();
    Check(pal4::RwGeometryPlugin::ProcessImageAlpha(800, runtime) &&
              runtime.image_arguments ==
                  std::vector<std::uint32_t>({2, 2, 2}) &&
              runtime.calls.back() == "destroy_image",
          "indexed scan checks two-to-the-depth alpha entries");
    runtime.bytes[22015] = 0xff;
    Check(!pal4::RwGeometryPlugin::ProcessImageAlpha(800, runtime),
          "fully opaque indexed palette does not match");
}

void RestoreAtomics() {
    Runtime runtime;
    pal4::RwGeometryPluginState state{4, 100};
    Check(pal4::RwGeometryPlugin::ProcessClumpAtomicsWrapper(state, 300,
                                                              runtime) &&
              runtime.calls == std::vector<std::string>({"atomics"}),
          "restore wrapper traverses clump once");
    runtime.calls.clear();
    state.engine_offset = 0;
    Check(!pal4::RwGeometryPlugin::ProcessClumpAtomicsWrapper(state, 300,
                                                               runtime) &&
              runtime.calls.empty(), "restore wrapper gates engine offset");
    state.engine_offset = 4;
    state.geometry_offset = 0;
    Check(!pal4::RwGeometryPlugin::ProcessClumpAtomicsWrapper(state, 300,
                                                               runtime) &&
              runtime.calls.empty(), "restore wrapper gates geometry offset");
    state.geometry_offset = 100;

    runtime.words[400] = 0x999;
    runtime.words[404] = 1;
    runtime.words[408] = 500;
    runtime.words[504] = 1000;
    runtime.words[508] = 1024;
    runtime.words[1000] = 111;
    runtime.words[1004] = 10;
    runtime.words[1008] = 300;
    runtime.words[1012] = 20;
    runtime.words[1016] = 222;
    runtime.words[1020] = 30;
    Check(pal4::RwGeometryPlugin::GetAtomicGeometryFlag(state, 300, runtime) ==
              0x999,
          "flag getter reads first plugin DWORD");
    Check(pal4::RwGeometryPlugin::ProcessAtomic(state, 300, runtime) == 300 &&
              runtime.words[404] == 0 && runtime.words[372] == 0x999 &&
              runtime.words[508] == 1016 && runtime.words[1000] == 111 &&
              runtime.words[1008] == 222 && runtime.words[1012] == 30,
          "restore removes middle pair and shifts following pair");

    runtime.words[404] = 1;
    runtime.words[400] = 0;
    runtime.words[508] = 1016;
    runtime.words[1000] = 300;
    Check(pal4::RwGeometryPlugin::ProcessAtomic(state, 300, runtime) == 300 &&
              runtime.words[372] == 0x1234 && runtime.words[508] == 1008 &&
              runtime.words[1000] == 222,
          "zero prior callback resolves default and removes first pair");
    runtime.words[404] = 1;
    runtime.words[408] = 0;
    Check(pal4::RwGeometryPlugin::ProcessAtomic(state, 300, runtime) == 300 &&
              runtime.words[404] == 0 && runtime.words[372] == 0x1234,
          "missing array still restores callback");
    runtime.words[404] = 0;
    runtime.calls.clear();
    Check(pal4::RwGeometryPlugin::ProcessAtomic(state, 300, runtime) == 300 &&
              runtime.calls == std::vector<std::string>({"read"}),
          "unmarked atomic bypasses restoration");
    state.geometry_offset = 0;
    runtime.calls.clear();
    Check(pal4::RwGeometryPlugin::GetAtomicGeometryFlag(state, 300, runtime) == 0 &&
              runtime.calls.empty(), "flag getter gates disabled plugin");
}

void CompareRecords() {
    Runtime runtime;
    const auto store_float = [&runtime](const std::uint32_t address, float value) {
        std::uint32_t bits = 0;
        std::memcpy(&bits, &value, sizeof(bits));
        runtime.words[address] = bits;
    };
    store_float(1004, 9.0f);
    store_float(1012, 3.0f);
    Check(pal4::RwGeometryPlugin::CompareAtomicGeometryFlags(1000, 1008,
                                                              runtime) &&
              !pal4::RwGeometryPlugin::CompareAtomicGeometryFlags(1008, 1000,
                                                                    runtime),
          "distance comparator is strict descending");
    store_float(1012, 9.0f);
    Check(!pal4::RwGeometryPlugin::CompareAtomicGeometryFlags(1000, 1008,
                                                               runtime),
          "equal distances do not compare greater");
    store_float(1012, std::numeric_limits<float>::quiet_NaN());
    Check(!pal4::RwGeometryPlugin::CompareAtomicGeometryFlags(1000, 1008,
                                                               runtime) &&
              !pal4::RwGeometryPlugin::CompareAtomicGeometryFlags(1008, 1000,
                                                                    runtime),
          "unordered distances do not compare greater");
}

void SortRecords() {
    const auto store_float = [](Runtime& runtime, const std::uint32_t address,
                                const float value) {
        std::uint32_t bits = 0;
        std::memcpy(&bits, &value, sizeof(bits));
        runtime.words[address] = bits;
    };
    Runtime runtime;
    pal4::RwGeometryPluginState state{4, 100};
    runtime.words[504] = 10000;
    runtime.words[508] = 10024;
    const std::vector<std::uint32_t> atomics{300, 500, 700};
    const std::vector<float> distances{2.0f, 7.0f, 4.0f};
    for (std::uint32_t index = 0; index < atomics.size(); ++index) {
        runtime.words[10000 + 8 * index] = atomics[index];
        store_float(runtime, 10004 + 8 * index, distances[index]);
        runtime.words[atomics[index] + 100] = 0xabc + index;
    }
    Check(pal4::RwGeometryPlugin::SortAtomicGeometries(state, 500, runtime) ==
              10024 &&
              runtime.words[10000] == 500 && runtime.words[10008] == 700 &&
              runtime.words[10016] == 300 && runtime.words[508] == 10000,
          "short array sorts descending and clears finish pointer");
    Check(runtime.callbacks ==
              std::vector<std::pair<std::uint32_t, std::uint32_t>>(
                  {{0xabd, 500}, {0xabe, 700}, {0xabc, 300}}),
          "sorted atomics invoke saved callbacks in distance order");
    runtime.callbacks.clear();
    Check(pal4::RwGeometryPlugin::SortAtomicGeometries(state, 500, runtime) == 0 &&
              runtime.callbacks.empty(), "empty initialized array returns zero");
    runtime.words[504] = 0;
    Check(pal4::RwGeometryPlugin::SortAtomicGeometries(state, 500, runtime) == 4,
          "missing begin returns engine offset");
    state.engine_offset = -2;
    Check(pal4::RwGeometryPlugin::SortAtomicGeometries(state, 500, runtime) == -2,
          "sort returns disabled engine offset");
    state.geometry_offset = -3;
    Check(pal4::RwGeometryPlugin::SortAtomicGeometries(state, 500, runtime) == -3,
          "sort returns disabled geometry offset");

    state = {4, 100};
    runtime.words[504] = 10000;
    runtime.words[508] = 10024;
    Check(pal4::RwGeometryPlugin::ClearAtomicGeometryArray(state, 500,
                                                            runtime) == 10024 &&
              runtime.words[508] == 10000,
          "standalone clear returns previous end and resets finish");
    state.engine_offset = 0;
    runtime.words[508] = 10024;
    Check(pal4::RwGeometryPlugin::ClearAtomicGeometryArray(state, 500,
                                                            runtime) == 0 &&
              runtime.words[508] == 10024,
          "standalone clear respects engine gate");

    Runtime large;
    state = {4, 100};
    constexpr std::uint32_t count = 24;
    large.words[504] = 10000;
    large.words[508] = 10000 + 8 * count;
    for (std::uint32_t index = 0; index < count; ++index) {
        const auto atomic = 300 + index * 200;
        large.words[10000 + 8 * index] = atomic;
        store_float(large, 10004 + 8 * index,
                    static_cast<float>((index * 17u) % count));
        large.words[atomic + 100] = index % 4u == 0u ? 0u : 0x900u + index;
    }
    Check(pal4::RwGeometryPlugin::SortAtomicGeometries(state, 500, large) ==
              static_cast<std::int32_t>(10000 + 8 * count) &&
              large.words[508] == 10000,
          "large array uses hybrid sort and clears finish pointer");
    for (std::uint32_t index = 1; index < count; ++index)
        Check(!pal4::RwGeometryPlugin::CompareAtomicGeometryFlags(
                  10000 + 8 * index, 10000 + 8 * (index - 1), large),
              "large array is sorted in descending distance order");
    Check(large.callbacks.size() == 18,
          "zero callback entries are skipped after sorting");

    Runtime insertion;
    insertion.words[1000] = 1;
    insertion.words[1008] = 2;
    insertion.words[1016] = 3;
    store_float(insertion, 1004, 1.0f);
    store_float(insertion, 1012, 3.0f);
    store_float(insertion, 1020, 2.0f);
    Check(pal4::RwGeometryPlugin::InsertionSortAtomicGeometries(
              1000, 1024, insertion) == 1024 &&
              insertion.words[1000] == 2 && insertion.words[1008] == 3 &&
              insertion.words[1016] == 1,
          "standalone insertion helper sorts complete eight-byte records");
}

}  // namespace

int main() {
    Registration();
    ClearAndCopy();
    ProcessClump();
    ProcessAtomic();
    ProcessMaterial();
    AtomicSetters();
    MaterialAlpha();
    ImageAlpha();
    RestoreAtomics();
    CompareRecords();
    SortRecords();
}
