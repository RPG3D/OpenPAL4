#pragma once

#include "pal4/string_buffer.h"

#include <cstdint>
#include <span>
#include <string>
#include <string_view>

namespace pal4 {

class IAnimationStringFileRuntime {
public:
    virtual ~IAnimationStringFileRuntime() = default;
    virtual void ReadLength(std::uint32_t file,
                            std::uint32_t& length) = 0;
    [[nodiscard]] virtual bool ReadPayload(std::uint32_t file,
                                           char* destination,
                                           std::uint32_t length) = 0;
    [[nodiscard]] virtual std::uint32_t AllocateTemporary(
        std::uint32_t bytes) = 0;
    virtual void FreeTemporary(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint8_t ReadReference(
        std::uint32_t address) = 0;
    virtual void WriteReference(std::uint32_t address,
                                std::uint8_t value) = 0;
    virtual void SetErrorFlag(StringBufferState& state,
                              std::uint8_t flag) = 0;
    virtual void Reallocate(StringBufferState& state,
                            std::uint32_t length) = 0;
    virtual void WriteData(std::uint32_t destination,
                           std::string_view data) = 0;
    virtual void WriteByte(std::uint32_t address,
                           std::uint8_t value) = 0;
};

class AnimationStringFileRuntimeStub final
    : public IAnimationStringFileRuntime {
public:
    void ReadLength(std::uint32_t, std::uint32_t&) override {}
    bool ReadPayload(std::uint32_t, char*,
                     std::uint32_t) override { return false; }
    std::uint32_t AllocateTemporary(std::uint32_t) override { return 0; }
    void FreeTemporary(std::uint32_t) override {}
    std::uint8_t ReadReference(std::uint32_t) override { return 0; }
    void WriteReference(std::uint32_t, std::uint8_t) override {}
    void SetErrorFlag(StringBufferState&, std::uint8_t) override {}
    void Reallocate(StringBufferState&, std::uint32_t) override {}
    void WriteData(std::uint32_t, std::string_view) override {}
    void WriteByte(std::uint32_t, std::uint8_t) override {}
};

class AnimationStringFileReader final {
public:
    // Host STL boundary for the same length-prefixed, first-NUL file format.
    [[nodiscard]] static bool Load(std::string& text,
                                   std::span<const std::uint8_t>& bytes);
    [[nodiscard]] static bool Load(StringBufferState& state,
                                   std::uint32_t file,
                                   IAnimationStringFileRuntime& runtime);
};

}  // namespace pal4
