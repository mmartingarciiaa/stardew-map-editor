#pragma once

#include <algorithm>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "map/MapDefinition.h"

namespace stardew::maps {

// Estado de edicion del mundo.
// El historial vive por area para evitar mezclar cambios de granja, pueblo y mazmorra.
// Una accion de edicion se guarda como snapshot del MapArea actual; la UI no se apila,
// solo el modelo del mundo que se esta editando.
struct MapEditorState {
    MapDefinition map;
    std::string activeAreaId;
    TileType selectedTile = TileType::Grass;
    std::optional<ElementType> selectedElement;
    bool dirty = false;

    std::unordered_map<std::string, std::vector<MapArea>> undoHistory;
    std::unordered_map<std::string, std::vector<MapArea>> redoHistory;
    std::optional<std::string> pendingAreaId;

    // Cambia el area que se muestra y se edita en la grilla actual.
    void setActiveArea(const std::string& areaId);
    // Crea un mundo inicial con un conjunto de areas tipicas del juego para empezar a editar.
    void makeDefaultWorld();
    void resizeActiveArea(int width, int height);
    // Comienza una accion de edicion: guarda el snapshot del area activa antes del cambio.
    void beginAreaAction();
    // Finaliza la accion de edicion y marca que el estado quedo modificado.
    void endAreaAction();

    bool hasArea(const std::string& areaId) const;
    MapArea* activeArea();
    const MapArea* activeArea() const;

    void applyTilePaint(int x, int y, TileType tile);
    void undoActiveArea();
    void redoActiveArea();

private:
    void pushSnapshot(std::unordered_map<std::string, std::vector<MapArea>>& history, const std::string& areaId);
    void restoreSnapshot(const std::string& areaId, const MapArea& snapshot);
};

inline void MapEditorState::setActiveArea(const std::string& areaId) {
    activeAreaId = areaId;
}

inline void MapEditorState::makeDefaultWorld() {
    map.name = "Nuevo mundo";
    map.areas.clear();
    map.elements.clear();
    map.transitions.clear();

    MapArea farm;
    farm.id = "farm";
    farm.type = AreaType::Farm;
    farm.width = 12;
    farm.height = 12;
    farm.tiles = std::vector<std::vector<TileType>>(
        static_cast<std::size_t>(farm.height),
        std::vector<TileType>(static_cast<std::size_t>(farm.width), TileType::Grass));
    map.areas.push_back(farm);

    MapArea village;
    village.id = "village";
    village.type = AreaType::Village;
    village.width = 12;
    village.height = 12;
    village.tiles = std::vector<std::vector<TileType>>(
        static_cast<std::size_t>(village.height),
        std::vector<TileType>(static_cast<std::size_t>(village.width), TileType::Grass));
    map.areas.push_back(village);

    MapArea dungeon;
    dungeon.id = "dungeon";
    dungeon.type = AreaType::DungeonFloor;
    dungeon.width = 12;
    dungeon.height = 12;
    dungeon.floorNumber = 1;
    dungeon.tiles = std::vector<std::vector<TileType>>(
        static_cast<std::size_t>(dungeon.height),
        std::vector<TileType>(static_cast<std::size_t>(dungeon.width), TileType::CaveFloor));
    map.areas.push_back(dungeon);

    activeAreaId = "farm";
    dirty = false;
}

inline void MapEditorState::resizeActiveArea(int width, int height) {
    auto* area = activeArea();
    if (area == nullptr || width < 1 || height < 1 ||
        (area->width == width && area->height == height)) {
        return;
    }

    beginAreaAction();
    const auto defaultTile = area->type == AreaType::DungeonFloor
        ? TileType::CaveFloor
        : TileType::Grass;
    std::vector<std::vector<TileType>> resizedTiles(
        static_cast<std::size_t>(height),
        std::vector<TileType>(static_cast<std::size_t>(width), defaultTile));
    const auto rowsToCopy = std::min(height, area->height);
    const auto columnsToCopy = std::min(width, area->width);
    for (int row = 0; row < rowsToCopy; ++row) {
        for (int column = 0; column < columnsToCopy; ++column) {
            resizedTiles[static_cast<std::size_t>(row)][static_cast<std::size_t>(column)] =
                area->tiles[static_cast<std::size_t>(row)][static_cast<std::size_t>(column)];
        }
    }
    area->width = width;
    area->height = height;
    area->tiles = std::move(resizedTiles);
    endAreaAction();
}

inline bool MapEditorState::hasArea(const std::string& areaId) const {
    for (const auto& area : map.areas) {
        if (area.id == areaId) {
            return true;
        }
    }
    return false;
}

inline MapArea* MapEditorState::activeArea() {
    for (auto& area : map.areas) {
        if (area.id == activeAreaId) {
            return &area;
        }
    }
    return nullptr;
}

inline const MapArea* MapEditorState::activeArea() const {
    for (const auto& area : map.areas) {
        if (area.id == activeAreaId) {
            return &area;
        }
    }
    return nullptr;
}

inline void MapEditorState::pushSnapshot(
    std::unordered_map<std::string, std::vector<MapArea>>& history,
    const std::string& areaId) {
    auto it = history.find(areaId);
    if (it == history.end()) {
        history[areaId] = {};
    }

    for (const auto& area : map.areas) {
        if (area.id == areaId) {
            history[areaId].push_back(area);
            return;
        }
    }
}

inline void MapEditorState::restoreSnapshot(const std::string& areaId, const MapArea& snapshot) {
    for (auto& area : map.areas) {
        if (area.id == areaId) {
            area = snapshot;
            return;
        }
    }
}

inline void MapEditorState::beginAreaAction() {
    if (activeAreaId.empty()) {
        return;
    }
    // La accion comienza con una foto del estado actual del area activa.
    // Eso permite deshacer la operacion completa, no celda por celda.
    pendingAreaId = activeAreaId;
    pushSnapshot(undoHistory, activeAreaId);
    redoHistory.erase(activeAreaId);
}

inline void MapEditorState::endAreaAction() {
    if (!pendingAreaId.has_value()) {
        return;
    }
    dirty = true;
    pendingAreaId.reset();
}

inline void MapEditorState::applyTilePaint(int x, int y, TileType tile) {
    if (activeAreaId.empty()) {
        return;
    }
    auto* area = activeArea();
    if (area == nullptr) {
        return;
    }
    if (x < 0 || y < 0 || y >= static_cast<int>(area->tiles.size()) ||
        x >= static_cast<int>(area->tiles[static_cast<std::size_t>(y)].size())) {
        return;
    }

    // El "brush" modifica la grilla del area activa. Si no se inicio una accion, se crea
    // una snapshot previa para poder deshacer toda la operacion de una vez.
    if (!pendingAreaId.has_value()) {
        beginAreaAction();
    }
    area->tiles[static_cast<std::size_t>(y)][static_cast<std::size_t>(x)] = tile;
    dirty = true;
}

inline void MapEditorState::undoActiveArea() {
    if (activeAreaId.empty()) {
        return;
    }
    auto undoIt = undoHistory.find(activeAreaId);
    if (undoIt == undoHistory.end() || undoIt->second.empty()) {
        return;
    }

    // Deshacer la ultima accion del area activa significa devolver ese snapshot completo.
    const auto snapshot = undoIt->second.back();
    undoIt->second.pop_back();
    pushSnapshot(redoHistory, activeAreaId);
    restoreSnapshot(activeAreaId, snapshot);
    dirty = true;
}

inline void MapEditorState::redoActiveArea() {
    if (activeAreaId.empty()) {
        return;
    }
    auto redoIt = redoHistory.find(activeAreaId);
    if (redoIt == redoHistory.end() || redoIt->second.empty()) {
        return;
    }

    const auto snapshot = redoIt->second.back();
    redoIt->second.pop_back();
    pushSnapshot(undoHistory, activeAreaId);
    restoreSnapshot(activeAreaId, snapshot);
    dirty = true;
}

}  // namespace stardew::maps
