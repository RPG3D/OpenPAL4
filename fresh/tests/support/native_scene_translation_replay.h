#pragma once
#include <filesystem>
namespace pal4::test {
void ReplayNativeSceneTranslation(const std::filesystem::path& root,
                                 const std::filesystem::path& fixture);
void ProbeNativeSceneMovement(const std::filesystem::path& root,
                              const std::filesystem::path& fixture);
}
