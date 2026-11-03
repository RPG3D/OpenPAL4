#include "pal4/game_data_class_fields.h"

namespace pal4 {

std::uint8_t GameDataClassFields::Initialize(
    std::uint32_t object, IGameDataClassFieldsRuntime& runtime) {
    constexpr std::uint32_t kFirstText = 0;
    constexpr std::uint32_t kSecondText = 1;
    const auto seed = static_cast<std::uint8_t>(object);

    const auto id_text = runtime.InitializeDirectText(kFirstText, seed, "ID");
    runtime.CreateEffect(object, kFirstText, 0);
    if (id_text) runtime.ReleaseText(kFirstText);

    runtime.ResetText(kFirstText, seed, 0);
    const auto class_text = runtime.AssignAllocatedText(kFirstText, "CLASS");
    runtime.CreateEffect(object, kFirstText, 4);
    if (class_text) runtime.ReleaseText(kFirstText);

    runtime.ResetText(kSecondText, seed, 0);
    (void)runtime.AssignAllocatedText(kSecondText, "TYPE");
    runtime.CreateEffect(object, kSecondText, 8);
    runtime.SetErrorFlag(kSecondText, 1);
    return 1;
}

bool GameDataClassFields::MembersInRange(
    std::uint32_t object, IGameDataClassFieldsRuntime& runtime) {
    return runtime.ReadWord(object + 4) <= 4u &&
           runtime.ReadWord(object + 8) <= 6u;
}

}  // namespace pal4
