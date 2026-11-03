#include "pal4/world_sector_vertex_instancer.h"

#include <algorithm>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

constexpr std::uint32_t kSector = 0x1000u;
constexpr std::uint32_t kHeader = 0x2000u;
constexpr std::uint32_t kMapped = 0xa000u;

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IWorldSectorVertexInstancerRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::vector<pal4::WorldSectorVertexElement> declaration;
    std::vector<std::string> calls;
    std::uint32_t flags = 0u;
    std::uint32_t usage_flags = 0u;
    std::uint32_t create_result = 1u;
    std::uint32_t position_width = 12u;
    bool compact = false;
    std::uint32_t last_process_texcoords = 0u;

    Runtime() {
        words[kSector + 8u] = 0x1100u;
        words[kSector + 12u] = 0x1200u;
        words[kSector + 16u] = 0x1300u;
        words[kSector + 20u] = 0u;
        words[kSector + 48u] = 0x1400u;
        words[kHeader + 4u] = 2u;
        words[kHeader + 68u] = 1u;
        words[kHeader + 84u] = 3u;
        words[kHeader + 88u] = 2u;
        words[kHeader + 104u] = 2u;
        words[kHeader + 120u] = 5u;
        words[kHeader + 124u] = 3u;
    }
    std::uint32_t VertexCapsAddress() override {
        calls.emplace_back("caps");
        return 0x5000u;
    }
    std::uint32_t WorldFlags() override { return flags; }
    std::uint32_t WorldSectorUsageFlags(std::uint32_t sector) override {
        Check(sector == kSector, "world sector");
        calls.emplace_back("anim");
        return usage_flags;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    std::uint16_t ReadHalf(std::uint32_t address) override {
        Check(address == kSector + 130u, "vertex count address");
        return 5u;
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        Check(address == 0x50ecu, "compact format capability address");
        return compact ? 0x80u : 0u;
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
    }
    void WriteHalf(std::uint32_t address, std::uint16_t value) override {
        Check(address == kHeader + 28u, "attribute flags address");
        words[address] = value;
    }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        Check(address == kHeader + 30u && value == 1u,
              "vertex buffer marker");
        calls.emplace_back("marker");
    }
    void CreateVertexDeclaration(
        std::span<const pal4::WorldSectorVertexElement> elements,
        std::uint32_t output) override {
        Check(output == kHeader + 52u, "declaration output address");
        declaration.assign(elements.begin(), elements.end());
        calls.emplace_back("declaration");
    }
    std::uint32_t CreateVertexBuffer(
        std::uint32_t stride, std::uint32_t bytes,
        std::uint32_t buffer_address,
        std::uint32_t offset_address) override {
        Check(stride == words[kHeader + 24u] && bytes == stride * 5u &&
                  buffer_address == kHeader + 16u &&
                  offset_address == kHeader + 20u,
              "vertex buffer arguments");
        calls.emplace_back("create-buffer");
        words[buffer_address] = 0x9000u;
        words[offset_address] = stride * 2u;
        return create_result;
    }
    std::uint32_t LockVertexBuffer(
        std::uint32_t buffer, std::uint32_t offset,
        std::uint32_t bytes, std::uint32_t lock_flags) override {
        Check(buffer == 0x9000u &&
                  offset == words[kHeader + 24u] * 2u &&
                  bytes == words[kHeader + 24u] * 5u &&
                  lock_flags == 0u, "vertex buffer lock arguments");
        calls.emplace_back("lock");
        return kMapped;
    }
    void UnlockVertexBuffer(std::uint32_t buffer) override {
        Check(buffer == 0x9000u, "vertex buffer unlock");
        calls.emplace_back("unlock");
    }
    std::uint32_t InstanceV3d(
        std::uint8_t type, std::uint32_t destination,
        std::uint32_t source, std::uint32_t count,
        std::uint32_t stride) override {
        Check(type == 2u && destination == kMapped &&
                  source == 0x1100u && count == 5u &&
                  stride == words[kHeader + 24u], "position copy arguments");
        calls.emplace_back("position");
        return position_width;
    }
    std::uint32_t InstanceV3dComp(
        std::uint8_t type, std::uint32_t destination,
        std::uint32_t source, std::uint32_t count,
        std::uint32_t stride) override {
        Check(type == (compact ? 14u : 2u) &&
                  destination == kMapped + position_width &&
                  source == 0x1200u && count == 5u &&
                  stride == words[kHeader + 24u], "normal copy arguments");
        calls.emplace_back("normal");
        return compact ? 4u : 12u;
    }
    std::uint32_t InstanceColor(
        std::uint32_t destination, std::uint32_t source,
        std::uint32_t count, std::uint32_t stride) override {
        const auto first = calls.empty() ||
                           std::find(calls.begin(), calls.end(), "color") ==
                               calls.end();
        const auto start = first ? 1u : 2u;
        Check(destination == kMapped + stride * start +
                  position_width + (compact ? 4u : 12u) &&
                  source == 0x1400u + 4u * start &&
                  count == (first ? 2u : 3u),
              "per-mesh color arguments");
        calls.emplace_back("color");
        return first ? 0x55u : 0x66u;
    }
    std::uint32_t InstanceV2d(
        std::uint8_t type, std::uint32_t destination,
        std::uint32_t source, std::uint32_t count,
        std::uint32_t stride) override {
        Check(type == 1u && destination >= kMapped &&
                  count == 5u && stride == words[kHeader + 24u],
              "texture copy arguments");
        calls.emplace_back(source == 0x1300u ? "tex0" : "tex1");
        return 8u;
    }
    std::uint32_t InstanceTangent(
        std::uint8_t type, std::uint32_t destination,
        std::uint32_t positions, std::uint32_t zero,
        std::uint32_t normals, std::uint32_t texcoords,
        std::uint32_t header, std::uint32_t stride) override {
        Check(type == (compact ? 14u : 2u) &&
                  destination >= kMapped && positions == 0x1100u &&
                  zero == 0u && normals == 0x1200u &&
                  header == kHeader && stride == words[kHeader + 24u],
              "weighted normal arguments");
        last_process_texcoords = texcoords;
        calls.emplace_back("weights");
        return compact ? 4u : 12u;
    }
};

void BaseFormat() {
    Runtime runtime;
    Check(pal4::WorldSectorVertexInstancer::Instance(
              kSector, kHeader, runtime) == 1u,
          "base format succeeds");
    Check(runtime.declaration ==
              (std::vector<pal4::WorldSectorVertexElement>{
                  {0u, 0u, 2u, 0u, 0u, 0u},
                  {255u, 0u, 17u, 0u, 0u, 0u}}) &&
              runtime.words[kHeader + 24u] == 12u &&
              runtime.words[kHeader + 28u] == 2u &&
              runtime.words[kHeader + 60u] == 5u,
          "base declaration and header layout");
    Check(runtime.words[kHeader + 84u] == 3u &&
              runtime.words[kHeader + 120u] == 4u &&
              runtime.words[kHeader + 76u] == 0u &&
              runtime.words[kHeader + 112u] == 0u &&
              runtime.words[kHeader + 48u] == 0u &&
              runtime.calls.back() == "unlock",
          "mesh offsets and absent colors are reset");
}

void ReinstanceReplacesBaseIndex() {
    Runtime runtime;
    runtime.words[kHeader + 84u] = 0xdeadbeefu;
    runtime.words[kHeader + 120u] = 0xffffffffu;
    for (int pass = 0; pass != 2; ++pass) {
        Check(pal4::WorldSectorVertexInstancer::Instance(kSector, kHeader, runtime) == 1u,
              "repeat instance succeeds");
        Check(runtime.words[kHeader + 84u] == 3u && runtime.words[kHeader + 120u] == 4u &&
                  runtime.words[kHeader + 68u] == 1u && runtime.words[kHeader + 104u] == 2u,
              "base indices derive from minVert, ignore stale destination and do not accumulate");
    }
}

void FullCompactFormat() {
    Runtime runtime;
    runtime.flags = 0x9cu;
    runtime.usage_flags = 0x10000u;
    runtime.compact = true;
    runtime.words[kSector + 20u] = 0x1500u;
    Check(pal4::WorldSectorVertexInstancer::Instance(
              kSector, kHeader, runtime) == 1u,
          "full compact format succeeds");
    Check(runtime.declaration ==
              (std::vector<pal4::WorldSectorVertexElement>{
                  {0u, 0u, 2u, 0u, 0u, 0u},
                  {0u, 12u, 14u, 0u, 3u, 0u},
                  {0u, 16u, 4u, 0u, 10u, 0u},
                  {0u, 20u, 1u, 0u, 5u, 0u},
                  {0u, 28u, 1u, 0u, 5u, 1u},
                  {0u, 36u, 14u, 0u, 6u, 0u},
                  {255u, 0u, 17u, 0u, 0u, 0u}}) &&
              runtime.words[kHeader + 24u] == 40u &&
              runtime.words[kHeader + 28u] == 0x3eu,
          "compact vertex elements, stride and attribute bits");
    Check(runtime.words[kHeader + 76u] == 0x55u &&
              runtime.words[kHeader + 112u] == 0x66u &&
              runtime.last_process_texcoords == 0x1500u &&
              runtime.calls.back() == "unlock",
          "per-mesh colors and flagged second texcoord input");
}

void UncompressedTangentFormat() {
    Runtime runtime;
    runtime.flags = 0x14u;
    runtime.usage_flags = 0x10000u;
    Check(pal4::WorldSectorVertexInstancer::Instance(
              kSector, kHeader, runtime) == 1u,
          "uncompressed tangent format succeeds");
    Check(runtime.declaration ==
              (std::vector<pal4::WorldSectorVertexElement>{
                  {0u, 0u, 2u, 0u, 0u, 0u},
                  {0u, 12u, 2u, 0u, 3u, 0u},
                  {0u, 24u, 1u, 0u, 5u, 0u},
                  {0u, 32u, 2u, 0u, 6u, 0u},
                  {255u, 0u, 17u, 0u, 0u, 0u}}) &&
              runtime.words[kHeader + 24u] == 44u &&
              runtime.words[kHeader + 28u] == 0x16u &&
              runtime.last_process_texcoords == 0x1300u,
          "uncompressed formats and first texcoord normal source");
    Check(std::find(runtime.calls.begin(), runtime.calls.end(), "color") ==
              runtime.calls.end() &&
              runtime.words[kHeader + 76u] == 0u &&
              runtime.words[kHeader + 112u] == 0u,
          "absent color array resets both mesh color fields");
}

void SecondTexcoordsAlone() {
    Runtime runtime;
    runtime.words[kSector + 20u] = 0x1500u;
    Check(pal4::WorldSectorVertexInstancer::Instance(
              kSector, kHeader, runtime) == 1u &&
              runtime.declaration ==
                  (std::vector<pal4::WorldSectorVertexElement>{
                      {0u, 0u, 2u, 0u, 0u, 0u},
                      {0u, 12u, 1u, 0u, 5u, 1u},
                      {255u, 0u, 17u, 0u, 0u, 0u}}) &&
              runtime.words[kHeader + 24u] == 20u &&
              std::find(runtime.calls.begin(), runtime.calls.end(),
                        "tex1") != runtime.calls.end(),
          "second texcoord pointer alone adds usage index one");
}

void FailuresAndEmptyMeshes() {
    Runtime runtime;
    runtime.create_result = 0u;
    Check(pal4::WorldSectorVertexInstancer::Instance(
              kSector, kHeader, runtime) == 0u &&
              runtime.calls.back() == "create-buffer",
          "buffer failure forwards zero without locking");
    Runtime mismatch;
    mismatch.position_width = 11u;
    Check(pal4::WorldSectorVertexInstancer::Instance(
              kSector, kHeader, mismatch) == 0u &&
              mismatch.calls.back() == "position",
          "width mismatch returns zero without unlocking");
    Runtime empty;
    empty.words[kHeader + 4u] = 0u;
    Check(pal4::WorldSectorVertexInstancer::Instance(
              kSector, kHeader, empty) == 0u &&
              empty.calls.back() == "create-buffer",
          "zero meshes stop before native decrement loop");
}

}  // namespace

int main() {
    BaseFormat();
    ReinstanceReplacesBaseIndex();
    FullCompactFormat();
    UncompressedTangentFormat();
    SecondTexcoordsAlone();
    FailuresAndEmptyMeshes();
}
