#pragma once

#include <cstdint>

namespace pal4 {

class IJigsawWindowUiRuntime {
public:
    virtual ~IJigsawWindowUiRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteByte(std::uint32_t address,
                           std::uint8_t value) = 0;
    virtual void SetParent(std::uint32_t frame, std::uint8_t active,
                           std::uint32_t parent) = 0;
    [[nodiscard]] virtual std::uint32_t FrameManager() = 0;
    [[nodiscard]] virtual std::uint32_t InsertActive(
        std::uint32_t array, std::uint32_t value) = 0;
    virtual void RemoveActive(std::uint32_t array,
                              std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t CreateXmlDocument() = 0;
    [[nodiscard]] virtual std::uint32_t Game() = 0;
    virtual void InitializeStateAndUI(std::uint32_t game,
                                      std::uint32_t mode,
                                      std::uint32_t document) = 0;
    [[nodiscard]] virtual std::uint32_t DestroyXmlDocument(
        std::uint32_t document) = 0;
    virtual void HandleFrameEvent(std::uint32_t name,
                                  std::uint32_t type,
                                  std::uint32_t payload) = 0;
    [[nodiscard]] virtual std::uint32_t FindFrame(
        std::uint32_t manager, std::uint32_t name) = 0;
    virtual void SetStepWithTimer(std::uint32_t frame,
                                  std::uint32_t step) = 0;
};

class JigsawWindowUi final {
public:
    // IDA 0x49fe20.
    [[nodiscard]] static std::uint32_t SetVisible(
        std::uint32_t frame, std::uint8_t active,
        std::uint32_t parent, IJigsawWindowUiRuntime& runtime);
    // IDA 0x49fed0.
    [[nodiscard]] static std::uint8_t HandleShowMessage(
        std::uint32_t message, IJigsawWindowUiRuntime& runtime);
};

}  // namespace pal4
