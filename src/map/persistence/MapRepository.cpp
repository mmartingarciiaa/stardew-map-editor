#include "map/persistence/MapRepository.h"

#include "map/persistence/MapFile.h"

namespace stardew::maps {

MapDefinition TomlMapRepository::loadMap(const std::string& path) const {
    return loadMapDefinition(path);
}

void TomlMapRepository::saveMap(const std::string& path, const MapDefinition& map) const {
    saveMapDefinition(path, map);
}

}  // namespace stardew::maps