#include "pal4/numeric_spinner_pool.h"

#include <new>
#include <string>

namespace pal4 {

std::uint32_t NumericSpinnerPool::CreateOrGet(
    const std::int32_t value, INumericSpinnerPoolRuntime& runtime) {
    const auto begin = runtime.ReadWord(kBegin);
    const auto end = runtime.ReadWord(kEnd);
    if (begin != 0u && end != begin) {
        const auto spinner = runtime.ReadWord(end - 4u);
        runtime.WriteWord(kEnd, end - 4u);
        runtime.UpdateValue(spinner, value);
        return spinner;
    }

    const auto serial = runtime.ReadWord(kSerial) + 1u;
    runtime.WriteWord(kSerial, serial);
    const auto name = std::string("TradeItem") +
                      std::to_string(static_cast<std::int32_t>(serial));
    const auto memory = runtime.Allocate(0x1e0u);
    return memory != 0u
               ? runtime.ConstructSpinner(memory, name, value) : 0u;
}

std::uint32_t NumericSpinnerPool::Append(
    const std::uint32_t spinner, INumericSpinnerPoolRuntime& runtime) {
    const auto begin = runtime.ReadWord(kBegin);
    const auto end = runtime.ReadWord(kEnd);
    const auto capacity_end = runtime.ReadWord(kCapacityEnd);
    if (capacity_end != end) {
        runtime.WriteWord(end, spinner);
        runtime.WriteWord(kEnd, end + 4u);
        return end + 4u;
    }

    const auto size = begin != 0u ? (end - begin) / 4u : 0u;
    const auto capacity = size + (size > 1u ? size : 1u);
    const auto replacement = runtime.Allocate(capacity * 4u);
    if (replacement == 0u) throw std::bad_alloc();
    for (std::uint32_t i = 0; i < size; ++i)
        runtime.WriteWord(replacement + 4u * i,
                          runtime.ReadWord(begin + 4u * i));
    runtime.WriteWord(replacement + 4u * size, spinner);
    runtime.Free(begin);
    runtime.WriteWord(kCapacityEnd, replacement + 4u * capacity);
    runtime.WriteWord(kBegin, replacement);
    runtime.WriteWord(kEnd, replacement + 4u * (size + 1u));
    return size;
}

std::uint32_t NumericSpinnerPool::Remove(
    const std::uint32_t spinner, INumericSpinnerPoolRuntime& runtime) {
    runtime.WriteWord(spinner + 476u, 0u);
    runtime.WriteWord(spinner + 316u, 0u);
    const auto child = runtime.ReadWord(spinner + 436u);
    const auto parent = runtime.ReadWord(child + 68u);
    if (parent != 0u) runtime.RemoveChild(parent, child);
    return Append(spinner, runtime);
}

std::uint32_t NumericSpinnerPool::Clear(
    INumericSpinnerPoolRuntime& runtime) {
    auto begin = runtime.ReadWord(kBegin);
    auto end = runtime.ReadWord(kEnd);
    for (std::uint32_t i = 0; begin != 0u &&
                              i < (end - begin) / 4u; ++i) {
        const auto spinner = runtime.ReadWord(begin + 4u * i);
        if (spinner != 0u) {
            runtime.DeleteSpinner(spinner, 1u);
            begin = runtime.ReadWord(kBegin);
            end = runtime.ReadWord(kEnd);
        }
    }
    runtime.WriteWord(kEnd, begin);
    return end;
}

std::uint32_t NumericSpinnerPool::Initialize(
    INumericSpinnerPoolRuntime& runtime) {
    for (std::uint32_t i = 0; i < 100u; ++i) {
        const auto spinner = CreateOrGet(0, runtime);
        (void)Append(spinner, runtime);
    }
    return 0u;
}

int NumericSpinnerPool::InitializeGlobalsAndRegisterCleanup(
    const std::uint32_t seed, INumericSpinnerPoolRuntime& runtime) {
    (void)InitializeGlobals(seed, runtime);
    return runtime.RegisterCleanup(0x4b18d0u);
}

int NumericSpinnerPool::InitializeGlobals(
    const std::uint32_t seed, INumericSpinnerPoolRuntime& runtime) {
    runtime.WriteByte(kInitialByte,
                      static_cast<std::uint8_t>(seed >> 24u));
    runtime.WriteWord(kBegin, 0u);
    runtime.WriteWord(kEnd, 0u);
    runtime.WriteWord(kCapacityEnd, 0u);
    return 0;
}

void NumericSpinnerPool::CleanupGlobals(
    INumericSpinnerPoolRuntime& runtime) {
    const auto guard = runtime.ReadByte(kCleanupGuard);
    if ((guard & 1u) != 0u) return;
    runtime.WriteByte(kCleanupGuard,
                      static_cast<std::uint8_t>(guard | 1u));
    runtime.Free(runtime.ReadWord(kBegin));
    runtime.WriteWord(kBegin, 0u);
    runtime.WriteWord(kEnd, 0u);
    runtime.WriteWord(kCapacityEnd, 0u);
}

}  // namespace pal4
