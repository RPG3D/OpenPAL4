#include "pal4/dialog_object.h"

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
struct Runtime final : pal4::IDialogObjectPreDeleteRuntime {
    std::vector<std::string> calls;
    std::map<std::pair<std::uint32_t, std::uint32_t>, std::uint32_t> words;
    std::vector<bool> enabled{true, false};
    bool active = true;
    void ResetProperties(pal4::DialogObjectState&) override {
        calls.emplace_back("reset-properties");
    }
    std::uint32_t ReadWord(std::uint32_t address,
                           std::uint32_t offset) override {
        calls.emplace_back("read:" + std::to_string(address) + ":" +
                           std::to_string(offset));
        return words.at({address, offset});
    }
    void WriteWord(std::uint32_t address, std::uint32_t offset,
                   std::uint32_t value) override {
        calls.emplace_back("write:" + std::to_string(address) + ":" +
                           std::to_string(offset));
        words[{address, offset}] = value;
    }
    void DestroyXmlNode(std::uint32_t node) override {
        calls.emplace_back("xml:" + std::to_string(node));
    }
    void Free(std::uint32_t address) override {
        calls.emplace_back("free:" + std::to_string(address));
    }
    bool EffectsEnabled() override {
        calls.emplace_back("enabled");
        const bool value = enabled.front(); enabled.erase(enabled.begin());
        return value;
    }
    bool EffectIsActive(std::uint32_t effect) override {
        calls.emplace_back("active:" + std::to_string(effect));
        return active;
    }
    void ReleaseEffectReferences(std::uint32_t effect) override {
        calls.emplace_back("release:" + std::to_string(effect));
    }
    std::uint32_t EffectFrame(std::uint32_t effect) override {
        calls.emplace_back("effect-frame:" + std::to_string(effect));
        return 0x8000u;
    }
    std::uint32_t FrameParent(std::uint32_t frame) override {
        calls.emplace_back("parent:" + std::to_string(frame));
        return frame == 0x8000u ? 1u : 0u;
    }
    void RemoveFrameChild(std::uint32_t frame) override {
        calls.emplace_back("detach:" + std::to_string(frame));
    }
    void ProcessEffectEvent(std::uint32_t owner,
                            std::uint32_t effect) override {
        Check(owner == 0xabcdu, "effect owner");
        calls.emplace_back("event:" + std::to_string(effect));
    }
    void DestroyEffect(std::uint32_t effect) override {
        calls.emplace_back("destroy:" + std::to_string(effect));
    }
    void ReleaseTextRecord(std::uint32_t address) override {
        calls.emplace_back("text:" + std::to_string(address));
    }
    void DestroyFrame(std::uint32_t frame) override {
        calls.emplace_back("destroy-frame:" + std::to_string(frame));
    }
    void CleanupXmlDocument(pal4::DialogObjectState&) override {
        calls.emplace_back("cleanup-xml");
    }
    void ClearTextElements(std::uint32_t manager) override {
        Check(manager == 0xbeefu, "text manager");
        calls.emplace_back("clear-text");
    }
};
struct Facets final : pal4::IDialogObjectFacetCleanupRuntime {
    std::vector<std::string>& calls;
    explicit Facets(std::vector<std::string>& log) : calls(log) {}
    std::uint32_t SharedFirstTreeSentinel() override { return 0u; }
    std::uint32_t ReadNodeWord(std::uint32_t node,
                               std::uint32_t offset) override {
        Check(node == 0x1000u && offset == 0u, "empty facet head");
        calls.emplace_back("clear-facets"); return node;
    }
    void WriteNodeWord(std::uint32_t, std::uint32_t,
                       std::uint32_t) override { std::abort(); }
    void DeleteFirstTreeSubtree(std::uint32_t) override { std::abort(); }
    void ReleaseNodeText(std::uint32_t) override { std::abort(); }
    void FreeNode(std::uint32_t) override { std::abort(); }
    std::uint32_t NextFirstTreeNode(std::uint32_t) override { std::abort(); }
    void EraseFirstTreeNode(std::uint32_t) override { std::abort(); }
};
std::size_t At(const std::vector<std::string>& calls, const char* item) {
    const auto found = std::find(calls.begin(), calls.end(), item);
    Check(found != calls.end(), item);
    return static_cast<std::size_t>(found - calls.begin());
}
}  // namespace

int main() {
    pal4::DialogObjectState state{};
    state.words[1u] = 0xffffffffu;
    state.words[3u] = 0xabcdu;
    state.words[4u] = 0xbeefu;
    state.words[20u] = 0xffffffffu;
    state.words[24u] = 0x2000u;
    state.words[25u] = 0x2010u;
    state.words[28u] = 0x5000u;
    state.words[29u] = 0x5028u;
    state.words[32u] = 0x3000u;
    state.words[33u] = 0x3008u;
    state.words[36u] = 0x1000u;
    state.words[43u] = 0x9000u;
    state.words[53u] = 0xffffffffu;
    state.words[54u] = 0x1234u;
    Runtime runtime;
    runtime.words[{0x3000u, 0u}] = 0x4000u;
    runtime.words[{0x3004u, 0u}] = 0u;
    runtime.words[{0x5000u, 16u}] = 0x6000u;
    runtime.words[{0x5014u, 16u}] = 0x7000u;
    Facets facets(runtime.calls);
    pal4::DialogObject::PrepareForDelete(state, runtime, facets);
    Check(state.words[25u] == 0x2000u && state.words[33u] == 0x3000u &&
          state.words[29u] == 0x5000u && state.words[43u] == 0u &&
          state.words[3u] == 0u && state.words[54u] == 0u,
          "container ends, frame and owner reset");
    Check(runtime.words.at({0x2000u, 4u}) == 0u &&
          runtime.words.at({0x2008u, 4u}) == 0u &&
          runtime.words.at({0x5000u, 16u}) == 0u &&
          runtime.words.at({0x5014u, 16u}) == 0u,
          "record slots cleared");
    Check(state.words[1u] == 0xffffff00u &&
          state.words[20u] == 0xff000000u &&
          state.words[53u] == 0xffffff00u,
          "native single-byte clears preserve neighbors");
    const auto one = std::bit_cast<std::uint32_t>(1.0f);
    Check(state.words[44u] == 0u && state.words[45u] == one &&
          state.words[46u] == one && state.words[47u] == one &&
          state.words[48u] == one && state.words[49u] == one &&
          state.words[50u] == 0u && state.words[51u] == 0u &&
          state.words[52u] == 0u, "native transform defaults restored");
    const auto& calls = runtime.calls;
    Check(At(calls, "reset-properties") < At(calls, "xml:16384") &&
          At(calls, "xml:16384") < At(calls, "release:24576") &&
          At(calls, "release:24576") < At(calls, "detach:32768") &&
          At(calls, "detach:32768") < At(calls, "event:24576") &&
          At(calls, "event:24576") < At(calls, "destroy:24576") &&
          At(calls, "destroy:24576") < At(calls, "destroy:28672") &&
          At(calls, "destroy:28672") < At(calls, "text:20480") &&
          At(calls, "text:20500") < At(calls, "destroy-frame:36864") &&
          At(calls, "destroy-frame:36864") < At(calls, "clear-facets") &&
          At(calls, "clear-facets") < At(calls, "cleanup-xml") &&
          At(calls, "cleanup-xml") < At(calls, "clear-text"),
          "native cleanup order");
    Check(std::find(calls.begin(), calls.end(), "release:28672") == calls.end() &&
          std::find(calls.begin(), calls.end(), "event:28672") == calls.end(),
          "active effect with disabled cleanup only destroys facet");

    pal4::DialogObjectState inactive_state{};
    inactive_state.words[3u] = 0xabcdu;
    inactive_state.words[4u] = 0xbeefu;
    inactive_state.words[28u] = 0x5000u;
    inactive_state.words[29u] = 0x5028u;
    inactive_state.words[36u] = 0x1000u;
    Runtime inactive;
    inactive.enabled = {false, false};
    inactive.active = false;
    inactive.words[{0x5000u, 16u}] = 0x6000u;
    inactive.words[{0x5014u, 16u}] = 0u;
    Facets inactive_facets(inactive.calls);
    pal4::DialogObject::PrepareForDelete(inactive_state, inactive,
                                         inactive_facets);
    Check(At(inactive.calls, "active:24576") <
              At(inactive.calls, "release:24576") &&
          At(inactive.calls, "release:24576") <
              At(inactive.calls, "destroy:24576") &&
          inactive.words.at({0x5000u, 16u}) == 0u &&
          inactive_state.words[29u] == 0x5000u &&
          std::find(inactive.calls.begin(), inactive.calls.end(),
                    "destroy:0") == inactive.calls.end(),
          "disabled system cleans inactive effect and skips null record");
}
