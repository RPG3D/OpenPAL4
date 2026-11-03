#pragma once

#include "pal4/host_server_data_codec.h"
#include "pal4/native_game_save_archive_buffer.h"

#include <cstdint>
#include <string_view>

namespace pal4 {

// Writes one live player slot's host-server collections to the save stream.
class NativeHostServerSaveRuntime final : public IHostServerStreamRuntime {
public:
    static constexpr std::uint32_t kArchiveHandle = 1u;

    explicit NativeHostServerSaveRuntime(NativeGameSaveArchiveBuffer& archive)
        : archive_(archive) {}

    [[nodiscard]] bool Serialize(std::uint32_t player_slot);

    void AssignMarker(ConfigStringState& marker,
                      std::string_view text) override;
    [[nodiscard]] std::uint32_t EmptyStringAddress() override;
    [[nodiscard]] std::uint8_t WriteCount(std::uint32_t stream,
                                          std::uint32_t value) override;
    [[nodiscard]] std::uint8_t WriteBytes(std::uint32_t stream,
                                          std::uint32_t address,
                                          std::uint32_t size) override;
    [[nodiscard]] std::uint8_t ReadByte(std::uint32_t address) override;
    void WriteByte(std::uint32_t address, std::uint8_t value) override;
    void Free(std::uint32_t address) override;

private:
    NativeGameSaveArchiveBuffer& archive_;
};

}  // namespace pal4
