#include "pal4/checked_iterator_access.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>

namespace {

class Runtime final : public pal4::ICheckedIteratorRuntime {
public:
    unsigned calls = 0u;
    std::uint32_t ReadContainer(std::uint32_t proxy) override {
        if (proxy != 0x1234u)
            throw std::runtime_error("iterator proxy address");
        ++calls;
        return 0x5678u;
    }
};

void Cases() {
    Runtime runtime;
    if (pal4::CheckedIteratorAccess::Container({0u}, runtime) != 0u ||
        runtime.calls != 0u)
        throw std::runtime_error("null proxy returns null without read");
    if (pal4::CheckedIteratorAccess::Container({0x1234u}, runtime) !=
            0x5678u || runtime.calls != 1u)
        throw std::runtime_error("non-null proxy returns container pointer");
}

}  // namespace

int main() {
    try {
        Cases();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
