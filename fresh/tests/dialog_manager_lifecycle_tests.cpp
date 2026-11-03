#include "pal4/dialog_manager_lifecycle.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IDialogManagerLifecycleRuntime {
    std::vector<std::string> calls;
    std::size_t fail_tree = 0;
    bool fail_reset = false;
    std::size_t tree_count = 0;

    void ConstructTree(pal4::DialogManagerState& state, std::size_t offset,
                       pal4::DialogTreeKind kind) override {
        ++tree_count;
        calls.emplace_back("construct:" + std::to_string(offset));
        Check(kind == (offset == 116 ? pal4::DialogTreeKind::Extended
                                     : pal4::DialogTreeKind::Standard),
              "first tree uses extended constructor only");
        Check(state.Byte(offset) == 0xa5 &&
                  state.Byte(offset + 1) == 0xa5 &&
                  state.Byte(offset + 8) == 0,
              "tree pre-initialization precedes constructor call");
        if (fail_tree == tree_count)
            throw std::runtime_error("tree constructor failed");
    }
    void DestroyTree(pal4::DialogManagerState&, std::size_t offset) override {
        calls.emplace_back("destroy:" + std::to_string(offset));
    }
    void CleanupArray(pal4::DialogManagerState& state,
                      std::size_t offset) override {
        Check(offset == 180 && state.Word(184) == 0 &&
                  state.Word(188) == 0 && state.Word(192) == 0,
              "array unwind operands");
        calls.emplace_back("array cleanup");
    }
    void Reset(pal4::DialogManagerState& state) override {
        calls.emplace_back("reset");
        Check(state.Word(24) == 0, "offset 24 cleared");
        for (std::size_t offset = 84; offset <= 112; offset += 4)
            Check(state.Word(offset) == 0, "eight dwords cleared");
        Check(state.Byte(180) == 0xa5 && state.Word(184) == 0 &&
                  state.Word(188) == 0 && state.Word(192) == 0,
              "array fields initialized before reset");
        if (fail_reset) throw std::runtime_error("reset failed");
    }
};

struct TreeRuntime final : pal4::IDialogManagerCleanupRuntime {
    std::unordered_map<std::uint32_t, std::array<std::uint32_t, 3>> nodes;
    std::unordered_map<std::uint32_t, std::uint32_t> successors;
    std::vector<std::string> calls;
    std::size_t fail_range = 0;

    std::uint32_t NodeWord(std::uint32_t node, std::size_t offset) override {
        Check(offset <= 8 && offset % 4 == 0, "node field offset");
        return nodes.at(node)[offset / 4];
    }
    void SetNodeWord(std::uint32_t node, std::size_t offset,
                     std::uint32_t value) override {
        nodes.at(node)[offset / 4] = value;
        calls.emplace_back("set:" + std::to_string(offset) + ":" +
                           std::to_string(value));
    }
    void FreeSubtree(pal4::DialogManagerState&,
                     std::uint32_t root) override {
        calls.emplace_back("subtree:" + std::to_string(root));
    }
    void FirstElement(pal4::DialogManagerState&) override {
        calls.emplace_back("first");
    }
    std::uint32_t NextNode(std::uint32_t node) override {
        calls.emplace_back("next:" + std::to_string(node));
        return successors.at(node);
    }
    void DeleteNode(pal4::DialogManagerState&,
                    std::uint32_t node) override {
        calls.emplace_back("delete:" + std::to_string(node));
    }
    std::uint32_t FreeMemory(std::uint32_t block) override {
        calls.emplace_back("free:" + std::to_string(block));
        return block + 0x10;
    }
    std::uint32_t FirstElementAt(pal4::DialogManagerState&,
                                 std::size_t offset) override {
        calls.emplace_back("firstat:" + std::to_string(offset));
        return nodes.at(offset == 164 ? 0x4000 : 0)[0];
    }
    void DeleteRange(pal4::DialogManagerState&, std::size_t offset,
                     std::uint32_t first, std::uint32_t last) override {
        calls.emplace_back("range:" + std::to_string(offset) + ":" +
                           std::to_string(first) + ":" +
                           std::to_string(last));
        if (fail_range == offset)
            throw std::runtime_error("range deletion failed");
    }
    void ClearRange(pal4::DialogManagerState&, std::size_t offset,
                    std::uint32_t first, std::uint32_t last) override {
        calls.emplace_back("clear:" + std::to_string(offset) + ":" +
                           std::to_string(first) + ":" +
                           std::to_string(last));
        if (fail_range == offset)
            throw std::runtime_error("range clearing failed");
    }
    void DestroyTreeOnException(pal4::DialogManagerState&,
                                std::size_t offset) override {
        calls.emplace_back("unwind:" + std::to_string(offset));
    }
};

void SeedCleanupState(pal4::DialogManagerState& state,
                      TreeRuntime& runtime) {
    state.SetWord(184, 0x5000);
    state.SetWord(188, 0x5004);
    state.SetWord(192, 0x5008);
    state.SetWord(168, 0x4000);
    state.SetWord(176, 1);
    state.SetWord(152, 0x3000);
    state.SetWord(160, 1);
    state.SetWord(136, 0x2000);
    state.SetWord(144, 1);
    state.SetWord(120, 0x1000);
    state.SetWord(128, 1);
    runtime.nodes[0x4000] = {0x4100, 0, 0};
    runtime.nodes[0x3000] = {0x3100, 0, 0};
    runtime.nodes[0x2000] = {0x2100, 0, 0};
    runtime.nodes[0x1000] = {0x1100, 0, 0};
}

void TestConstruction() {
    pal4::DialogManagerState state;
    state.SetWord(0, 0x12345678);
    state.SetWord(80, 0x87654321);
    state.SetWord(24, 0xffffffff);
    for (std::size_t offset = 84; offset <= 112; offset += 4)
        state.SetWord(offset, 0xffffffff);
    Runtime runtime;
    Check(pal4::DialogManagerLifecycle::Construct(state, 0xa5, runtime) ==
              &state,
          "constructor returns this");
    Check(state.Word(0) == 0x12345678 &&
              state.Word(80) == 0x87654321,
          "constructor preserves unrelated fields");
    Check(runtime.calls == std::vector<std::string>{
              "construct:116", "construct:132", "construct:148",
              "construct:164", "reset"},
          "four tree members precede reset");
}

void TestConstructorUnwind() {
    constexpr std::array<std::size_t, 4> offsets{116, 132, 148, 164};
    for (std::size_t failed = 1; failed <= 5; ++failed) {
        pal4::DialogManagerState state;
        Runtime runtime;
        runtime.fail_tree = failed <= 4 ? failed : 0;
        runtime.fail_reset = failed == 5;
        bool threw = false;
        try {
            static_cast<void>(pal4::DialogManagerLifecycle::Construct(
                state, 0xa5, runtime));
        } catch (const std::runtime_error&) {
            threw = true;
        }
        Check(threw, "constructor failure propagates");
        std::vector<std::string> expected;
        const auto attempts = failed <= 4 ? failed : 4;
        for (std::size_t index = 0; index < attempts; ++index)
            expected.emplace_back("construct:" +
                                  std::to_string(offsets[index]));
        if (failed == 5) {
            expected.emplace_back("reset");
            expected.emplace_back("array cleanup");
        }
        const auto built = failed <= 4 ? failed - 1 : 4;
        for (std::size_t count = built; count != 0; --count)
            expected.emplace_back("destroy:" +
                                  std::to_string(offsets[count - 1]));
        Check(runtime.calls == expected,
              "SEH unwinds completed members in reverse order");
    }
}

void TestPrimaryTreeBulkDestruction() {
    pal4::DialogManagerState state;
    state.SetWord(4, 0x1000);
    state.SetWord(12, 2);
    pal4::DialogManagerPrimaryTreeGlobals globals{2, 0x9000};
    TreeRuntime runtime;
    runtime.nodes[0x1000] = {0x2000, 0x2100, 0x2200};
    Check(pal4::DialogManagerLifecycle::DestroyPrimaryTree(
              state, globals, runtime) == 1,
          "bulk destructor returns remaining shared count");
    Check(state.Word(4) == 0 && state.Word(12) == 0 &&
              globals.references == 1 && globals.nil_node == 0x9000,
          "bulk destructor clears own tree and preserves shared nil");
    Check(runtime.nodes.at(0x1000) ==
              std::array<std::uint32_t, 3>{0x1000, 0x9000, 0x1000},
          "bulk path resets head before freeing it");
    Check(runtime.calls == std::vector<std::string>{
              "subtree:8448", "set:4:36864", "set:0:4096",
              "set:8:4096", "first", "free:4096"},
          "bulk subtree and head release order");
}

void TestPrimaryTreeIterativeDestruction() {
    pal4::DialogManagerState state;
    state.SetWord(4, 0x1000);
    state.SetWord(12, 0);
    pal4::DialogManagerPrimaryTreeGlobals globals{2, 0x9000};
    TreeRuntime runtime;
    runtime.nodes[0x1000] = {0x2000, 0, 0};
    runtime.successors = {{0x2000, 0x3000}, {0x3000, 0x1000}};
    Check(pal4::DialogManagerLifecycle::DestroyPrimaryTree(
              state, globals, runtime) == 1,
          "iterative destructor returns remaining references");
    Check(runtime.calls == std::vector<std::string>{
              "next:8192", "delete:8192", "next:12288",
              "delete:12288", "free:4096"},
          "next pointer is captured before each node deletion");
}

void TestPrimaryTreeLastReference() {
    pal4::DialogManagerState state;
    state.SetWord(4, 0x1000);
    pal4::DialogManagerPrimaryTreeGlobals globals{1, 0x9000};
    TreeRuntime runtime;
    runtime.nodes[0x1000] = {0x1000, 0, 0};
    Check(pal4::DialogManagerLifecycle::DestroyPrimaryTree(
              state, globals, runtime) == 0x9010 &&
              globals.references == 0 && globals.nil_node == 0,
          "last reference frees shared nil and forwards allocator result");
    Check(runtime.calls ==
              std::vector<std::string>{"free:4096", "free:36864"},
          "empty tree frees head before shared nil");

    state.SetWord(4, 0x1000);
    globals = {1, 0};
    runtime.calls.clear();
    Check(pal4::DialogManagerLifecycle::DestroyPrimaryTree(
              state, globals, runtime) == 0 &&
              runtime.calls == std::vector<std::string>{"free:4096"},
          "missing shared nil returns zero without a second free");
}

void TestFullCleanup() {
    pal4::DialogManagerState state;
    TreeRuntime runtime;
    SeedCleanupState(state, runtime);
    pal4::DialogManagerPrimaryTreeGlobals collections{3, 0x9000};
    pal4::DialogManagerPrimaryTreeGlobals primary{1, 0xa000};
    Check(pal4::DialogManagerLifecycle::Cleanup(
              state, collections, primary, runtime) == 0xa010,
          "cleanup returns final primary nil free result");
    for (const auto offset : {120u, 128u, 136u, 144u, 152u, 160u,
                              168u, 176u, 184u, 188u, 192u})
        Check(state.Word(offset) == 0, "cleanup clears owned fields");
    Check(collections.references == 0 && collections.nil_node == 0 &&
              primary.references == 0 && primary.nil_node == 0,
          "cleanup releases both shared sentinels");
    Check(runtime.calls == std::vector<std::string>{
              "free:20480", "firstat:164", "range:164:16640:16384",
              "free:16384", "range:148:12544:12288", "free:12288",
              "range:132:8448:8192", "free:8192", "free:36864",
              "clear:116:4352:4096", "free:4096", "free:40960"},
          "array and four trees are released in IDA order");
}

void TestFullCleanupExceptionUnwind() {
    for (const auto fail_offset : {164u, 148u, 132u, 116u}) {
        pal4::DialogManagerState state;
        TreeRuntime runtime;
        SeedCleanupState(state, runtime);
        runtime.fail_range = fail_offset;
        pal4::DialogManagerPrimaryTreeGlobals collections{3, 0x9000};
        pal4::DialogManagerPrimaryTreeGlobals primary{1, 0xa000};
        bool threw = false;
        try {
            static_cast<void>(pal4::DialogManagerLifecycle::Cleanup(
                state, collections, primary, runtime));
        } catch (const std::runtime_error&) {
            threw = true;
        }
        Check(threw, "range error propagates");
        const std::vector<std::string> expected =
            fail_offset == 164 ? std::vector<std::string>{
                                     "unwind:148", "unwind:132", "unwind:116"}
            : fail_offset == 148 ? std::vector<std::string>{
                                     "unwind:132", "unwind:116"}
            : fail_offset == 132 ? std::vector<std::string>{"unwind:116"}
                                 : std::vector<std::string>{};
        Check(runtime.calls.size() >= expected.size(),
              "unwind call count");
        Check(std::vector<std::string>(runtime.calls.end() - expected.size(),
                                       runtime.calls.end()) == expected,
              "SEH unwinds remaining tree members in reverse order");
    }
}

}  // namespace

int main() {
    try {
        TestConstruction();
        TestConstructorUnwind();
        TestPrimaryTreeBulkDestruction();
        TestPrimaryTreeIterativeDestruction();
        TestPrimaryTreeLastReference();
        TestFullCleanup();
        TestFullCleanupExceptionUnwind();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
