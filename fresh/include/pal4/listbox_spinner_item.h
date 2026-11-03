#pragma once

#include <cstdint>
#include <string_view>

namespace pal4 {

class IListboxSpinnerItemRuntime {
public:
    virtual ~IListboxSpinnerItemRuntime() = default;
    [[nodiscard]] virtual std::string_view ReadCString(
        std::uint32_t address) = 0;
    virtual void ConstructBase(std::uint32_t item,
                               std::string_view text) = 0;
    [[nodiscard]] virtual std::uint32_t CreateOrGetSpinner(
        std::int32_t value) = 0;
    virtual void ResetSpinner(std::uint32_t spinner) = 0;
    virtual void RemoveSpinner(std::uint32_t spinner) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t BaseItemVtable() = 0;
    virtual void DestroyString(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t CallVirtual(
        std::uint32_t spinner, std::uint32_t slot) = 0;
    virtual void UpdateSpinnerValue(std::uint32_t spinner,
                                    std::int32_t value) = 0;
};

class ListboxSpinnerItem final {
public:
    static constexpr std::uint32_t kVtable = 0x844150u;

    // IDA 0x4b1910.
    [[nodiscard]] static std::uint32_t Construct(
        std::uint32_t item, std::int32_t value,
        IListboxSpinnerItemRuntime& runtime);
    // IDA 0x4b1a50; incorrectly identified as std::ios_base destructor.
    static void Destruct(std::uint32_t item,
                         IListboxSpinnerItemRuntime& runtime);
    // IDA 0x4b1ad0.
    [[nodiscard]] static std::uint32_t PrepareAndCallUpdate(
        std::uint32_t item, IListboxSpinnerItemRuntime& runtime);
    // IDA 0x4b1b10.
    [[nodiscard]] static std::uint32_t CallUpdate(
        std::uint32_t item, IListboxSpinnerItemRuntime& runtime);
    // IDA 0x4b1b40/0x4b1b50.
    [[nodiscard]] static std::int32_t GetCurrentValue(
        std::uint32_t item, IListboxSpinnerItemRuntime& runtime);
    static void SetCurrentValue(std::uint32_t item, std::int32_t value,
                                IListboxSpinnerItemRuntime& runtime);
    // IDA 0x4b1b60 with tail at 0x4b16e0.
    static void SetRange(std::uint32_t item, std::int32_t minimum,
                         std::int32_t maximum,
                         IListboxSpinnerItemRuntime& runtime);
    // IDA 0x4b1b70.
    [[nodiscard]] static std::uint32_t SetChangeCallback(
        std::uint32_t item, std::uint32_t first,
        std::uint32_t second, IListboxSpinnerItemRuntime& runtime);
};

}  // namespace pal4
