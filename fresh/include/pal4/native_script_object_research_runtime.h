#pragma once

#include "pal4/script_object_research.h"

namespace pal4 {

class NativeSceneGobAssets;

class NativeScriptObjectResearchRuntime final
    : public IScriptObjectResearchRuntime {
public:
    explicit NativeScriptObjectResearchRuntime(NativeSceneGobAssets& objects)
        : objects_(objects) {}

    [[nodiscard]] std::uint32_t ReadWord(std::uint32_t address) override;
    [[nodiscard]] std::uint32_t EmptyStringAddress() override;
    [[nodiscard]] std::uint32_t ObjectManager() override;
    [[nodiscard]] std::uint32_t FindByName(
        std::uint32_t manager, std::uint32_t name) override;
    void WriteWord(std::uint32_t address, std::uint32_t value) override;

private:
    NativeSceneGobAssets& objects_;
};

}  // namespace pal4
