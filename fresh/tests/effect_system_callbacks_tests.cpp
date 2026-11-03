#include "pal4/effect_system_callbacks.h"

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IEffectSystemCallbackRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::unordered_map<std::uint32_t, std::uint8_t> bytes;
    std::unordered_map<std::uint32_t, float> floats;
    std::unordered_map<std::uint32_t, pal4::EffectPosition> positions;
    std::vector<std::string> calls;
    std::uint32_t auxiliary = 0x2000u;
    std::uint32_t candidate = 0x3000u;
    std::uint32_t all_result = 0x9876u;

    std::uint8_t ReadByte(std::uint32_t address) override {
        return bytes.at(address);
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words.at(address);
    }
    float ReadFloat(std::uint32_t address) override {
        return floats.at(address);
    }
    pal4::EffectPosition ObjectPosition(
        std::uint32_t handle) override {
        calls.push_back("position");
        return positions.at(handle);
    }
    std::uint32_t AuxiliarySingleton() override { return auxiliary; }
    std::uint32_t CurrentCamera() override { return ReadWord(0x950cd0u); }
    std::uint32_t FindElement7(std::uint32_t tree,
                               std::uint32_t key) override {
        Check(tree == auxiliary + 28u && key == 0x1000u,
              "lookup uses auxiliary tree and object key");
        calls.push_back("lookup");
        return candidate;
    }
    void InvokeEffect(std::uint32_t node,
                      std::uint32_t object) override {
        Check(node == candidate && object == 0x1000u,
              "node callback receives object");
        calls.push_back("invoke");
    }
    void AddEffect(std::uint32_t owner,
                   std::uint32_t atomic) override {
        Check(owner == auxiliary && atomic == 0x1234u,
              "add receives singleton and atomic");
        calls.push_back("add");
    }
    void RemoveEffect(std::uint32_t owner,
                      std::uint32_t atomic) override {
        Check(owner == auxiliary && atomic == 0x1234u,
              "remove receives singleton and atomic");
        calls.push_back("remove");
    }
    std::uint32_t ForAllAtomics(
        std::uint32_t clump, std::uint8_t enabled,
        const std::function<std::uint32_t(std::uint32_t,
                                          std::uint8_t)>& callback) override {
        Check(clump == 0x4567u && enabled == 2u,
              "all-atomics preserves both native arguments");
        calls.push_back("all");
        Check(callback(0x1234u, enabled) == 0x1234u,
              "traversal invokes the registered atomic callback");
        return all_result;
    }
};

void TestGate() {
    Runtime runtime;
    runtime.bytes[0x5004u] = 0u;
    Check(pal4::EffectSystemCallbacks::ValidateObjectTransform(
              0x5000u, 0x1000u, {}, runtime) &&
              runtime.calls.empty(),
          "inactive gate does not resolve transform");
    runtime.bytes[0x5004u] = 1u;
    runtime.words[0x1004u] = 0x6000u;
    runtime.positions[0x6000u] = {1.0f, 2.0f, 3.0f};
    runtime.floats[0x5008u] = 25.0f;
    Check(pal4::EffectSystemCallbacks::ValidateObjectTransform(
              0x5000u, 0x1000u, {4.0f, 6.0f, 3.0f}, runtime),
          "squared radius includes its boundary");
    runtime.floats[0x5008u] = 24.0f;
    Check(!pal4::EffectSystemCallbacks::ValidateObjectTransform(
              0x5000u, 0x1000u, {4.0f, 6.0f, 3.0f}, runtime),
          "outside radius is rejected");
    runtime.floats[0x5008u] = 30.0f;
    Check(!pal4::EffectSystemCallbacks::ValidateObjectTransform(
              0x5000u, 0x1000u,
              {std::numeric_limits<float>::quiet_NaN(), 0.0f, 0.0f},
              runtime),
          "unordered x87 comparison is rejected");
}

void TestEffectObject() {
    Runtime runtime;
    runtime.words = {{0x2020u, 0x4000u}, {0x300cu, 0x1000u},
                     {0x1004u, 0x6000u}, {0x950cd0u, 0x7000u},
                     {0x7004u, 0x8000u}};
    runtime.positions = {{0x6000u, {0.0f, 0.0f, 0.0f}},
                         {0x8000u, {3.0f, 4.0f, 0.0f}}};
    runtime.floats[0x2008u] = 25.0f;
    Check(pal4::EffectSystemCallbacks::ProcessEffectObject(
              0x1000u, runtime) == 0x1000u &&
              runtime.calls ==
                  std::vector<std::string>{"lookup", "position",
                                           "position", "invoke"},
          "matching callback fires at squared-radius boundary");
    runtime.calls.clear();
    runtime.floats[0x2008u] = 24.0f;
    (void)pal4::EffectSystemCallbacks::ProcessEffectObject(0x1000u, runtime);
    Check(runtime.calls.back() == "position" &&
              runtime.calls.size() == 3u,
          "distant object skips callback");
    runtime.calls.clear();
    runtime.candidate = 0x4000u;
    (void)pal4::EffectSystemCallbacks::ProcessEffectObject(0x1000u, runtime);
    Check(runtime.calls == std::vector<std::string>{"lookup"},
          "missing tree key skips transforms");
    runtime.candidate = 0x3000u;
    runtime.words[0x300cu] = 0x1001u;
    runtime.calls.clear();
    (void)pal4::EffectSystemCallbacks::ProcessEffectObject(0x1000u, runtime);
    Check(runtime.calls == std::vector<std::string>{"lookup"},
          "lower-bound candidate above key is not exact");
    runtime.auxiliary = 0u;
    bool threw = false;
    try {
        (void)pal4::EffectSystemCallbacks::ProcessEffectObject(
            0x1000u, runtime);
    } catch (const std::logic_error&) {
        threw = true;
    }
    Check(threw, "absent auxiliary singleton throws");
}

void TestAtomicDispatch() {
    Runtime runtime;
    Check(pal4::EffectSystemCallbacks::ProcessAtomic(
              0x1234u, 1u, runtime) == 0x1234u &&
              pal4::EffectSystemCallbacks::ProcessAtomic(
                  0x1234u, 0u, runtime) == 0x1234u &&
              runtime.calls == std::vector<std::string>{"add", "remove"},
          "one atomic dispatches both branches");
    Check(pal4::EffectSystemCallbacks::ProcessAllAtomics(
              0x4567u, 2u, runtime) == runtime.all_result &&
              runtime.calls ==
                  std::vector<std::string>{"add", "remove", "all", "add"},
          "all-atomics wrapper invokes callback and returns traversal result");
    runtime.auxiliary = 0u;
    bool threw = false;
    try {
        (void)pal4::EffectSystemCallbacks::ProcessAtomic(
            0x1234u, 1u, runtime);
    } catch (const std::logic_error&) {
        threw = true;
    }
    Check(threw, "atomic callback requires auxiliary singleton");
}

}  // namespace

int main() {
    TestGate();
    TestEffectObject();
    TestAtomicDispatch();
}
