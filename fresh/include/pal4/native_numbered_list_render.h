#pragma once

#include "pal4/numbered_list_item_render.h"
#include <windows.h>

namespace pal4 {

// Borrows valid native item/image/cache addresses and game services. All CEGUI
// colour, rectangle and rendering-cache operations call the original DLL.
class NativeNumberedListRenderRuntime final : public INumberedListRenderRuntime {
public:
    NativeNumberedListRenderRuntime(HMODULE module, INumberedListRenderDataRuntime& data);
    std::uint32_t ReadWord(std::uint32_t address) override;
    void WriteWord(std::uint32_t address, std::uint32_t value) override;
    std::uint8_t ReadByte(std::uint32_t address) override;
    float ReadFloat(std::uint32_t address) override;
    std::string ReadCString(std::uint32_t address) override;
    std::uint32_t GetSystemFont() override;
    std::uint32_t ModulatedBlack(std::uint32_t item, float modulation) override;
    void CacheWhiteImage(std::uint32_t cache, std::uint32_t image,
        const std::array<float, 4>& rect, float alpha, const std::array<float, 4>& clip,
        bool first_flag, bool second_flag) override;
    void CacheItemText(std::uint32_t cache, std::uint32_t text_address,
        std::uint32_t font, std::uint32_t format, const std::array<float, 4>& rect,
        float alpha, const std::array<float, 4>& clip, std::uint32_t colour) override;
    void CacheBlackText(std::uint32_t cache, std::string_view text,
        NumberedTextEncoding encoding, std::uint32_t font, std::uint32_t format,
        const std::array<float, 4>& rect, float alpha, const std::array<float, 4>& clip) override;
    std::array<float, 4> IntersectRect(const std::array<float, 4>& rect,
        const std::array<float, 4>& clip) override;
    std::string ConvertToUtf8(std::string_view text) override;
    std::uint32_t GetIconImage(std::uint32_t slot, std::uint32_t category, std::uint32_t variant) override;
    std::string FormatItemCount(std::uint32_t trigger, std::uint32_t count) override;
    std::vector<NumberedGameRecord> GetGameRecords() override;
    bool TriggerObject(std::uint32_t id) override;
    void TriggerPlayer(std::uint32_t id) override;
private:
    HMODULE module_;
    INumberedListRenderDataRuntime& data_;
    // Original DLL colour is24 bytes; four corners make a96-byte ColourRect.
    alignas(4) std::array<std::byte, 96> white_{};
    alignas(4) std::array<std::byte, 96> black_{};
    alignas(4) std::array<std::byte, 96> modulated_{};
};

}  // namespace pal4
