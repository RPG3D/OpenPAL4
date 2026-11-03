#pragma once

#include <cstdint>

namespace pal4 {

struct AnimationScriptRecordState final {
    std::uint32_t first_header = 0;
    std::uint32_t first_text = 0;
    std::uint32_t first_length = 0;
    std::uint32_t first_capacity = 0;
    std::uint32_t second_header = 0;
    std::uint32_t second_text = 0;
    std::uint32_t second_length = 0;
    std::uint32_t second_capacity = 0;
};
static_assert(sizeof(AnimationScriptRecordState) == 32);

class IAnimationScriptRecordRuntime {
public:
    virtual ~IAnimationScriptRecordRuntime() = default;
    [[nodiscard]] virtual AnimationScriptRecordState& RecordAt(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint8_t ReadReference(
        std::uint32_t address) = 0;
    virtual void WriteReference(std::uint32_t address,
                                std::uint8_t value) = 0;
    [[nodiscard]] virtual std::uint32_t Free(std::uint32_t address) = 0;
};

class AnimationScriptRecordRuntimeStub final
    : public IAnimationScriptRecordRuntime {
public:
    AnimationScriptRecordState& RecordAt(std::uint32_t) override {
        return record_;
    }
    std::uint8_t ReadReference(std::uint32_t) override { return 0; }
    void WriteReference(std::uint32_t, std::uint8_t) override {}
    std::uint32_t Free(std::uint32_t) override { return 0; }

private:
    AnimationScriptRecordState record_{};
};

class IAnimationScriptRecordCopyRuntime
    : public IAnimationScriptRecordRuntime {
public:
    virtual void AssignString(std::uint32_t destination,
                              std::uint32_t source) = 0;
    virtual void EraseString(std::uint32_t field,
                             std::uint32_t position,
                             std::uint32_t count) = 0;
    [[nodiscard]] virtual std::uint32_t EmptyText() = 0;
    [[nodiscard]] virtual bool AllocateText(std::uint32_t field,
                                            std::uint32_t length,
                                            bool exact) = 0;
    virtual void CopyText(std::uint32_t destination,
                          std::uint32_t source,
                          std::uint32_t length) = 0;
    virtual void SetTextSize(std::uint32_t field,
                             std::uint32_t length) = 0;
};

class AnimationScriptRecord final {
public:
    [[nodiscard]] static std::uint32_t Cleanup(
        AnimationScriptRecordState& record,
        IAnimationScriptRecordRuntime& runtime);
    static void CopyConstruct(std::uint32_t destination,
                              std::uint32_t source,
                              IAnimationScriptRecordCopyRuntime& runtime);
};

}  // namespace pal4
