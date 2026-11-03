#include "pal4/cegui_lifecycle.h"

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

class Runtime final : public pal4::ICeguiLifecycleRuntime {
public:
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint32_t> successors;
    std::vector<std::string> events;
    std::uint32_t system = 0x7000u;
    std::uint32_t images = 0x9000u;
    std::uint32_t pulse_result = 1u;

    std::uint32_t ReadDword(std::uint32_t address) override {
        return words.at(address);
    }
    void WriteDword(std::uint32_t address,
                    std::uint32_t value) override {
        words[address] = value;
    }
    void CleanupNodesRecursive(std::uint32_t base,
                               std::uint32_t root) override {
        Check(base == 0x1000u && root == 0x3000u,
              "recursive cleanup receives map base and root");
        events.emplace_back("cleanup-root");
    }
    std::uint32_t AdvanceCursor(std::uint32_t cursor) override {
        events.emplace_back("advance:" + std::to_string(cursor));
        return successors.at(cursor);
    }
    void DestroyNode(std::uint32_t base,
                     std::uint32_t node) override {
        Check(base == 0x1000u, "node destructor receives map base");
        events.emplace_back("destroy:" + std::to_string(node));
    }
    std::uint32_t Free(std::uint32_t address) override {
        events.emplace_back("free:" + std::to_string(address));
        return address + 1u;
    }
    std::uint32_t GetSystemSingletonPtr() override {
        events.emplace_back("system-pointer");
        return system;
    }
    std::uint32_t GetImageManager() override {
        events.emplace_back("image-manager");
        return images;
    }
    void DestroyAllImages(std::uint32_t manager) override {
        Check(manager == images, "image cleanup uses singleton manager");
        events.emplace_back("destroy-images");
    }
    void InvokeDeletingDestructor(std::uint32_t object,
                                  std::uint32_t flags) override {
        Check(flags == 1u, "virtual destructor uses deleting flag");
        events.emplace_back("delete:" + std::to_string(object));
    }
    void DestructSystem(std::uint32_t address) override {
        events.emplace_back("destruct:" + std::to_string(address));
    }
    std::uint32_t GetSystemSingleton() override {
        events.emplace_back("system-singleton");
        return system;
    }
    std::uint32_t InjectTimePulse(std::uint32_t address,
                                  float delta) override {
        Check(address == system && delta == 0.25f,
              "time pulse forwards singleton and float argument");
        events.emplace_back("pulse");
        return pulse_result;
    }
    void RenderGui(std::uint32_t address) override {
        Check(address == system, "render uses singleton system");
        events.emplace_back("render");
    }
};

void NonemptyTreeUsesRecursiveCleanup() {
    Runtime runtime;
    pal4::CeguiTreeGlobals globals{2u, 0x4000u};
    runtime.words[0x1004u] = 0x2000u;
    runtime.words[0x100cu] = 2u;
    runtime.words[0x2000u] = 0x2100u;
    runtime.words[0x2004u] = 0x3000u;
    Check(pal4::CeguiMapTreeLifecycle::Destroy(
              0x1000u, globals, runtime) == 1u &&
              runtime.events == std::vector<std::string>{
                  "cleanup-root", "free:8192"},
          "nonzero count cleans tree then releases header");
    Check(runtime.words.at(0x2000u) == 0x2000u &&
              runtime.words.at(0x2004u) == 0x4000u &&
              runtime.words.at(0x2008u) == 0x2000u &&
              runtime.words.at(0x1004u) == 0u &&
              runtime.words.at(0x100cu) == 0u &&
              globals.reference_count == 1u && globals.nil == 0x4000u,
          "header links reset and shared nil retained while referenced");
}

void EmptyCountResidualNodesAndFinalNil() {
    Runtime runtime;
    pal4::CeguiTreeGlobals globals{1u, 0x4000u};
    runtime.words[0x1004u] = 0x2000u;
    runtime.words[0x100cu] = 0u;
    runtime.words[0x2000u] = 0x2100u;
    runtime.successors[0x2100u] = 0x2200u;
    runtime.successors[0x2200u] = 0x2000u;
    Check(pal4::CeguiMapTreeLifecycle::Destroy(
              0x1000u, globals, runtime) == 0x4001u &&
              runtime.events == std::vector<std::string>{
                  "advance:8448", "destroy:8448", "advance:8704",
                  "destroy:8704", "free:8192", "free:16384"},
          "residual nodes advance before destruction and final nil is freed");
    Check(globals.reference_count == 0u && globals.nil == 0u,
          "last tree clears shared sentinel globals");
    runtime.events.clear();
    globals.reference_count = 0u;
    globals.nil = 0x4000u;
    runtime.words[0x1004u] = 0x2000u;
    runtime.words[0x2000u] = 0x2000u;
    Check(pal4::CeguiMapTreeLifecycle::Destroy(
              0x1000u, globals, runtime) == 0xffffffffu &&
              globals.nil == 0x4000u &&
              runtime.events == std::vector<std::string>{"free:8192"},
          "zero reference count underflows without releasing shared nil");
}

void SystemDestructionAndUiOrder() {
    Runtime runtime;
    Check(pal4::CeguiSystemLifecycle::DestroySystem(
              0x7000u, 2u, runtime) == 0x7000u &&
              runtime.events == std::vector<std::string>{
                  "destruct:28672"},
          "nondeleting system destructor returns object");
    runtime.events.clear();
    Check(pal4::CeguiSystemLifecycle::DestroySystem(
              0x7000u, 3u, runtime) == 0x7000u &&
              runtime.events == std::vector<std::string>{
                  "destruct:28672", "free:28672"},
          "deleting system destructor frees after object teardown");

    runtime.events.clear();
    runtime.words[0x701cu] = 0x8000u;
    runtime.words[0x5008u] = 0x6000u;
    runtime.words[0x5010u] = 0u;
    runtime.words[0x6000u] = 0x6000u;
    pal4::CeguiTreeGlobals globals{1u, 0u};
    Check(pal4::CeguiSystemLifecycle::DestroyUi(
              0x5000u, globals, runtime) == 0u &&
              runtime.events == std::vector<std::string>{
                  "system-pointer", "image-manager", "destroy-images",
                  "delete:28672", "delete:32768", "free:24576"},
          "UI teardown destroys images, system and provider before map");
    runtime.events.clear();
    runtime.words[0x701cu] = 0u;
    runtime.words[0x5008u] = 0x6000u;
    globals.reference_count = 1u;
    Check(pal4::CeguiSystemLifecycle::DestroyUi(
              0x5000u, globals, runtime) == 0u &&
              runtime.events == std::vector<std::string>{
                  "system-pointer", "image-manager", "destroy-images",
                  "delete:28672", "free:24576"},
          "missing provider skips its destructor");
}

void TimePulseAndRenderForward() {
    Runtime runtime;
    Check(pal4::CeguiSystemLifecycle::TimePulse(
              0.25f, runtime) == 1u &&
              runtime.events == std::vector<std::string>{
                  "system-singleton", "pulse"},
          "time pulse forwards float and result");
    runtime.events.clear();
    pal4::CeguiSystemLifecycle::Render(runtime);
    Check(runtime.events == std::vector<std::string>{
              "system-singleton", "render"},
          "render forwards to CEGUI singleton");
    pal4::CeguiLifecycleStub stub;
    Check(pal4::CeguiSystemLifecycle::TimePulse(
              0.0f, stub) == 0u,
          "linkable lifecycle stub runs without CEGUI");
    pal4::CeguiSystemLifecycle::Render(stub);
}

}  // namespace

int main() {
    NonemptyTreeUsesRecursiveCleanup();
    EmptyCountResidualNodesAndFinalNil();
    SystemDestructionAndUiOrder();
    TimePulseAndRenderForward();
    std::cout << "4 CEGUI lifecycle test cases passed\n";
}
