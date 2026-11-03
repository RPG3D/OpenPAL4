#include "pal4/collection_object_operations.h"

#include <array>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::CollectionObjectRuntimeStub {
    struct World final : pal4::CollectionWorldRuntimeStub {
        std::vector<char> events;
        void PrepareGeometry(pal4::CollectionState&,
                             std::size_t offset) override {
            Check(offset == 0xdcu,
                  "add reaches nested geometry preparation");
            events.push_back('G');
        }
        int CompareName(pal4::CollectionWorldState&,
                        std::string_view name) override {
            Check(name == "scene", "add forwards resolved name text");
            events.push_back('C');
            return 0;
        }
    } world;
    std::vector<char> events;
    std::vector<std::uint32_t> rotated;
    double elapsed{};
    float render_elapsed{};
    std::uint32_t created_storage{0x5000u};
    std::uint32_t clump{0x7000u};
    std::uint32_t loaded_id{};
    std::uint32_t loaded_object{};
    std::uint32_t loaded_variant{};
    std::uint32_t event_object{};
    std::uint32_t deleted_object{};
    std::uint32_t* expected_end{};

    std::uint32_t ActiveCameraWord() override {
        events.push_back('A');
        return 0x91u;
    }
    std::string_view NameText(std::uint32_t name) override {
        Check(name == 0x34u, "name text resolves original address");
        events.push_back('N');
        return "scene";
    }
    pal4::ICollectionWorldRuntime& WorldRuntime() override {
        events.push_back('D');
        return world;
    }
    int UpdateRenderParameters(pal4::CollectionState&, float seconds) override {
        render_elapsed = seconds;
        events.push_back('U');
        return -31;
    }
    void RotateComponent(std::uint32_t object, double seconds) override {
        Check(seconds == elapsed, "rotate forwards the same elapsed double");
        rotated.push_back(object);
        events.push_back('R');
    }
    int RenderWorld(pal4::CollectionState&, int argument) override {
        Check(argument == 73, "world thunk forwards its argument");
        events.push_back('W');
        return 29;
    }
    int SortAtomicGeometries(pal4::CollectionState&,
                             std::size_t offset) override {
        Check(offset == 0xdcu,
              "atomic thunk passes nested +0xc4 / collection +0xdc");
        events.push_back('S');
        return -23;
    }
    std::uint32_t AllocateGameObject(std::size_t size) override {
        Check(size == 0x184u, "game object allocation size is 0x184");
        events.push_back('a');
        return created_storage;
    }
    std::uint32_t ConstructComponent(std::uint32_t storage) override {
        Check(storage == 0x5000u, "component constructor receives allocation");
        events.push_back('c');
        return 0x5100u;
    }
    void LoadComponent(std::uint32_t object, std::uint32_t id,
                       std::uint32_t prefix, std::uint32_t name,
                       std::uint32_t variant, std::uint32_t zero) override {
        Check(prefix == 0x111u && name == 0x222u && zero == 0u,
              "component load forwards caller strings and zero");
        loaded_object = object;
        loaded_id = id;
        loaded_variant = variant;
        events.push_back('l');
    }
    std::uint32_t WorldMember(pal4::CollectionState&) override {
        events.push_back('w');
        return 0x6100u;
    }
    void AddToWorld(std::uint32_t object, std::uint32_t world) override {
        Check(object == loaded_object && world == 0x6100u,
              "world insertion receives created component and world member");
        events.push_back('i');
    }
    void ProcessEvent(pal4::CollectionState&, std::uint32_t* vector_end,
                      std::uint32_t mode, std::uint32_t& object) override {
        Check(vector_end == expected_end && mode == 1u &&
                  object == loaded_object,
              "event processing receives vector end, mode and object slot");
        event_object = object;
        object = object ? 0x5200u : 0u;
        events.push_back('e');
    }
    std::uint32_t ObjectClump(std::uint32_t object) override {
        Check(object == (event_object ? 0x5200u : 0u),
              "post-event object is used for clump lookup");
        events.push_back('q');
        return clump;
    }
    void ProcessClumpAtomics(std::uint32_t value,
                             pal4::CollectionState&,
                             std::size_t offset) override {
        Check(value == clump && offset == 0xdcu,
              "clump atomics receive collection +0xdc data");
        events.push_back('p');
    }
    void HideHookAtomics(std::uint32_t value) override {
        Check(value == clump, "hook hiding receives same clump");
        events.push_back('h');
    }
    std::uint32_t ObjectId(std::uint32_t object,
                            std::size_t offset) override {
        Check(offset == 0x170u, "ID lookup reads component +0x170");
        events.push_back('f');
        if (object == 0x100u || object == 0x300u) return 7u;
        if (object == 0x200u) return 9u;
        return 0u;
    }
    void RemoveFromWorld(std::uint32_t object) override {
        deleted_object = object;
        events.push_back('r');
    }
    void CleanupComponent(std::uint32_t object) override {
        Check(object == deleted_object,
              "component cleanup follows world removal for same object");
        events.push_back('k');
    }
    void DeleteComponent(std::uint32_t object,
                         std::uint32_t flags) override {
        Check(object == deleted_object && flags == 1u,
              "virtual deleting destructor receives flag one");
        events.push_back('d');
    }
};

void CheckDispatchAndRotation() {
    pal4::CollectionState state{};
    Runtime runtime;
    Check(pal4::CollectionObjectOperations::AddElement(
              state, 0x12u, 0x34u, runtime) == 0 &&
              runtime.events == std::vector<char>{'A', 'N', 'D'} &&
              runtime.world.events == std::vector<char>{'G', 'C'},
          "add resolves camera and name before nested initializer");
    runtime.events.clear();
    std::array<std::uint32_t, 2> elements{0x100u, 0x200u};
    state.first = elements.data();
    state.last = elements.data() + 2;
    runtime.elapsed = 1.25;
    pal4::CollectionObjectOperations::RotateGameObjects(state, 1.25, runtime);
    Check(runtime.events == std::vector<char>{'U', 'R', 'R'} &&
              runtime.rotated ==
                  std::vector<std::uint32_t>{0x100u, 0x200u} &&
              runtime.render_elapsed == 1.25f,
          "rotation updates render parameters before each component");
    runtime.events.clear();
    state.first = nullptr;
    state.last = nullptr;
    pal4::CollectionObjectOperations::RotateGameObjects(state, 2.0, runtime);
    Check(runtime.events == std::vector<char>{'U'} &&
              runtime.rotated.size() == 2u,
          "empty collection still updates render parameters");
    runtime.events.clear();
    Check(pal4::CollectionObjectOperations::UpdateRenderParameters(
              state, 1.234567890123, runtime) == -31 &&
              runtime.render_elapsed ==
                  static_cast<float>(1.234567890123) &&
              runtime.events == std::vector<char>{'U'},
          "render update narrows double to float and forwards result");
    runtime.events.clear();
    Check(pal4::CollectionObjectOperations::GetGameObjectCount(
              state, 73, runtime) == 29 &&
              pal4::CollectionObjectOperations::RenderWorldThunk(
                  state, 73, runtime) == 29 &&
              pal4::CollectionObjectOperations::GetGameObject(
                  state, -4, runtime) == -23 &&
              runtime.events == std::vector<char>{'W', 'W', 'S'},
          "world and atomic thunks forward their respective results");
}

void CheckCreate() {
    std::array<std::uint32_t, 2> entries{};
    pal4::CollectionState state{};
    state.first = entries.data();
    state.last = entries.data() + 1;
    state.base.references = 41u;
    Runtime runtime;
    runtime.expected_end = state.last;
    Check(pal4::CollectionObjectOperations::CreateGameObject(
              state, 0x111u, 0x222u, 5u, runtime) == 0x5200u &&
              runtime.events == std::vector<char>{
                  'a', 'c', 'l', 'w', 'i', 'e', 'q', 'p', 'h'} &&
              runtime.loaded_id == 41u &&
              runtime.loaded_variant == 5u &&
              state.base.references == 42u,
          "creation increments ID and processes post-event clump");
    runtime.events.clear();
    runtime.created_storage = 0u;
    runtime.clump = 0u;
    Check(pal4::CollectionObjectOperations::CreateGameObject(
              state, 0x111u, 0x222u, 6u, runtime) == 0u &&
              runtime.events == std::vector<char>{
                  'a', 'l', 'w', 'i', 'e', 'q'} &&
              runtime.loaded_object == 0u &&
              runtime.loaded_id == 42u &&
              state.base.references == 43u,
          "null allocation skips construction but preserves observed calls");
}

void CheckFindAndRemove() {
    std::array<std::uint32_t, 3> elements{0x100u, 0x200u, 0x300u};
    pal4::CollectionState state{};
    state.first = elements.data();
    state.last = elements.data() + elements.size();
    Runtime runtime;
    Check(pal4::CollectionObjectOperations::FindGameObjectById(
              state, 9u, runtime) == 0x200u &&
              pal4::CollectionObjectOperations::FindGameObjectById(
                  state, 8u, runtime) == 0u,
          "lookup returns matching object and zero on a miss");
    runtime.events.clear();
    auto* old_end = state.last;
    Check(pal4::CollectionObjectOperations::RemoveGameObjectById(
              state, 7u, runtime) == old_end &&
              state.last == elements.data() + 2 &&
              elements[0] == 0x200u && elements[1] == 0x300u &&
              runtime.events == std::vector<char>{'f', 'r', 'k', 'd'},
          "removal deletes first match and shifts following DWORDs");
    runtime.events.clear();
    old_end = state.last;
    Check(pal4::CollectionObjectOperations::RemoveGameObjectById(
              state, 8u, runtime) == old_end &&
              state.last == old_end &&
              runtime.events == std::vector<char>{'f', 'f'},
          "missing ID does not change collection");
    runtime.events.clear();
    Check(pal4::CollectionObjectOperations::RemoveGameObjectById(
              state, 7u, runtime) == old_end &&
              state.last == elements.data() + 1 &&
              runtime.events == std::vector<char>{'f', 'f', 'r', 'k', 'd'},
          "last-element removal decrements end without shifting");
    state.first = state.last = nullptr;
    runtime.events.clear();
    Check(pal4::CollectionObjectOperations::FindGameObjectById(
              state, 7u, runtime) == 0u &&
              pal4::CollectionObjectOperations::RemoveGameObjectById(
                  state, 7u, runtime) == nullptr &&
              runtime.events.empty(),
          "empty collection does not inspect object IDs");
}

}  // namespace

int main() {
    CheckDispatchAndRotation();
    CheckCreate();
    CheckFindAndRemove();
    std::cout << "collection object operations tests passed\n";
}
