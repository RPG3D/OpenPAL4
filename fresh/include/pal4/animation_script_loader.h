#pragma once

#include "pal4/animation_script_record_array.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

namespace pal4 {

class IAnimationScriptLoaderRuntime : public IAnimationScriptRecordRuntime {
public:
    virtual ~IAnimationScriptLoaderRuntime() = default;
    [[nodiscard]] virtual std::uint32_t Open(std::string_view path,
                                             std::string_view mode) = 0;
    [[nodiscard]] virtual bool EndOfFile(std::uint32_t file) = 0;
    [[nodiscard]] virtual std::string ReadLine(std::uint32_t file,
                                               std::size_t buffer_size) = 0;
    [[nodiscard]] virtual std::size_t EffectiveLength(
        std::string_view line) = 0;
    virtual void Insert(AnimationScriptRecordArrayState& state,
                        std::uint32_t position, std::uint32_t count,
                        std::string_view first, std::string_view second) = 0;
    virtual void Close(std::uint32_t file) = 0;
};

class AnimationScriptLoaderRuntimeStub final
    : public IAnimationScriptLoaderRuntime {
public:
    AnimationScriptRecordState& RecordAt(std::uint32_t) override {
        return record_;
    }
    std::uint8_t ReadReference(std::uint32_t) override { return 0; }
    void WriteReference(std::uint32_t, std::uint8_t) override {}
    std::uint32_t Free(std::uint32_t) override { return 0; }
    [[nodiscard]] std::uint32_t Open(std::string_view,
                                     std::string_view) override { return 0; }
    [[nodiscard]] bool EndOfFile(std::uint32_t) override { return true; }
    [[nodiscard]] std::string ReadLine(std::uint32_t,
                                       std::size_t) override { return {}; }
    [[nodiscard]] std::size_t EffectiveLength(
        std::string_view line) override { return line.size(); }
    void Insert(AnimationScriptRecordArrayState&, std::uint32_t,
                std::uint32_t, std::string_view,
                std::string_view) override {}
    void Close(std::uint32_t) override {}

private:
    AnimationScriptRecordState record_{};
};

class AnimationScriptLoader final {
public:
    [[nodiscard]] static int Load(AnimationScriptRecordArrayState& state,
                                  std::string_view source,
                                  IAnimationScriptLoaderRuntime& runtime);
};

}  // namespace pal4
