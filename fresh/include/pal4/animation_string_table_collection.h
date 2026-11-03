#pragma once

#include "pal4/animation_string_table.h"

#include <cstdint>

namespace pal4 {

struct AnimationStringTableCollectionState final {
    std::uint32_t reserved = 0;
    std::uint32_t begin = 0;
    std::uint32_t end = 0;
    std::uint32_t limit = 0;
};
static_assert(sizeof(AnimationStringTableCollectionState) == 16);

class IAnimationStringTableCollectionRuntime {
public:
    virtual ~IAnimationStringTableCollectionRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadPointer(
        std::uint32_t slot) = 0;
    virtual void WritePointer(std::uint32_t slot,
                              std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t AllocatePointerArray(
        std::uint32_t bytes) = 0;
    [[nodiscard]] virtual IAnimationStringTableRuntime& TableRuntime() = 0;
    [[nodiscard]] virtual std::uint32_t OpenFile(std::uint32_t path) = 0;
    virtual void ReadWord(std::uint32_t file, std::uint32_t& value) = 0;
    virtual void CloseFile(std::uint32_t file) = 0;
    [[nodiscard]] virtual std::uint32_t AllocateTable() = 0;
    [[nodiscard]] virtual std::uint32_t PrepareEmptyTableTemplate(
        std::uint32_t path) = 0;
    virtual void ReleaseEmptyTableTemplate(std::uint32_t source) = 0;
};

class AnimationStringTableCollectionRuntimeStub final
    : public IAnimationStringTableCollectionRuntime {
public:
    std::uint32_t ReadPointer(std::uint32_t) override { return 0; }
    void WritePointer(std::uint32_t, std::uint32_t) override {}
    std::uint32_t AllocatePointerArray(std::uint32_t) override {
        return 0;
    }
    IAnimationStringTableRuntime& TableRuntime() override {
        return table_runtime_;
    }
    std::uint32_t OpenFile(std::uint32_t) override { return 0; }
    void ReadWord(std::uint32_t, std::uint32_t&) override {}
    void CloseFile(std::uint32_t) override {}
    std::uint32_t AllocateTable() override { return 0; }
    std::uint32_t PrepareEmptyTableTemplate(std::uint32_t) override {
        return 0;
    }
    void ReleaseEmptyTableTemplate(std::uint32_t) override {}

private:
    AnimationStringTableRuntimeStub table_runtime_{};
};

class AnimationStringTableCollection final {
public:
    [[nodiscard]] static bool LoadFromBytes(
        std::vector<AnimationStringTableData>& tables,
        std::span<const std::uint8_t> bytes);
    [[nodiscard]] static const AnimationStringTableData* Find(
        std::span<const AnimationStringTableData> tables, std::string_view name);
    [[nodiscard]] static std::uint32_t Release(
        AnimationStringTableCollectionState& state,
        IAnimationStringTableCollectionRuntime& runtime);
    [[nodiscard]] static std::uint32_t Find(
        const AnimationStringTableCollectionState& state,
        std::uint32_t name,
        IAnimationStringTableCollectionRuntime& runtime);
    [[nodiscard]] static std::uint32_t Clear(
        AnimationStringTableCollectionState& state,
        IAnimationStringTableCollectionRuntime& runtime);
    [[nodiscard]] static bool LoadFromFile(
        AnimationStringTableCollectionState& state,
        std::uint32_t path,
        IAnimationStringTableCollectionRuntime& runtime);
    [[nodiscard]] static std::uint32_t Insert(
        AnimationStringTableCollectionState& state,
        std::uint32_t position, std::uint32_t count,
        std::uint32_t value,
        IAnimationStringTableCollectionRuntime& runtime);
};

}  // namespace pal4
