#pragma once

#include <string>
#include <vector>

#include "map/MapDefinition.h"
#include "map/MapRequirements.h"
#include "map/MapValidationIssue.h"
#include "map/persistence/MapRepository.h"

namespace stardew::maps {

struct MapLoadResult {
    MapDefinition map;
    std::vector<MapValidationIssue> issues;
};

// Coordina persistencia y validacion sin depender de Qt ni del formato TOML.
class MapService {
public:
    MapService(const MapRepository& repository, MapRequirements requirements);

    MapLoadResult loadMap(const std::string& path) const;
    std::vector<MapValidationIssue> saveMap(
        const std::string& path,
        const MapDefinition& map) const;

private:
    const MapRepository& repository_;
    MapRequirements requirements_;
};

}  // namespace stardew::maps