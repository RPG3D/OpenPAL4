#include "pal4/world_manager_scene_lifecycle.h"

#include <cstdint>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

constexpr std::uint32_t kObject = 0x1000u;

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IWorldManagerSceneRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::unordered_map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> calls;

    void ConstructBaseWorld(std::uint32_t object) override {
        Check(object == kObject, "base constructor address");
        calls.emplace_back("base-construct");
        words[object] = 0x84b818u;
    }
    void DestroyBaseWorld(std::uint32_t object) override {
        Check(object == kObject, "base destructor address");
        calls.emplace_back("base-destroy");
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.emplace_back("read:" + std::to_string(address - kObject));
        return words[address];
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        calls.emplace_back("word:" + std::to_string(address - kObject));
        words[address] = value;
    }
    void WriteByte(std::uint32_t address,
                   std::uint8_t value) override {
        calls.emplace_back("byte:" + std::to_string(address - kObject));
        bytes[address] = value;
    }
    void Free(std::uint32_t address) override {
        calls.emplace_back("free:" + std::to_string(address));
    }
};

void ConstructFields() {
    Runtime runtime;
    Check(pal4::WorldManagerSceneLifecycle::Construct(
              kObject, runtime) == kObject &&
              runtime.words[kObject] ==
                  pal4::WorldManagerSceneLifecycle::kVtable &&
              runtime.bytes[kObject + 196u] == 0u &&
              runtime.bytes[kObject + 212u] == 0u,
          "constructor installs derived vtable and defines two stack bytes");
    for (const auto offset : {200u, 204u, 208u, 216u,
                              220u, 224u, 228u})
        Check(runtime.words[kObject + offset] == 0u,
              "constructor zeroes pointer and size words");
    Check(runtime.calls ==
              (std::vector<std::string>{
                  "base-construct", "byte:196", "word:200",
                  "word:204", "word:208", "byte:212",
                  "word:216", "word:220", "word:224",
                  "word:228", "word:0"}),
          "derived initialization follows base constructor in IDA order");
}

void DestroyFieldsAndDeletingFlag() {
    Runtime runtime;
    runtime.words[kObject + 216u] = 0x9000u;
    runtime.words[kObject + 200u] = 0x8000u;
    runtime.words[kObject + 220u] = 2u;
    runtime.words[kObject + 224u] = 3u;
    runtime.words[kObject + 204u] = 4u;
    runtime.words[kObject + 208u] = 5u;
    pal4::WorldManagerSceneLifecycle::Destroy(kObject, runtime);
    Check(runtime.calls ==
              (std::vector<std::string>{
                  "word:0", "read:216", "free:36864",
                  "word:216", "word:220", "word:224",
                  "read:200", "free:32768", "word:200",
                  "word:204", "word:208", "base-destroy"}),
          "destructor frees the second allocation first and then base");
    for (const auto offset : {200u, 204u, 208u,
                              216u, 220u, 224u})
        Check(runtime.words[kObject + offset] == 0u,
              "destructor clears both three-word groups");

    Runtime retained;
    Check(pal4::WorldManagerSceneLifecycle::DeletingDestructor(
              kObject, 2u, retained) == kObject &&
              retained.calls.back() == "base-destroy",
          "deleting destructor ignores non-low flag bits");
    Runtime released;
    Check(pal4::WorldManagerSceneLifecycle::DeletingDestructor(
              kObject, 3u, released) == kObject &&
              released.calls[released.calls.size() - 2u] ==
                  "base-destroy" &&
              released.calls.back() == "free:4096",
          "low flag bit frees object after full destruction");
}

}  // namespace

int main() {
    ConstructFields();
    DestroyFieldsAndDeletingFlag();
}
