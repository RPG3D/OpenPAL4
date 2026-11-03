#pragma once

#include <cstdint>
#include <string>

namespace pal4 {

class IExceptionInfoRuntime {
public:
    virtual ~IExceptionInfoRuntime() = default;
    virtual void FreeBaseBuffer(std::uint32_t address) = 0;
};

class IExceptionInfoCopyRuntime {
public:
    virtual ~IExceptionInfoCopyRuntime() = default;
    [[nodiscard]] virtual std::uint32_t DuplicateBaseBuffer(
        std::uint32_t address) = 0;
};

class ExceptionInfo final {
public:
    static constexpr std::uint32_t kDerivedVtable = 0x84195cu;
    static constexpr std::uint32_t kBaseVtable = 0x867384u;

    ExceptionInfo& Format(const char* initial, const char* function,
                          const char* file, int line);
    void Destroy(IExceptionInfoRuntime& runtime);
    ExceptionInfo& CopyFrom(const ExceptionInfo& source,
                            IExceptionInfoCopyRuntime& runtime);
    void SetBaseBuffer(std::uint32_t address, std::uint32_t owned);
    void SetExtraField(std::uint32_t value) { extra_field_ = value; }

    [[nodiscard]] std::uint32_t Vtable() const { return vtable_; }
    [[nodiscard]] std::uint32_t BaseBuffer() const { return base_buffer_; }
    // IDA 0x4d51d0: std::exception::what returns its pointer or empty text.
    [[nodiscard]] std::uint32_t BaseWhatAddress() const;
    [[nodiscard]] std::uint32_t BaseBufferOwned() const { return base_buffer_owned_; }
    [[nodiscard]] std::uint32_t ExtraField() const { return extra_field_; }
    [[nodiscard]] const std::string& Text() const { return text_; }
    [[nodiscard]] const char* TextPointer() const { return text_.c_str(); }

private:
    std::uint32_t vtable_ = 0u;
    std::uint32_t base_buffer_ = 0u;
    std::uint32_t base_buffer_owned_ = 0u;
    std::uint32_t extra_field_ = 0u;
    std::string text_;
};

}  // namespace pal4
