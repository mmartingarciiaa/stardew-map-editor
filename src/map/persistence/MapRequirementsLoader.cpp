#include "map/persistence/MapRequirementsLoader.h"

#include <cstdint>
#include <limits>
#include <set>
#include <stdexcept>
#include <string>
#include <tuple>
#include <utility>

#include "map/persistence/TomlTableReader.h"

#include <toml++/toml.hpp>

namespace stardew::maps {
namespace {

/// Lee un numero de piso opcional y comprueba que sea positivo y representable como int.
std::optional<int> optionalFloor(const detail::TomlTableReader& table, std::string_view key) {
    const auto value = table.optionalInt(key);
    if (value.has_value() && *value < 1) {
        throw std::runtime_error("transition floor must be a positive integer");
    }
    return value;
}

}  // namespace

/// Carga y valida las reglas del mapa desde un archivo TOML.
/// @param path Ruta del archivo TOML de requisitos.
/// @return Reglas tipadas que puede consumir el validador de mapas.
/// @throws std::runtime_error Si el archivo no se puede parsear o contiene reglas incompletas,
///                            desconocidas o contradictorias.
MapRequirements loadMapRequirements(const std::string& path) {
    // Parsear una sola vez permite reutilizar el documento para leer todas sus reglas.
    toml::table document;
    try {
        document = toml::parse_file(path);
    } catch (const toml::parse_error& error) {
        throw std::runtime_error("cannot parse map requirements '" + path + "': " +
                                 std::string(error.description()));
    }

    // Lee la version y las opciones generales antes de procesar las listas de requisitos.
    const detail::TomlTableReader reader(document, "requirements");
    MapRequirements requirements{
        reader.requiredInt("format_version"),
        reader.requiredValue<bool>("allow_structure_overlap"),
        reader.requiredValue<bool>("require_outgoing_transition_for_each_area"),
        {},
        {},
        {},
    };
    if (requirements.formatVersion <= 0) {
        throw std::runtime_error("requirements format_version must be positive");
    }

    // Carga las cantidades requeridas por tipo de area y sus numeros de piso, si corresponden.
    std::set<AreaType> areaTypes;
    const auto* requiredAreas = reader.requiredArray("required_areas");
    for (const auto& node : *requiredAreas) {
        const auto& table = detail::requiredTable(node, "each required_areas entry");
        const detail::TomlTableReader areaReader(table, "required area");
        const auto type = areaReader.requiredEnum<AreaType>("type", areaTypeFromString);
        const int count = areaReader.requiredInt("count");
        if (count < 0 || !areaTypes.insert(type).second) {
            throw std::runtime_error("required area count is negative or its type is duplicated");
        }

        // Los pisos deben ser positivos, unicos y coincidir con la cantidad declarada.
        std::vector<int> floorNumbers;
        if (const auto* floors = areaReader.optionalArray("floor_numbers")) {
            std::set<int> uniqueFloors;
            for (const auto& floorNode : *floors) {
                const auto floor = floorNode.value<std::int64_t>();
                if (!floor.has_value() || *floor < 1 || *floor > std::numeric_limits<int>::max() ||
                    !uniqueFloors.insert(static_cast<int>(*floor)).second) {
                    throw std::runtime_error("required area has an invalid or duplicate floor number");
                }
                floorNumbers.push_back(static_cast<int>(*floor));
            }
            if (!floorNumbers.empty() && floorNumbers.size() != static_cast<std::size_t>(count)) {
                throw std::runtime_error("required floor_numbers count does not match area count");
            }
        }

        requirements.areas.push_back({type, count, std::move(floorNumbers)});
    }

    // Convierte las entidades requeridas a enums y rechaza reglas duplicadas o desconocidas.
    std::set<std::tuple<AreaType, ElementType, ElementLayer>> elementRequirements;
    const auto* requiredElements = reader.requiredArray("required_elements");
    for (const auto& node : *requiredElements) {
        const auto& table = detail::requiredTable(node, "each required_elements entry");
        const detail::TomlTableReader elementReader(table, "required element");
        const auto areaType = elementReader.requiredEnum<AreaType>("area_type", areaTypeFromString);
        const auto type = elementReader.requiredEnum<ElementType>("type", elementTypeFromString);
        const auto layer = elementReader.requiredEnum<ElementLayer>("layer", elementLayerFromString);
        if (!elementRequirements.emplace(areaType, type, layer).second) {
            throw std::runtime_error("duplicate required element rule");
        }
        requirements.elements.push_back({areaType, type, layer});
    }

    // Carga las conexiones que cada tipo de area o piso debe ofrecer.
    using TransitionKey = std::tuple<AreaType, std::optional<int>, AreaType, std::optional<int>>;
    std::set<TransitionKey> transitionRequirements;
    const auto* requiredTransitions = reader.requiredArray("required_transitions");
    for (const auto& node : *requiredTransitions) {
        const auto& table = detail::requiredTable(node, "each required_transitions entry");
        const detail::TomlTableReader transitionReader(table, "required transition");
        const auto fromType = transitionReader.requiredEnum<AreaType>(
            "from_area_type", areaTypeFromString);
        const auto fromFloor = optionalFloor(transitionReader, "from_floor");
        const auto toType = transitionReader.requiredEnum<AreaType>(
            "to_area_type", areaTypeFromString);
        const auto toFloor = optionalFloor(transitionReader, "to_floor");
        if ((fromFloor.has_value() && fromType != AreaType::DungeonFloor) ||
            (toFloor.has_value() && toType != AreaType::DungeonFloor)) {
            throw std::runtime_error("only dungeon transition endpoints can specify a floor");
        }
        if (!transitionRequirements.emplace(fromType, fromFloor, toType, toFloor).second) {
            throw std::runtime_error("duplicate required transition rule");
        }
        requirements.transitions.push_back({fromType, fromFloor, toType, toFloor});
    }

    return requirements;
}

}  // namespace stardew::maps