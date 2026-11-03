#include "pal4/game_data_progress_ratio.h"

namespace pal4 {

double GameDataProgressRatio::Calculate(
    IGameDataProgressRatioRuntime& runtime) {
    const auto database = runtime.GameDatabase();
    const auto owner = runtime.ReadWord(database + 0x60u);
    const auto collection = owner + 0x5cu;
    const auto begin = runtime.ReadWord(collection + 4u);
    const auto count = begin != 0u
        ? static_cast<std::int32_t>(
              runtime.ReadWord(collection + 8u) - begin) / 400
        : 0;
    const auto numerator = runtime.ReadWord(runtime.GlobalData() + 0xb0u);
    return static_cast<double>(numerator) /
           static_cast<double>(count);
}

double GameDataProgressRatio::CalculateItems(
    IGameDataProgressRatioRuntime& runtime) {
    const auto database = runtime.GameDatabase();
    const auto owner = runtime.ReadWord(database + 4u);
    const auto collection = owner + 0x5cu;
    const auto begin = runtime.ReadWord(collection + 4u);
    const auto count = begin != 0u
        ? static_cast<std::int32_t>(
              runtime.ReadWord(collection + 8u) - begin) / 168
        : 0;
    const auto numerator = runtime.ReadWord(runtime.GlobalData() + 0xa0u);
    return static_cast<double>(numerator) /
           static_cast<double>(count);
}

}  // namespace pal4
