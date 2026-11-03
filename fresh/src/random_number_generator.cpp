#include "pal4/random_number_generator.h"
#include "pal4/comparison_mt_trace.h"
#ifdef _WIN32
extern "C" __declspec(dllimport) unsigned long __stdcall GetCurrentThreadId(void);
#endif

namespace pal4 {
namespace {

constexpr std::uint32_t kMatrixA = 0x9908b0dfu;

std::uint32_t Twist(const std::uint32_t current,
                    const std::uint32_t following,
                    const std::uint32_t distant) {
    const std::uint32_t joined =
        (current & 0x80000000u) | (following & 0x7fffffffu);
    return distant ^ (joined >> 1u) ^ ((joined & 1u) ? kMatrixA : 0u);
}

}  // namespace

RandomNumberGenerator::RandomNumberGenerator() = default;

void RandomNumberGenerator::Seed(const std::uint32_t value) {
    state_[0] = value;
    for (std::uint32_t index = 1u; index < kStateCount; ++index) {
        state_[index] = 69069u * state_[index - 1u];
    }
    index_ = kStateCount;
}

void RandomNumberGenerator::Refresh() {
    for (std::uint32_t index = 0u; index < 227u; ++index) {
        state_[index] = Twist(state_[index], state_[index + 1u],
                              state_[index + 397u]);
    }
    for (std::uint32_t index = 227u; index < 623u; ++index) {
        state_[index] = Twist(state_[index], state_[index + 1u],
                              state_[index - 227u]);
    }
    state_[623u] = Twist(state_[623u], state_[0u], state_[396u]);
    index_ = 0u;
}

std::uint32_t RandomNumberGenerator::Next(std::source_location source) {
    const auto before=index_;
    if (index_ >= kStateCount) {
        if (index_ == kStateCount + 1u) {
            Seed(kDefaultSeed);
        }
        Refresh();
    }
    std::uint32_t value = state_[index_++];
    value ^= value >> 11u;
    value ^= (value & 0xff3a58adu) << 7u;
    value ^= (value & 0xffffdf8cu) << 15u;
    value ^= value >> 18u;
    auto& trace=GetComparisonMtTrace();
    if(trace.Enabled()){
        std::uint32_t thread{};
#ifdef _WIN32
        thread=GetCurrentThreadId();
#endif
        trace.Add({0u,value,thread,0u,before,index_,source.function_name(),
                   reinterpret_cast<std::uintptr_t>(this),source.line()});
    }
    return value;
}

}  // namespace pal4
