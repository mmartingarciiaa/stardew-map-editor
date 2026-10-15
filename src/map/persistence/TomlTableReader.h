#pragma once

#include <cstdint>
#include <limits>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

#include <toml++/toml.hpp>

namespace stardew::maps::detail {

// Agrupa conversiones comunes de una tabla TOML y adjunta contexto a sus errores.
class TomlTableReader {
public:
    TomlTableReader(const toml::table& table, std::string context)
        : table_(table), context_(std::move(context)) {}

    template <typename Value>
    std::optional<Value> optionalValue(std::string_view key) const {
        const auto* node = table_[key].node();
        if (node == nullptr) {
            return std::nullopt;
        }
        const auto value = node->template value<Value>();
        if (!value.has_value()) {
            throw std::runtime_error(
                context_ + " has an invalid '" + std::string(key) + "' value");
        }
        return *value;
    }

    template <typename Value>
    Value requiredValue(std::string_view key) const {
        const auto value = optionalValue<Value>(key);
        if (!value.has_value()) {
            throw std::runtime_error(
                context_ + " is missing required value '" + std::string(key) + "'");
        }
        return *value;
    }

    int requiredInt(std::string_view key) const {
        const auto value = requiredValue<std::int64_t>(key);
        checkIntRange(value, key);
        return static_cast<int>(value);
    }

    std::optional<int> optionalInt(std::string_view key) const {
        const auto value = optionalValue<std::int64_t>(key);
        if (!value.has_value()) {
            return std::nullopt;
        }
        checkIntRange(*value, key);
        return static_cast<int>(*value);
    }

    template <typename Enum, typename Convert>
    std::optional<Enum> optionalEnum(std::string_view key, Convert convert) const {
        const auto name = optionalValue<std::string>(key);
        if (!name.has_value()) {
            return std::nullopt;
        }
        const auto value = convert(*name);
        if (!value.has_value()) {
            throw std::runtime_error(
                context_ + " has unknown '" + std::string(key) + "': " + *name);
        }
        return value;
    }

    template <typename Enum, typename Convert>
    Enum requiredEnum(std::string_view key, Convert convert) const {
        const auto name = requiredValue<std::string>(key);
        const auto value = convert(name);
        if (!value.has_value()) {
            throw std::runtime_error(
                context_ + " has unknown '" + std::string(key) + "': " + name);
        }
        return *value;
    }

    const toml::array* optionalArray(std::string_view key) const {
        const auto* node = table_[key].node();
        if (node == nullptr) {
            return nullptr;
        }
        const auto* array = node->as_array();
        if (array == nullptr) {
            throw std::runtime_error(
                context_ + " value '" + std::string(key) + "' must be an array");
        }
        return array;
    }

    const toml::array* requiredArray(std::string_view key) const {
        const auto* array = optionalArray(key);
        if (array == nullptr) {
            throw std::runtime_error(
                context_ + " is missing array '" + std::string(key) + "'");
        }
        return array;
    }

private:
    void checkIntRange(std::int64_t value, std::string_view key) const {
        if (value < std::numeric_limits<int>::min() || value > std::numeric_limits<int>::max()) {
            throw std::runtime_error(
                context_ + " has an out-of-range '" + std::string(key) + "'");
        }
    }

    const toml::table& table_;
    std::string context_;
};

// Comprueba que una entrada de array sea una tabla TOML.
inline const toml::table& requiredTable(const toml::node& node, std::string_view context) {
    const auto* table = node.as_table();
    if (table == nullptr) {
        throw std::runtime_error(std::string(context) + " must be a table");
    }
    return *table;
}

}  // namespace stardew::maps::detail