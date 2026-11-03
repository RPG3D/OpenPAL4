#pragma once
#include <cstddef>
#include <cstdint>
#include <functional>
#include <map>
#include <string>
#include <string_view>
#include <vector>

namespace pal4::portable {
class ScriptModuleCache {
public:
    using Bytes = std::vector<std::uint8_t>;
    using Reader = std::function<Bytes(std::string_view)>;

    const Bytes& Load(std::string_view module, const Reader& reader);
    const Bytes* Find(std::string_view module) const;
    void RetainOnly(std::string_view module);
    void Clear();
    std::size_t Size() const;

private:
    std::map<std::string,Bytes> modules_;
};
} // namespace pal4::portable
