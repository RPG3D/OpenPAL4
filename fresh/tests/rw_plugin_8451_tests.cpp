#include "pal4/rw_plugin_8451.h"

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

class Runtime final : public pal4::IRwPlugin8451Runtime {
public:
    std::int32_t RegisterEngine(
        std::uint32_t size, std::uint32_t id,
        pal4::RwPlugin8451Callback construct,
        pal4::RwPlugin8451Callback destruct) override {
        using C = pal4::RwPlugin8451Callback;
        Check(size == 1 && id == 8451 && construct == C::Identity &&
                  destruct == C::Identity, "engine registration");
        calls.push_back("engine");
        return engine_result;
    }
    std::int32_t RegisterGeometry(
        std::uint32_t size, std::uint32_t id,
        pal4::RwPlugin8451Callback construct,
        pal4::RwPlugin8451Callback destruct,
        pal4::RwPlugin8451Callback copy) override {
        using C = pal4::RwPlugin8451Callback;
        Check(size == 8 && id == 8451 &&
                  construct == C::GeometryConstruct &&
                  destruct == C::GeometryDestruct &&
                  copy == C::GeometryCopy, "geometry registration");
        calls.push_back("geometry");
        return geometry_result;
    }
    std::int32_t RegisterGeometryStream(
        std::uint32_t id, pal4::RwPlugin8451Callback read,
        pal4::RwPlugin8451Callback write,
        pal4::RwPlugin8451Callback size) override {
        using C = pal4::RwPlugin8451Callback;
        Check(id == 8451 && read == C::Identity &&
                  write == C::Identity && size == C::StreamSize,
              "stream registration");
        calls.push_back("stream");
        return stream_result;
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        calls.push_back("write:" + std::to_string(address));
        words[address] = value;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.push_back("read:" + std::to_string(address));
        return words[address];
    }
    std::uint32_t ForAllAtomics(
        std::uint32_t clump, pal4::RwPlugin8451Callback callback,
        std::uint32_t user_data) override {
        Check((clump == 0 || clump == 0x900) &&
                  callback == pal4::RwPlugin8451Callback::HideHookAtomic &&
                  user_data == 0, "hide traversal args");
        calls.push_back("atomics");
        return 0xabcu;
    }
    std::int32_t UserDataCount(std::uint32_t geometry) override {
        Check(geometry == 0x400, "user data geometry");
        calls.push_back("count");
        return static_cast<std::int32_t>(texts.size());
    }
    std::uint32_t UserDataArray(
        std::uint32_t geometry, std::uint32_t index) override {
        Check(geometry == 0x400 && index < texts.size(),
              "user data array index");
        calls.push_back("array:" + std::to_string(index));
        return 0x1000 + index * 16;
    }
    std::string_view UserDataText(
        std::uint32_t array, std::uint32_t index) override {
        Check(array == 0x1000 + index * 16 && index < texts.size(),
              "user data text index");
        calls.push_back("text:" + std::to_string(index));
        return texts[index];
    }
    std::uint32_t HookRenderCallbackAddress() override {
        calls.push_back("hook_callback");
        return hook_callback;
    }
    std::uint32_t DefaultRenderCallbackAddress() override {
        calls.push_back("default_callback");
        return 0x8888;
    }
    std::uint32_t FrameCount(std::uint32_t frame) override {
        Check(frame == 0x777, "hidden render frame");
        calls.push_back("frame_count");
        return frame_count;
    }
    void UpdateFrameObjects(std::uint32_t frame) override {
        Check(frame == 0x777, "hidden render update frame");
        calls.push_back("frame_update");
    }
    std::uint32_t ForAllAtomicsWithAttachment(
        std::uint32_t clump, pal4::RwPlugin8451Callback callback,
        std::string_view name, std::uint32_t frame) override {
        Check(clump == 0x900 &&
                  callback == pal4::RwPlugin8451Callback::AttachFrameToAtomic &&
                  name == "hook_h" && (frame == 0xaaa || frame == 0),
              "attachment traversal context");
        calls.push_back("attach_atomics");
        return 0xdef;
    }
    void RemoveFrameChild(std::uint32_t frame) override {
        Check(frame == 0xaaa, "remove attached frame");
        calls.push_back("remove_frame");
    }
    void AddFrameChild(std::uint32_t parent,
                       std::uint32_t child) override {
        Check(parent == 0x777 && child == 0xaaa,
              "add attached frame");
        calls.push_back("add_frame");
    }
    void ForAllAtomicsWithQuery(
        std::uint32_t clump, pal4::RwPlugin8451Callback callback,
        std::string_view name, std::uint32_t& output) override {
        Check(clump == 0x900 &&
                  callback == pal4::RwPlugin8451Callback::QueryUserDataAtomic &&
                  name == "hook_h", "query traversal context");
        calls.push_back("query_atomics");
        output = 0x200;
    }

    std::int32_t engine_result = 4;
    std::int32_t geometry_result = 8;
    std::int32_t stream_result = 8;
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> calls;
    std::vector<std::string> texts;
    std::uint32_t hook_callback = 0x9999;
    std::uint32_t frame_count = 2;
};

}  // namespace

int main() {
    Runtime runtime;
    pal4::RwPlugin8451State state;
    Check(pal4::RwPlugin8451::Register(state, runtime) &&
              state.engine_offset == 4 && state.geometry_offset == 8 &&
              runtime.calls == std::vector<std::string>{
                  "engine", "geometry", "stream"},
          "registration order and offsets");
    runtime.calls.clear();
    runtime.engine_result = -1;
    Check(!pal4::RwPlugin8451::Register(state, runtime) &&
              state.engine_offset == -1 && state.geometry_offset == 8 &&
              runtime.calls == std::vector<std::string>{"engine"},
          "engine registration failure stops sequence");
    runtime.calls.clear();
    runtime.engine_result = 4;
    runtime.geometry_result = -1;
    Check(!pal4::RwPlugin8451::Register(state, runtime) &&
              state.geometry_offset == -1 &&
              runtime.calls == std::vector<std::string>{
                  "engine", "geometry"},
          "geometry registration failure stops sequence");
    runtime.calls.clear();
    runtime.geometry_result = 8;
    runtime.stream_result = 9;
    Check(!pal4::RwPlugin8451::Register(state, runtime) &&
              runtime.calls == std::vector<std::string>{
                  "engine", "geometry", "stream"},
          "stream offset mismatch fails registration");

    pal4::RwPlugin8451RuntimeStub stub;
    Check(!pal4::RwPlugin8451::Register(state, stub),
          "empty RenderWare stub is constructible");
    runtime.words[0x208] = 12;
    runtime.words[0x20c] = 13;
    Check(pal4::RwPlugin8451::GeometryConstruct(
              0x200, state, runtime) == 0x200 &&
              runtime.words[0x208] == 0 && runtime.words[0x20c] == 0,
          "geometry constructor clears two words");
    runtime.words[0x208] = 3;
    Check(pal4::RwPlugin8451::GeometryDestruct(
              0x200, state, runtime) == 0x200 &&
              runtime.words[0x208] == 0,
          "geometry destructor clears same two words");
    runtime.calls.clear();
    state.geometry_offset = 0;
    Check(pal4::RwPlugin8451::GeometryConstruct(
              0x200, state, runtime) == 0x200 && runtime.calls.empty(),
          "nonpositive constructor offset bypasses writes");
    state.geometry_offset = 8;
    runtime.calls.clear();
    runtime.words[0x308] = 0xaaaa;
    runtime.words[0x30c] = 0xbbbb;
    Check(pal4::RwPlugin8451::GeometryCopy(
              0x200, 0x300, state, runtime) == 0x200 &&
              runtime.words[0x208] == 0xaaaa &&
              runtime.words[0x20c] == 0xbbbb &&
              runtime.calls == std::vector<std::string>{
                  "read:776", "write:520", "read:780", "write:524"},
          "geometry copy transfers both words in native order");
    Check(pal4::RwPlugin8451::PluginSize() == 8,
          "stream size callback returns eight");

    runtime.calls.clear();
    Check(pal4::RwPlugin8451::HideClumpHookAtomics(0, runtime) == 0xabc &&
              runtime.calls == std::vector<std::string>{"atomics"},
          "clump hide traversal has no null gate");
    runtime.calls.clear();
    Check(pal4::RwPlugin8451::HideClumpHookAtomics(
              0x900, runtime) == 0xabc &&
              runtime.calls == std::vector<std::string>{"atomics"},
          "clump hide callback and result forwarded");

    runtime.words[0x218] = 0x400;
    runtime.words[0x20c] = 0;
    runtime.words[0x248] = 0x1234;
    runtime.texts = {"plain", "hook_", "hook_hand", "hook_other"};
    for (std::uint32_t index = 0; index < runtime.texts.size(); ++index)
        runtime.words[0x1004 + index * 16] = 3;
    runtime.calls.clear();
    Check(pal4::RwPlugin8451::HideHookAtomic(
              0x200, state, runtime) == 0x200 &&
              runtime.words[0x20c] == 1 &&
              runtime.words[0x208] == 0 &&
              runtime.words[0x248] == 0x9999,
          "matching hook user data hides atomic");
    Check(runtime.calls.back() == "read:524" &&
              runtime.calls[2] == "array:0",
          "scan continues after first hook match");
    runtime.calls.clear();
    Check(pal4::RwPlugin8451::HideHookAtomic(
              0x200, state, runtime) == 0x200 &&
              runtime.words[0x248] == 0x9999,
          "already hidden atomic stays hidden");

    runtime.words[0x20c] = 0;
    runtime.words[0x248] = 0x1234;
    runtime.hook_callback = 0;
    runtime.calls.clear();
    Check(pal4::RwPlugin8451::HideHookAtomic(
              0x200, state, runtime) == 0x200 &&
              runtime.words[0x248] == 0x8888,
          "zero hook callback selects RenderWare default");
    runtime.hook_callback = 0x9999;

    runtime.words[0x204] = 0x777;
    runtime.calls.clear();
    Check(pal4::RwPlugin8451::RenderHiddenAtomic(
              0x200, runtime) == 0x200 &&
              runtime.calls == std::vector<std::string>{
                  "read:516", "frame_count", "frame_update"},
          "hidden render updates frame when count exceeds one");
    runtime.frame_count = 1;
    runtime.calls.clear();
    Check(pal4::RwPlugin8451::RenderHiddenAtomic(
              0x200, runtime) == 0x200 &&
              runtime.calls == std::vector<std::string>{
                  "read:516", "frame_count"},
          "single frame bypasses update");

    runtime.calls.clear();
    Check(pal4::RwPlugin8451::ProcessClumpUserData(
              0x900, "hook_h", 0xaaa, runtime) == 0xdef &&
              runtime.calls == std::vector<std::string>{"attach_atomics"},
          "clump wrapper forwards paired name and frame context");
    runtime.words[0x20c] = 1;
    runtime.words[0xaae] = 0x1234;
    runtime.calls.clear();
    Check(pal4::RwPlugin8451::AttachFrameToMatchingAtomic(
              0x200, "hook_h", 0xaaa, state, runtime) == 0 &&
              runtime.words[0x208] == 0xaaa,
          "prefix-matching user data attaches frame and stops traversal");
    Check(runtime.calls.back() == "add_frame" &&
              runtime.calls[runtime.calls.size() - 3] == "remove_frame",
          "existing parent is removed before attaching frame");
    runtime.calls.clear();
    Check(pal4::RwPlugin8451::AttachFrameToMatchingAtomic(
              0x200, "absent", 0xaaa, state, runtime) == 0x200 &&
              runtime.calls.back() == "text:3",
          "no matching user data scans all entries");
    runtime.words[0x20c] = 0;
    runtime.calls.clear();
    Check(pal4::RwPlugin8451::AttachFrameToMatchingAtomic(
              0x200, "hook_h", 0xaaa, state, runtime) == 0x200,
          "unhandled plugin skips otherwise matching user data");
    runtime.words[0x20c] = 1;
    runtime.calls.clear();
    Check(pal4::RwPlugin8451::AttachFrameToMatchingAtomic(
              0x200, "hook_h", 0, state, runtime) == 0 &&
              runtime.words[0x208] == 0 &&
              runtime.calls.back() == "write:520",
          "zero target frame stops without frame tree operations");

    runtime.calls.clear();
    Check(pal4::RwPlugin8451::RemoveFrameChildAndClear(
              0x900, "hook_h", 0xaaa, runtime) == 0xdef &&
              runtime.calls == std::vector<std::string>{
                  "read:2734", "remove_frame", "attach_atomics"},
          "clear wrapper detaches old frame before zero-frame traversal");
    runtime.calls.clear();
    Check(pal4::RwPlugin8451::RemoveFrameChildAndClear(
              0x900, "hook_h", 0, runtime) == 0xdef &&
              runtime.calls == std::vector<std::string>{"attach_atomics"},
          "zero frame skips detach");

    runtime.calls.clear();
    Check(pal4::RwPlugin8451::QueryClumpUserData(
              0x900, std::nullopt, runtime) == 0 &&
              runtime.calls.empty(), "null name pointer bypasses query");
    Check(pal4::RwPlugin8451::QueryClumpUserData(
              0x900, "hook_h", runtime) == 0x200 &&
              runtime.calls == std::vector<std::string>{"query_atomics"},
          "clump query returns matching atomic output");
    runtime.calls.clear();
    std::uint32_t found = 77;
    Check(pal4::RwPlugin8451::QueryAtomicUserData(
              0x200, "hook_h", found, runtime) == 0 &&
              found == 0x200,
          "atomic query matches truncated user data and stops traversal");
    found = 77;
    runtime.calls.clear();
    Check(pal4::RwPlugin8451::QueryAtomicUserData(
              0x200, "absent", found, runtime) == 0x200 &&
              found == 77 && runtime.calls.back() == "text:3",
          "unmatched query scans all arrays and preserves output");
}
