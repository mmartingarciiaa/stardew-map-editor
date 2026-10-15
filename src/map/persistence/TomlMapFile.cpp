#include "map/persistence/TomlMapFile.h"

#include <cstdint>
#include <fstream>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "map/persistence/TomlTableReader.h"

#include <toml++/toml.hpp>

namespace stardew::maps {
namespace {

std::vector<std::vector<TileType>> readTiles(const toml::table& areaTable, const std::string& id) {
    std::vector<std::vector<TileType>> tiles;
    const detail::TomlTableReader areaReader(areaTable, "area '" + id + "'");
    const auto* rows = areaReader.optionalArray("tiles");
    if (rows == nullptr) {
        return tiles;
    }

    // Convierte cada celda al enum para que un ID de terreno desconocido no pase inadvertido.
    for (const auto& rowNode : *rows) {
        const auto* row = rowNode.as_array();
        if (row == nullptr) {
            throw std::runtime_error("tile rows in area '" + id + "' must be arrays");
        }
        std::vector<TileType> tileRow;
        for (const auto& tileNode : *row) {
            const auto name = tileNode.value<std::string>();
            if (!name.has_value()) {
                throw std::runtime_error("tile IDs in area '" + id + "' must be strings");
            }
            const auto tile = tileTypeFromString(*name);
            if (!tile.has_value()) {
                throw std::runtime_error("area '" + id + "' has unknown tile type: " + *name);
            }
            tileRow.push_back(*tile);
        }
        tiles.push_back(std::move(tileRow));
    }
    return tiles;
}

template <typename Enum>
void insertOptionalEnum(toml::table& table, std::string_view key, const std::optional<Enum>& value) {
    if (value.has_value()) {
        table.insert_or_assign(std::string(key), std::string(toString(*value)));
    }
}

void insertOptionalInt(toml::table& table, std::string_view key, const std::optional<int>& value) {
    if (value.has_value()) {
        table.insert_or_assign(std::string(key), *value);
    }
}

// Convierte una entrada TOML de area al modelo tipado.
MapArea readArea(const toml::node& node) {
    const auto& table = detail::requiredTable(node, "each map area");
    const detail::TomlTableReader reader(table, "area");
    MapArea area;
    area.id = reader.requiredValue<std::string>("id");
    const detail::TomlTableReader areaReader(table, "area '" + area.id + "'");
    area.type = areaReader.optionalEnum<AreaType>("type", areaTypeFromString);
    area.width = areaReader.requiredInt("width");
    area.height = areaReader.requiredInt("height");
    area.floorNumber = areaReader.optionalInt("floor");
    area.tiles = readTiles(table, area.id);
    return area;
}

// Convierte una entrada TOML de elemento al modelo tipado.
MapElement readElement(const toml::node& node) {
    const auto& table = detail::requiredTable(node, "each map element");
    const detail::TomlTableReader reader(table, "element");
    MapElement element;
    element.id = reader.requiredValue<std::string>("id");
    const detail::TomlTableReader elementReader(table, "element '" + element.id + "'");
    element.areaId = elementReader.requiredValue<std::string>("area");
    element.type = elementReader.optionalEnum<ElementType>("type", elementTypeFromString);
    element.layer = elementReader.optionalEnum<ElementLayer>("layer", elementLayerFromString);
    element.x = elementReader.requiredInt("x");
    element.y = elementReader.requiredInt("y");
    element.width = elementReader.requiredInt("width");
    element.height = elementReader.requiredInt("height");
    return element;
}

// Convierte una entrada TOML de transicion con destino y coordenada de llegada.
MapTransition readTransition(const toml::node& node) {
    const auto& table = detail::requiredTable(node, "each map transition");
    const detail::TomlTableReader reader(table, "transition");
    return {
        reader.requiredValue<std::string>("id"),
        reader.requiredValue<std::string>("from_area"),
        reader.requiredValue<std::string>("trigger_element"),
        reader.requiredValue<std::string>("to_area"),
        reader.requiredInt("target_x"),
        reader.requiredInt("target_y"),
    };
}

// Serializa el terreno y los datos propios de un area.
toml::table serializeArea(const MapArea& area) {
    toml::table table;
    table.insert_or_assign("id", area.id);
    insertOptionalEnum(table, "type", area.type);
    table.insert_or_assign("width", area.width);
    table.insert_or_assign("height", area.height);
    insertOptionalInt(table, "floor", area.floorNumber);

    toml::array rows;
    for (const auto& tileRow : area.tiles) {
        toml::array row;
        for (const auto tile : tileRow) {
            row.push_back(std::string(toString(tile)));
        }
        rows.push_back(std::move(row));
    }
    table.insert_or_assign("tiles", std::move(rows));
    return table;
}

// Serializa un elemento con su area, tipo, capa, posicion y huella.
toml::table serializeElement(const MapElement& element) {
    toml::table table;
    table.insert_or_assign("id", element.id);
    table.insert_or_assign("area", element.areaId);
    insertOptionalEnum(table, "type", element.type);
    insertOptionalEnum(table, "layer", element.layer);
    table.insert_or_assign("x", element.x);
    table.insert_or_assign("y", element.y);
    table.insert_or_assign("width", element.width);
    table.insert_or_assign("height", element.height);
    return table;
}

// Serializa la conexion dirigida entre dos areas.
toml::table serializeTransition(const MapTransition& transition) {
    toml::table table;
    table.insert_or_assign("id", transition.id);
    table.insert_or_assign("from_area", transition.fromAreaId);
    table.insert_or_assign("trigger_element", transition.triggerElementId);
    table.insert_or_assign("to_area", transition.toAreaId);
    table.insert_or_assign("target_x", transition.targetX);
    table.insert_or_assign("target_y", transition.targetY);
    return table;
}

}  // namespace

MapDefinition loadTomlMapDefinition(const std::string& path) {
    // Parsea el documento y convierte errores de sintaxis en diagnosticos con la ruta del mapa.
    toml::table document;
    try {
        document = toml::parse_file(path);
    } catch (const toml::parse_error& error) {
        throw std::runtime_error(
            "cannot parse map '" + path + "': " + std::string(error.description()));
    }

    // Los campos semanticos ausentes quedan vacios para que validateMap pueda reportarlos.
    MapDefinition map;
    const detail::TomlTableReader reader(document, "map");
    map.formatVersion = reader.optionalInt("format_version");
    map.name = reader.optionalValue<std::string>("name").value_or("");

    // Lee areas, dimensiones, piso opcional y grilla de terreno tipada.
    if (const auto* areas = reader.optionalArray("areas")) {
        for (const auto& node : *areas) {
            map.areas.push_back(readArea(node));
        }
    }

    // Lee elementos y conserva tipo/capa ausentes para que el validador los explique.
    if (const auto* elements = reader.optionalArray("elements")) {
        for (const auto& node : *elements) {
            map.elements.push_back(readElement(node));
        }
    }

    // Las rutas vinculan un elemento de salida con una celda del area de destino.
    if (const auto* transitions = reader.optionalArray("transitions")) {
        for (const auto& node : *transitions) {
            map.transitions.push_back(readTransition(node));
        }
    }

    return map;
}

void saveTomlMapDefinition(const std::string& path, const MapDefinition& map) {
    // Construye el TOML a partir del modelo; los enums se escriben con sus IDs estables.
    toml::table document;
    insertOptionalInt(document, "format_version", map.formatVersion);
    document.insert_or_assign("name", map.name);

    // Serializa cada tipo de registro con la funcion que conoce sus campos.
    toml::array areas;
    for (const auto& area : map.areas) {
        areas.push_back(serializeArea(area));
    }
    document.insert_or_assign("areas", std::move(areas));

    toml::array elements;
    for (const auto& element : map.elements) {
        elements.push_back(serializeElement(element));
    }
    document.insert_or_assign("elements", std::move(elements));

    toml::array transitions;
    for (const auto& transition : map.transitions) {
        transitions.push_back(serializeTransition(transition));
    }
    document.insert_or_assign("transitions", std::move(transitions));

    // Comprueba tanto la apertura como la escritura para no anunciar un guardado incompleto.
    std::ofstream output(path);
    if (!output) {
        throw std::runtime_error("cannot open map for writing: " + path);
    }
    output << document;
    if (!output) {
        throw std::runtime_error("failed while writing map: " + path);
    }
}

}  // namespace stardew::maps