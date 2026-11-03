#pragma once

#include "pal4/pal_floor_attribute.h"

#include <cstddef>
#include <cstdint>

namespace pal4 {

struct FloorColorValueRecord {
    std::uint32_t key{};
    FloorTextRecord sound_path{};
};
static_assert(sizeof(FloorColorValueRecord) == 20u);

struct FloorColorNode {
    std::uint32_t left{};
    std::uint32_t parent{};
    std::uint32_t right{};
    std::uint32_t key{};
    FloorTextRecord sound_path{};
    std::uint32_t black{};
};
static_assert(sizeof(FloorColorNode) == 36u);
static_assert(offsetof(FloorColorNode, sound_path) == 16u);
static_assert(offsetof(FloorColorNode, black) == 32u);

class FloorColorValue final {
public:
    static void Copy(std::uint32_t destination, std::uint32_t source,
                     IPalFloorAttributeRuntime& runtime);
};

struct FloorColorInsertResult {
    std::uint32_t node{};
    bool inserted{};
};

class FloorColorTree final {
public:
    static constexpr std::uint32_t kNilAddress = 0x8e414cu;
    static constexpr std::uint32_t kOwnerCountAddress = 0x8e4150u;
    static void Initialize(PalFloorAttributeState& state,
                           std::uint8_t first_byte,
                           std::uint8_t second_byte,
                           bool allow_duplicates,
                           IPalFloorAttributeRuntime& runtime);
    static void ReleaseSharedNil(IPalFloorAttributeRuntime& runtime);
    [[nodiscard]] static FloorColorInsertResult Insert(
        FloorColorTreeState& tree, std::uint32_t value_record,
        IPalFloorAttributeRuntime& runtime);
    [[nodiscard]] static std::uint32_t InsertAt(
        FloorColorTreeState& tree, std::uint32_t child_hint,
        std::uint32_t parent, std::uint32_t value_record,
        IPalFloorAttributeRuntime& runtime);
    [[nodiscard]] static std::uint32_t RotateLeft(
        FloorColorTreeState& tree, std::uint32_t node,
        IPalFloorAttributeRuntime& runtime);
    [[nodiscard]] static std::uint32_t RotateRight(
        FloorColorTreeState& tree, std::uint32_t node,
        IPalFloorAttributeRuntime& runtime);
    [[nodiscard]] static std::uint32_t Leftmost(
        FloorColorTreeState& tree, std::uint32_t root,
        IPalFloorAttributeRuntime& runtime);
    [[nodiscard]] static std::uint32_t Previous(
        FloorColorTreeState& tree, std::uint32_t node,
        IPalFloorAttributeRuntime& runtime);
    [[nodiscard]] static std::uint32_t Next(
        FloorColorTreeState& tree, std::uint32_t node,
        IPalFloorAttributeRuntime& runtime);
    [[nodiscard]] static std::uint32_t Find(
        FloorColorTreeState& tree, std::uint32_t key,
        IPalFloorAttributeRuntime& runtime);
    [[nodiscard]] static std::uint32_t Remove(
        FloorColorTreeState& tree, std::uint32_t node,
        IPalFloorAttributeRuntime& runtime);
    [[nodiscard]] static std::uint32_t DestroySubtree(
        FloorColorTreeState& tree, std::uint32_t root,
        IPalFloorAttributeRuntime& runtime);
    [[nodiscard]] static std::uint32_t EraseRange(
        FloorColorTreeState& tree, std::uint32_t first,
        std::uint32_t last, IPalFloorAttributeRuntime& runtime);
};

}  // namespace pal4
