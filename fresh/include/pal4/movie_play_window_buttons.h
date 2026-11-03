#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>

#include "pal4/movie_play_window_lifecycle.h"

namespace pal4 {

class IMoviePlayWindowButtonsRuntime {
public:
    virtual ~IMoviePlayWindowButtonsRuntime() = default;
    [[nodiscard]] virtual std::uint32_t FindWindow(
        std::string_view name) = 0;
    [[nodiscard]] virtual std::uint32_t AllocateCallback(
        std::size_t bytes) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t SubscribeClicked(
        std::uint32_t window, std::uint32_t callback) = 0;
    [[nodiscard]] virtual std::uint32_t ReferenceCount(
        std::uint32_t reference) = 0;
    virtual void SetReferenceCount(std::uint32_t reference,
                                   std::uint32_t count) = 0;
    virtual void DeleteReference(std::uint32_t reference,
                                 std::uint32_t flags) = 0;
};

class MoviePlayWindowButtonsRuntimeStub :
    public IMoviePlayWindowButtonsRuntime {
public:
    [[nodiscard]] std::uint32_t FindWindow(
        std::string_view) override { return 0u; }
    [[nodiscard]] std::uint32_t AllocateCallback(
        std::size_t) override { return 0u; }
    void WriteWord(std::uint32_t, std::uint32_t) override {}
    [[nodiscard]] std::uint32_t SubscribeClicked(
        std::uint32_t, std::uint32_t) override { return 0u; }
    [[nodiscard]] std::uint32_t ReferenceCount(
        std::uint32_t) override { return 0u; }
    void SetReferenceCount(std::uint32_t, std::uint32_t) override {}
    void DeleteReference(std::uint32_t, std::uint32_t) override {}
};

class MoviePlayWindowButtons final {
public:
    static constexpr std::uint32_t kCallbackVtable = 0x843688u;

    static void Initialize(MoviePlayWindowState& state,
                           std::uint32_t object_address,
                           IMoviePlayWindowButtonsRuntime& runtime);
};

}  // namespace pal4
