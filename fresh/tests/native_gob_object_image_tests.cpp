#include "pal4/gob_action.h"
#include "pal4/gob_effect.h"
#include "pal4/gob_get_item.h"
#include "pal4/gob_machine.h"
#include "pal4/gob_obstacle.h"
#include "pal4/native_gob_obstacle_config.h"
#include "pal4/gob_marker.h"
#include "pal4/gob_normal.h"
#include "pal4/gob_sound.h"
#include "pal4/native_gob_object_image.h"

#include <array>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string_view>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

class SchemaRuntime final : public pal4::IRwDictSchemaRuntime {
public:
    std::uint32_t ReadWord(std::uint32_t) override { return 0u; }
    std::uint32_t ListBegin(std::uint32_t) override { return 0u; }
    std::uint32_t ListEnd(std::uint32_t) override { return 0u; }
    void AdvanceUVAnim(std::uint32_t, float) override {}
    void ApplyUVAnim(std::uint32_t) override {}
    void DestroyList(std::uint32_t) override {}
    void DestroyDict(std::uint32_t) override {}
    std::uint32_t SetCurrentDict(std::uint32_t) override { return 0u; }
    std::uint32_t OpenStream(std::uint32_t, std::uint32_t,
                             const char*) override { return 0u; }
    bool FindChunk(std::uint32_t, std::uint32_t) override { return false; }
    std::uint32_t ReadDict(std::uint32_t) override { return 0u; }
    void CloseStream(std::uint32_t) override {}
    std::uint32_t CreateList(std::uint32_t, std::uint32_t) override {
        return 0u;
    }
    std::uint32_t ForAllAtomics(std::uint32_t,
        std::uint32_t (*)(std::uint32_t, std::uint32_t,
                          pal4::IRwDictSchemaRuntime&),
        std::uint32_t) override { return 0u; }
    std::uint32_t ForAllWorldMaterials(std::uint32_t,
        std::uint32_t (*)(std::uint32_t, std::uint32_t,
                          pal4::IRwDictSchemaRuntime&),
        std::uint32_t) override { return 0u; }
    std::uint32_t ForAllGeometryMaterials(std::uint32_t,
        std::uint32_t (*)(std::uint32_t, std::uint32_t,
                          pal4::IRwDictSchemaRuntime&),
        std::uint32_t) override { return 0u; }
    bool HasUVAnim(std::uint32_t) override { return false; }
    std::uint32_t ListAppend(std::uint32_t) override { return 0u; }
    void WriteWord(std::uint32_t, std::uint32_t) override {}
};

}  // namespace

int main() {
    pal4::NativeBaseObjectMemory memory;
    SchemaRuntime schema;
    std::srand(123u);(void)std::rand();const auto expected_next=std::rand();
    Check(pal4::NativeGobRandomDelay(0.f,0.f,123.f)==0.f && std::rand()==expected_next,
        "equal-bound GOB delay retains original CRT reseed and draw");
    constexpr std::array types{
        std::pair{0u, pal4::GobNormal::kVtable},
        std::pair{3u, pal4::GobSound::kVtable},
        std::pair{5u, pal4::GobMachine::kVtable},
        std::pair{6u, pal4::GobAction::kVtable},
        std::pair{7u, pal4::GobGetItem::kVtable},
        std::pair{8u, pal4::GobEffect::kVtable},
        std::pair{9u, pal4::GobMarker::kVtable},
        std::pair{10u, pal4::GobObstacle::kVtable},
    };
    for (const auto [type, vtable] : types) {
        const auto before = memory.OutstandingAllocations();
        {
            pal4::NativeGobObjectImage object(memory, schema);
            const auto address = object.Construct(type);
            Check(address != 0u && object.Type() == type &&
                      memory.ReadDword(address) == vtable,
                  "GOB image has the original type vtable");
            if (type == pal4::GobSound::TypeKey()) {
                Check(memory.ReadDword(address+0x1e8u)==0u,"Sound starts with an empty name record");
                object.AssignSoundName("WF025");
                object.AssignSoundName("WF036");
                Check(std::string(reinterpret_cast<const char*>(memory.ReadDword(address+0x1e8u)))=="WF036",
                    "Sound name replacement uses the retained record owner");
            }
            if (type == pal4::GobObstacle::TypeKey())
                Check(memory.ReadDword(address+0x32cu)==0xffffffffu && memory.ReadDword(address+0x330u)==0u,
                    "Obstacle effect handle uses the shared invalid-id sentinel");
            if (type == pal4::GobAction::TypeKey())
                memory.WriteDword(address +
                    pal4::GobAction::kAnimationOffset + 76u, 0u);
            if (type == pal4::GobMachine::TypeKey() || type == pal4::GobObstacle::TypeKey()) {
                memory.WriteDword(address + 0x278u, 1u);
                memory.WriteDword(address + 0x27cu, 1u);
                memory.WriteByte(address +
                    pal4::GobMachine::kPrimaryTasksOffset + 12u, 0u);
                memory.WriteByte(address +
                    pal4::GobMachine::kContextTasksOffset + 12u, 0u);
                Check(object.SetVisible(1u) == 0u &&
                          memory.ReadDword(address + 0x184u) == 1u &&
                          memory.ReadDword(address + 0x278u) == 0u &&
                          memory.ReadDword(address + 0x27cu) == 0u &&
                          memory.ReadByte(address +
                              pal4::GobMachine::kPrimaryTasksOffset + 12u) == 1u &&
                          memory.ReadByte(address +
                              pal4::GobMachine::kContextTasksOffset + 12u) == 1u &&
                          object.SetVisible(0u) == 0u &&
                          memory.ReadDword(address + 0x184u) == 0u,
                      "machine visibility clears both task collections");
                memory.WriteDword(address + 0x284u, 1u);
                bool rejected = false;
                try {
                    (void)object.SetVisible(1u);
                } catch (const std::logic_error& error) {
                    rejected = std::string_view(error.what()).find(
                        "machine repeat trigger") != std::string_view::npos;
                }
                Check(rejected && memory.ReadDword(address + 0x184u) == 0u,
                      "unbound machine repeat trigger preserves visibility");
                memory.WriteDword(address + 0x284u, 0u);
                memory.WriteDword(address +
                    pal4::GobMachine::kPrimaryTasksOffset + 8u, 1u);
                rejected = false;
                try {
                    (void)object.SetVisible(1u);
                } catch (const std::logic_error& error) {
                    rejected = std::string_view(error.what()).find(
                        "active GOB machine tasks") != std::string_view::npos;
                }
                Check(rejected && memory.ReadDword(address + 0x184u) == 0u,
                      "active machine tasks reject before visibility mutation");
                memory.WriteDword(address +
                    pal4::GobMachine::kPrimaryTasksOffset + 8u, 0u);
            } else if (type == pal4::GobEffect::TypeKey()) {
                Check(object.SetVisible(0u) == 0u &&
                          memory.ReadDword(address + 0x184u) == 0u,
                      "inactive effect visibility uses the effect override");
                bool rejected = false;
                try {
                    (void)object.SetVisible(1u);
                } catch (const std::logic_error& error) {
                    rejected = std::string_view(error.what()).find(
                        "effect creation") != std::string_view::npos;
                }
                Check(rejected &&
                          memory.ReadDword(address + 0x184u) == 0u,
                      "unbound effect creation preserves visibility");
                memory.WriteDword(address +
                    pal4::GobEffect::kEffectHandleOffset + 4u, 1u);
                rejected = false;
                try {
                    (void)object.SetVisible(0u);
                } catch (const std::logic_error& error) {
                    rejected = std::string_view(error.what()).find(
                        "active GOB effect handle") != std::string_view::npos;
                }
                Check(rejected && memory.ReadDword(address + 0x184u) == 0u,
                      "active effect handle rejects before visibility mutation");
                memory.WriteDword(address +
                    pal4::GobEffect::kEffectHandleOffset + 4u, 0u);
            } else {
                Check(object.SetVisible(1u) == 1u &&
                          memory.ReadDword(address + 0x184u) == 1u &&
                          (type != pal4::GobAction::TypeKey() ||
                           memory.ReadDword(address +
                               pal4::GobAction::kAnimationOffset + 76u) == 1u) &&
                          object.SetVisible(0u) == 0u &&
                          memory.ReadDword(address + 0x184u) == 0u,
                      "GOB image dispatches bound visibility paths");
            }
            if (type == pal4::GobMachine::TypeKey() || type == pal4::GobObstacle::TypeKey())
                Check(memory.ReadDword(address +
                          pal4::GobMachine::kPrimaryTasksOffset + 4u) != 0u &&
                          memory.ReadDword(address +
                          pal4::GobMachine::kContextTasksOffset + 4u) != 0u,
                      "machine GOB owns both task sentinels");
        }
        Check(memory.OutstandingAllocations() == before,
              "GOB image releases its nested allocations");
    }
}
