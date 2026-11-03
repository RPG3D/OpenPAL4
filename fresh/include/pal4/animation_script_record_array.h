#pragma once

#include "pal4/animation_script_record.h"

#include <cstdint>

namespace pal4 {

struct AnimationScriptRecordArrayState final {
    std::uint32_t reserved = 0;
    std::uint32_t begin = 0;
    std::uint32_t end = 0;
    std::uint32_t capacity = 0;
};
static_assert(sizeof(AnimationScriptRecordArrayState) == 16);

class IAnimationScriptRecordArrayRuntime {
public:
    virtual ~IAnimationScriptRecordArrayRuntime() = default;
    virtual void SetErrorFlag(std::uint32_t string_field,
                              std::uint8_t flag) = 0;
    virtual void Free(std::uint32_t address) = 0;
};

class AnimationScriptRecordArrayRuntimeStub final
    : public IAnimationScriptRecordArrayRuntime {
public:
    void SetErrorFlag(std::uint32_t, std::uint8_t) override {}
    void Free(std::uint32_t) override {}
};

class IAnimationScriptRecordInsertRuntime
    : public IAnimationScriptRecordCopyRuntime {
public:
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    virtual void CopyRecord(std::uint32_t destination,
                            std::uint32_t source) {
        AnimationScriptRecord::CopyConstruct(destination, source, *this);
    }
};

class AnimationScriptRecordInsertRuntimeStub final
    : public IAnimationScriptRecordInsertRuntime {
public:
    AnimationScriptRecordState& RecordAt(std::uint32_t) override {
        return record_;
    }
    std::uint8_t ReadReference(std::uint32_t) override { return 0; }
    void WriteReference(std::uint32_t, std::uint8_t) override {}
    std::uint32_t Free(std::uint32_t) override { return 0; }
    std::uint32_t Allocate(std::uint32_t) override { return 0; }
    void AssignString(std::uint32_t, std::uint32_t) override {}
    void EraseString(std::uint32_t, std::uint32_t,
                     std::uint32_t) override {}
    std::uint32_t EmptyText() override { return 0; }
    bool AllocateText(std::uint32_t, std::uint32_t,
                      bool) override { return false; }
    void CopyText(std::uint32_t, std::uint32_t,
                  std::uint32_t) override {}
    void SetTextSize(std::uint32_t, std::uint32_t) override {}

private:
    AnimationScriptRecordState record_{};
};

class AnimationScriptRecordArray final {
public:
    static constexpr std::uint32_t kRecordStride = 32u;

    [[nodiscard]] static std::int32_t Count(
        const AnimationScriptRecordArrayState& state);
    [[nodiscard]] static std::uint32_t AddressAt(
        const AnimationScriptRecordArrayState& state, std::uint32_t index);

    [[nodiscard]] static int Release(AnimationScriptRecordArrayState& state,
                                     IAnimationScriptRecordArrayRuntime& runtime);
    [[nodiscard]] static std::uint32_t ClearRecords(
        AnimationScriptRecordArrayState& state,
        IAnimationScriptRecordRuntime& runtime);
    [[nodiscard]] static int ReleaseData(
        AnimationScriptRecordArrayState& state,
        IAnimationScriptRecordRuntime& runtime);
    static void Insert(AnimationScriptRecordArrayState& state,
                       std::uint32_t position, std::uint32_t count,
                       std::uint32_t source_record,
                       IAnimationScriptRecordInsertRuntime& runtime);
};

}  // namespace pal4
