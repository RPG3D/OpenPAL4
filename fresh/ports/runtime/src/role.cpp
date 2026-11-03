#include "pal4/portable/role.h"
#include <bit>
#include <cmath>
#include <stdexcept>
#include <ostream>
#include <unordered_set>

namespace pal4::portable {
namespace {
void Require(bool value, const char* message) { if (!value) throw std::runtime_error(message); }
std::uint32_t Word(std::istream& stream) {
    unsigned char b[4]{};
    Require(bool(stream.read(reinterpret_cast<char*>(b), 4)), "truncated Role resource");
    return b[0] | (std::uint32_t(b[1]) << 8) | (std::uint32_t(b[2]) << 16) | (std::uint32_t(b[3]) << 24);
}
std::string Text(std::istream& stream, std::uint32_t limit) {
    const auto size = Word(stream);
    Require(size <= limit, "Role text exceeds limit");
    std::string value(size, '\0');
    Require(bool(stream.read(value.data(), size)), "truncated Role text");
    Require(value.find('\0') == std::string::npos, "embedded NUL in Role text");
    // 严格 UTF-8：拒绝截断、过长编码、代理码点和超出 Unicode 范围的值。
    for (std::size_t i = 0; i < value.size();) {
        const auto c = static_cast<unsigned char>(value[i++]);
        if (c < 0x80) continue;
        unsigned count{}; std::uint32_t point{}, minimum{};
        if (c >= 0xc2 && c <= 0xdf) { count=1; point=c & 31; minimum=0x80; }
        else if (c >= 0xe0 && c <= 0xef) { count=2; point=c & 15; minimum=0x800; }
        else if (c >= 0xf0 && c <= 0xf4) { count=3; point=c & 7; minimum=0x10000; }
        else throw std::runtime_error("invalid Role UTF-8");
        Require(value.size() - i >= count, "truncated Role UTF-8");
        while (count--) {
            const auto next=static_cast<unsigned char>(value[i++]);
            Require((next & 0xc0) == 0x80, "invalid Role UTF-8 continuation");
            point=(point << 6) | (next & 63);
        }
        Require(point >= minimum && point <= 0x10ffff && (point < 0xd800 || point > 0xdfff), "invalid Role Unicode point");
    }
    return value;
}
}
const RoleValue& RoleRecord::Find(std::string_view name) const {
    for (const auto& field : fields_) if (field.name == name) return field.value;
    throw std::runtime_error("missing Role field " + std::string(name));
}
std::uint32_t RoleRecord::Number(std::string_view name) const { return std::get<std::uint32_t>(Find(name)); }
const std::string& RoleRecord::Text(std::string_view name) const { return std::get<std::string>(Find(name)); }
float RoleRecord::Real(std::string_view name) const { return std::get<float>(Find(name)); }
RoleCatalog ReadRoles(std::istream& stream) {
    Require(Word(stream) == 0x4c523450 && Word(stream) == 1, "unsupported Role resource");
    Require(Word(stream) == 4 && Word(stream) == 50, "invalid Role counts");
    RoleCatalog roles;
    for (std::uint32_t index=0; index<roles.size(); ++index) {
        Require(Word(stream) == index, "invalid Role player index");
        auto& role=roles[index]; std::unordered_set<std::string> names;
        for (std::size_t i=0; i<role.fields_.size(); ++i) {
            auto& field=role.fields_[i]; field.name=Text(stream, 64); field.type=Word(stream);
            Require(!field.name.empty() && field.name.find_first_not_of("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_") == std::string::npos,
                    "invalid Role field name");
            Require(names.insert(field.name).second, "duplicate Role field");
            if (index) Require(field.name == roles[0].fields_[i].name && field.type == roles[0].fields_[i].type, "Role schema mismatch");
            switch (field.type) {
            case 1: field.value=Word(stream); break;
            case 2: field.value=Text(stream, 4096); break;
            case 3: {
                const float value=std::bit_cast<float>(Word(stream));
                Require(std::isfinite(value), "non-finite Role value"); field.value=value; break;
            }
            default: throw std::runtime_error("unknown Role field type");
            }
        }
        Require(role.Number("ROLE_ID") == 1001 + index, "invalid Role ID");
        for (const auto* name : {"RA_NAME", "RA_ICON", "RA_MODEL", "RA_TEXTURE", "SOUND_WOUNDED1", "SOUND_WOUNDED2", "SOUND_WOUNDED3"})
            (void)role.Text(name);
        const auto& model=role.Text("RA_MODEL");
        Require(!model.empty() && model.find_first_not_of("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_-") == std::string::npos,
                "invalid Role model resource name");
    }
    Require(stream.peek() == std::char_traits<char>::eof() && !stream.bad(), "trailing Role resource bytes");
    return roles;
}
void WriteRoleReport(std::ostream& stream, const RoleCatalog& roles) {
    stream << "{\"pointer_bits\":" << sizeof(void*) * 8 << ",\"gameplay_initialized\":false,\"roles\":[";
    constexpr char hex[]="0123456789abcdef";
    for (std::size_t index=0; index<roles.size(); ++index) {
        if (index) stream << ',';
        stream << "{\"player_index\":" << index << ",\"fields\":[";
        bool first=true;
        for (const auto& field : roles[index].Fields()) {
            if (!first) stream << ',';
            first=false;
            stream << "{\"name\":\"" << field.name << "\",\"type\":" << field.type;
            if (field.type == 2) {
                stream << ",\"text_hex\":\"";
                for (unsigned char c : std::get<std::string>(field.value)) stream << hex[c >> 4] << hex[c & 15];
                stream << '"';
            } else if (field.type == 3) stream << ",\"float_bits\":" << std::bit_cast<std::uint32_t>(std::get<float>(field.value));
            else stream << ",\"value\":" << std::get<std::uint32_t>(field.value);
            stream << '}';
        }
        stream << "]}";
    }
    stream << "]}";
    Require(bool(stream), "cannot write Role report");
}
} // namespace pal4::portable
