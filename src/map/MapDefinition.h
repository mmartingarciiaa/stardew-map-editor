#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace stardew::maps {

enum class AreaType {
    Farm,
    Village,
    DungeonFloor,
};

enum class ElementType {
    Cabin,
    Market,
    CommunityCenter,
    DungeonEntrance,
    MerchantMarnie,
    MerchantClint,
    MerchantPierre,
    MerchantRobin,
    Rock,
    OreNode,
    Chest,
    Decoration,
    Door,
    Stairs,
    SpawnPoint,
};

enum class ElementLayer {
    Structure,
    Object,
    Npc,
};

enum class TileType {
    Grass,
    Dirt,
    TilledSoil,
    Water,
    Path,
    Stone,
    CaveFloor,
    CaveWall,
    WoodFloor,
    Sand,
};

inline std::optional<TileType> tileTypeFromString(std::string_view value) {
    if (value == "grass") return TileType::Grass;
    if (value == "dirt") return TileType::Dirt;
    if (value == "tilled_soil") return TileType::TilledSoil;
    if (value == "water") return TileType::Water;
    if (value == "path") return TileType::Path;
    if (value == "stone") return TileType::Stone;
    if (value == "cave_floor") return TileType::CaveFloor;
    if (value == "cave_wall") return TileType::CaveWall;
    if (value == "wood_floor") return TileType::WoodFloor;
    if (value == "sand") return TileType::Sand;
    return std::nullopt;
}

inline std::string_view toString(TileType type) {
    switch (type) {
        case TileType::Grass: return "grass";
        case TileType::Dirt: return "dirt";
        case TileType::TilledSoil: return "tilled_soil";
        case TileType::Water: return "water";
        case TileType::Path: return "path";
        case TileType::Stone: return "stone";
        case TileType::CaveFloor: return "cave_floor";
        case TileType::CaveWall: return "cave_wall";
        case TileType::WoodFloor: return "wood_floor";
        case TileType::Sand: return "sand";
    }
    return "unknown_tile";
}

inline std::optional<AreaType> areaTypeFromString(std::string_view value) {
    if (value == "farm") return AreaType::Farm;
    if (value == "village") return AreaType::Village;
    if (value == "dungeon_floor") return AreaType::DungeonFloor;
    return std::nullopt;
}

inline std::string_view toString(AreaType type) {
    switch (type) {
        case AreaType::Farm: return "farm";
        case AreaType::Village: return "village";
        case AreaType::DungeonFloor: return "dungeon_floor";
    }
    return "unknown_area";
}

inline std::optional<ElementType> elementTypeFromString(std::string_view value) {
    if (value == "cabin") return ElementType::Cabin;
    if (value == "market") return ElementType::Market;
    if (value == "community_center") return ElementType::CommunityCenter;
    if (value == "dungeon_entrance") return ElementType::DungeonEntrance;
    if (value == "merchant_marnie") return ElementType::MerchantMarnie;
    if (value == "merchant_clint") return ElementType::MerchantClint;
    if (value == "merchant_pierre") return ElementType::MerchantPierre;
    if (value == "merchant_robin") return ElementType::MerchantRobin;
    if (value == "rock") return ElementType::Rock;
    if (value == "ore_node") return ElementType::OreNode;
    if (value == "chest") return ElementType::Chest;
    if (value == "decoration") return ElementType::Decoration;
    if (value == "door") return ElementType::Door;
    if (value == "stairs") return ElementType::Stairs;
    if (value == "spawn_point") return ElementType::SpawnPoint;
    return std::nullopt;
}

inline std::string_view toString(ElementType type) {
    switch (type) {
        case ElementType::Cabin: return "cabin";
        case ElementType::Market: return "market";
        case ElementType::CommunityCenter: return "community_center";
        case ElementType::DungeonEntrance: return "dungeon_entrance";
        case ElementType::MerchantMarnie: return "merchant_marnie";
        case ElementType::MerchantClint: return "merchant_clint";
        case ElementType::MerchantPierre: return "merchant_pierre";
        case ElementType::MerchantRobin: return "merchant_robin";
        case ElementType::Rock: return "rock";
        case ElementType::OreNode: return "ore_node";
        case ElementType::Chest: return "chest";
        case ElementType::Decoration: return "decoration";
        case ElementType::Door: return "door";
        case ElementType::Stairs: return "stairs";
        case ElementType::SpawnPoint: return "spawn_point";
    }
    return "unknown_element";
}

inline std::optional<ElementLayer> elementLayerFromString(std::string_view value) {
    if (value == "structure") return ElementLayer::Structure;
    if (value == "object") return ElementLayer::Object;
    if (value == "npc") return ElementLayer::Npc;
    return std::nullopt;
}

inline std::string_view toString(ElementLayer layer) {
    switch (layer) {
        case ElementLayer::Structure: return "structure";
        case ElementLayer::Object: return "object";
        case ElementLayer::Npc: return "npc";
    }
    return "unknown_layer";
}

struct MapArea {
    std::string id;
    std::optional<AreaType> type;
    int width = 0;
    int height = 0;
    std::optional<int> floorNumber;
    std::vector<std::vector<TileType>> tiles;

    bool operator==(const MapArea&) const = default;
};

struct MapElement {
    std::string id;
    std::string areaId;
    std::optional<ElementType> type;
    std::optional<ElementLayer> layer;
    int x = 0;
    int y = 0;
    int width = 1;
    int height = 1;

    bool operator==(const MapElement&) const = default;
};

struct MapTransition {
    // Una transicion dirigida conecta un elemento activador con una celda de otra area.
    std::string id;
    std::string fromAreaId;
    std::string triggerElementId;
    std::string toAreaId;
    int targetX;
    int targetY;

    bool operator==(const MapTransition&) const = default;
};

struct MapDefinition {
    std::optional<int> formatVersion;
    std::string name;
    std::vector<MapArea> areas;
    std::vector<MapElement> elements;
    std::vector<MapTransition> transitions;

    bool operator==(const MapDefinition&) const = default;
};

}  // namespace stardew::maps