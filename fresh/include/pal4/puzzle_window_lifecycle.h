#pragma once

#include "pal4/jigsaw_window_storage.h"

#include <cstdint>

namespace pal4 {

class IPuzzleWindowStorageRuntime : public IJigsawWindowStorageRuntime {
public:
    virtual void ReleaseStepName(std::uint32_t string_buffer) = 0;
};

class PuzzleWindowStorage final {
public:
    // IDA 0x4a0f70.
    [[nodiscard]] static std::uint32_t Destruct(
        std::uint32_t frame, IPuzzleWindowStorageRuntime& runtime);
};

class IPuzzleWindowUiRuntime {
public:
    virtual ~IPuzzleWindowUiRuntime() = default;
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
    virtual void RemoveTimerByName(std::uint32_t queue,
                                   std::uint32_t name) = 0;
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
    virtual void SetStepCount(std::uint32_t frame,
                              std::uint32_t step) = 0;
};

class PuzzleWindowUi final {
public:
    // IDA 0x4a10a0.
    [[nodiscard]] static std::uint32_t SetVisible(
        std::uint32_t frame, std::uint8_t active,
        std::uint32_t parent, IPuzzleWindowUiRuntime& runtime);
    // IDA 0x4a1180.
    [[nodiscard]] static std::uint8_t HandleShowMessage(
        std::uint32_t message, IPuzzleWindowUiRuntime& runtime);
};

class IPuzzleWindowStepRuntime {
public:
    virtual ~IPuzzleWindowStepRuntime() = default;
    virtual void ProcessPuzzleStep(std::uint32_t puzzle,
                                   std::uint32_t step) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(
        std::uint32_t address) = 0;
    virtual void SetTextFromString(std::uint32_t window,
                                   std::uint32_t string) = 0;
    [[nodiscard]] virtual std::uint32_t FrameManager() = 0;
    virtual void ScheduleTimer(std::uint32_t queue,
                               std::uint32_t name,
                               std::uint32_t period_bits,
                               std::uint32_t frame,
                               std::uint32_t callback,
                               std::uint32_t mode) = 0;
    [[nodiscard]] virtual std::uint32_t UpdateDisplay(
        std::uint32_t frame) = 0;
};

class PuzzleWindowStep final {
public:
    // IDA 0x4a11f0.
    [[nodiscard]] static std::uint8_t SetStepCount(
        std::uint32_t frame, std::uint32_t step,
        IPuzzleWindowStepRuntime& runtime);
};

}  // namespace pal4
