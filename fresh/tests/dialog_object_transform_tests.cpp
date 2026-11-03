#include "pal4/dialog_object.h"

#include <bit>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace {
void Check(bool ok, const char* message) {
    if (!ok) { std::cerr << "FAIL: " << message << '\n'; std::exit(1); }
}
void Write(pal4::DialogObjectState& state, std::size_t word, float value) {
    state.words[word] = std::bit_cast<std::uint32_t>(value);
}
float Read(const pal4::DialogObjectState& state, std::size_t word) {
    return std::bit_cast<float>(state.words[word]);
}
struct Runtime final : pal4::IDialogObjectRuntime {
    std::vector<std::string> calls;
    std::uint32_t parent = 0u;
    std::uint32_t Allocate(std::uint32_t) override { return 0u; }
    void Free(std::uint32_t) override {}
    void WriteHeapWord(std::uint32_t, std::uint32_t,
                       std::uint32_t) override {}
    std::uint32_t ReadHeapWord(std::uint32_t,
                               std::uint32_t) override { return 0u; }
    std::uint32_t InitializeTextManager(std::uint32_t) override { return 0u; }
    void DestroyTextManager(std::uint32_t) override {}
    void InitializeXmlDocument(pal4::DialogObjectState&) override {}
    void DestroyXmlDocument(pal4::DialogObjectState&) override {}
    std::uint8_t ConstructorStackByte() override { return 0u; }
    void DestroyFirstTreeBranch(std::uint32_t) override {}
    void DestroySecondTreeBranch(std::uint32_t) override {}
    std::uint32_t NextFirstTreeNode(std::uint32_t) override { return 0u; }
    std::uint32_t NextSecondTreeNode(std::uint32_t) override { return 0u; }
    void EraseFirstTreeNode(std::uint32_t) override {}
    void EraseSecondTreeNode(std::uint32_t) override {}
    void DestroyStringRecord(std::uint32_t) override {}
    void CleanupBeforeDelete(std::uint32_t, std::uint32_t,
                             std::uint32_t) override {}
    std::uint32_t VirtualDelete(std::uint32_t,
                                std::uint8_t) override { return 0u; }
    int RefreshTransform(pal4::DialogObjectState&) override {
        calls.emplace_back("refresh"); return 42;
    }
    std::uint32_t FrameParent(std::uint32_t frame) override {
        Check(frame == 0x5000u, "stored frame queried");
        calls.emplace_back("parent"); return parent;
    }
    int RemoveFrameChild(std::uint32_t frame) override {
        Check(frame == 0x5000u, "stored frame removed");
        calls.emplace_back("remove"); parent = 0u; return 88;
    }
    int AddFrameChild(std::uint32_t new_parent,
                      std::uint32_t frame) override {
        Check(frame == 0x5000u && new_parent == 0x6000u,
              "stored frame added to requested parent");
        calls.emplace_back("add"); parent = new_parent; return 77;
    }
};
}  // namespace

int main() {
    pal4::DialogObjectState state{};
    state.words.fill(0xdeadbeefu);
    Runtime runtime;
    for (std::size_t i = 0u; i < 3u; ++i) {
        Write(state, 44u + i, float(i + 1u));
        Write(state, 47u + i, 1.0f);
        Write(state, 50u + i, 0.0f);
    }
    Check(pal4::DialogObject::SetPosition(
              state, {4.0f, 5.0f, 6.0f}, false, runtime) == 42 &&
          Read(state, 44u) == 4.0f && Read(state, 45u) == 5.0f &&
          Read(state, 46u) == 6.0f && Read(state, 47u) == 1.0f,
          "absolute position replaces three words and refreshes frame");
    Check(pal4::DialogObject::SetPosition(
              state, {1.0f, -2.0f, 3.0f}, true, runtime) == 42 &&
          Read(state, 44u) == 5.0f && Read(state, 45u) == 3.0f &&
          Read(state, 46u) == 9.0f,
          "relative position accumulates per axis");
    Check(pal4::DialogObject::SetRotation(
              state, {10.0f, 20.0f, 30.0f}, false, runtime) == 42 &&
          pal4::DialogObject::SetRotation(
              state, {1.0f, 2.0f, 3.0f}, true, runtime) == 42 &&
          Read(state, 50u) == 11.0f && Read(state, 51u) == 22.0f &&
          Read(state, 52u) == 33.0f,
          "rotation absolute and relative modes use +200..+208");
    Check(pal4::DialogObject::SetScale(
              state, {2.0f, 3.0f, 4.0f}, false, runtime) == 42 &&
          pal4::DialogObject::SetScale(
              state, {0.5f, -1.0f, 2.0f}, true, runtime) == 42 &&
          Read(state, 47u) == 2.5f && Read(state, 48u) == 2.0f &&
          Read(state, 49u) == 6.0f && runtime.calls.size() == 6u,
          "scale modes use +188..+196 and refresh each time");

    state.words[43u] = 0x5000u;
    runtime.calls.clear();
    Check(pal4::DialogObject::SetParentFrame(
              state, 0x6000u, runtime) == 77 &&
          runtime.calls == std::vector<std::string>{"parent", "add"},
          "unparented frame is added directly");
    runtime.parent = 0x7000u;
    runtime.calls.clear();
    Check(pal4::DialogObject::SetParentFrame(
              state, 0x6000u, runtime) == 77 &&
          runtime.calls == std::vector<std::string>{
              "parent", "remove", "add"},
          "reparented frame detaches before adding");
    runtime.calls.clear();
    Check(pal4::DialogObject::SetParentFrame(state, 0u, runtime) == 88 &&
          runtime.calls == std::vector<std::string>{"parent", "remove"},
          "null parent detaches a parented frame");
    runtime.calls.clear();
    Check(pal4::DialogObject::SetParentFrame(state, 0u, runtime) == 0 &&
          runtime.calls == std::vector<std::string>{"parent"},
          "null parent leaves an unparented frame alone");
    runtime.parent = 0x7000u;
    runtime.calls.clear();
    Check(pal4::DialogObject::RemoveFromParentFrame(state, runtime) == 88 &&
          runtime.calls == std::vector<std::string>{"parent", "remove"},
          "explicit parent removal dispatches only when attached");
    runtime.calls.clear();
    Check(pal4::DialogObject::RemoveFromParentFrame(state, runtime) ==
              0x5000 && runtime.calls == std::vector<std::string>{"parent"},
          "unparented removal returns the frame handle");
    state.words[43u] = 0u;
    runtime.calls.clear();
    Check(pal4::DialogObject::RemoveFromParentFrame(state, runtime) == 0 &&
          runtime.calls.empty(), "null frame removal is inert");

    state.words[53u] = 0xabcdef00u;
    Check(pal4::DialogObject::SetVisibility(state, 5u) == 5 &&
          state.words[53u] == 0xabcdef01u && state.words[54u] == 5u,
          "nonzero visibility sets flag and stores source value");
    Check(pal4::DialogObject::SetVisibility(state, 0u) == 0 &&
          state.words[53u] == 0xabcdef00u && state.words[54u] == 0u,
          "zero visibility clears both fields");
    state.words[53u] |= 1u;
    state.words[54u] = 99u;
    Check(pal4::DialogObject::ResetVisibility(state) == 0 &&
          state.words[53u] == 0xabcdef00u && state.words[54u] == 0u,
          "reset visibility preserves neighboring bytes");
}
