#pragma once
#include <array>
#include <cstdint>
#include <istream>
#include <string>
#include <string_view>
#include <variant>

namespace pal4::portable {
// 原数据库字段类型；值持有自己的文本，不保存原 x86 对象地址。
using RoleValue = std::variant<std::uint32_t, std::string, float>;
struct RoleField { std::string name; std::uint32_t type; RoleValue value; };
class RoleRecord {
public:
    std::uint32_t Number(std::string_view name) const;
    const std::string& Text(std::string_view name) const;
    float Real(std::string_view name) const;
    const std::array<RoleField, 50>& Fields() const noexcept { return fields_; }
private:
    std::array<RoleField, 50> fields_{};
    const RoleValue& Find(std::string_view name) const;
    friend std::array<RoleRecord, 4> ReadRoles(std::istream&);
};
using RoleCatalog = std::array<RoleRecord, 4>;
// P4RL v1：完整四条 Role 记录，字段名、类型及值；不代表玩家已初始化。
RoleCatalog ReadRoles(std::istream& stream);
// 诊断使用：文本编码为 UTF-8 十六进制，浮点输出原始 uint32 位模式。
void WriteRoleReport(std::ostream& stream, const RoleCatalog& roles);
} // namespace pal4::portable
