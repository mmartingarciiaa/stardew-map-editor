#include "map/persistence/MapRepository.h"

#include "map/persistence/TomlMapFile.h"

namespace stardew::maps {

MapDefinition TomlMapRepository::loadMap(const std::string& path) const {
    return loadTomlMapDefinition(path);
}

void TomlMapRepository::saveMap(const std::string& path, const MapDefinition& map) const {
    saveTomlMapDefinition(path, map);
}

}  // namespace stardew::maps