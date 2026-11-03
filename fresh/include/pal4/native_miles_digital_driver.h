#pragma once

#include <string>

namespace pal4 {

class NativeMilesDigitalDriver final {
public:
    explicit NativeMilesDigitalDriver(const std::string& original_root);
    ~NativeMilesDigitalDriver();
    NativeMilesDigitalDriver(const NativeMilesDigitalDriver&) = delete;
    NativeMilesDigitalDriver& operator=(const NativeMilesDigitalDriver&) = delete;

    [[nodiscard]] bool Open();
    void SetMasterVolume(float level);
    void Close();
    [[nodiscard]] void* Handle() const { return driver_; }
    [[nodiscard]] void* Library() const { return library_; }
    [[nodiscard]] std::string LastError() const;

private:
    std::string redist_directory_;
    void* library_{};
    void* driver_{};
    void* set_redist_directory_{};
    void* startup_{};
    void* shutdown_{};
    void* open_driver_{};
    void* close_driver_{};
    void* set_master_volume_{};
    void* last_error_{};
    bool started_{};
};

}  // namespace pal4
