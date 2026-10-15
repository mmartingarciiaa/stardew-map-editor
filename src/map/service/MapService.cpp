#include "map/service/MapService.h"

#include <utility>

#include "map/validation/MapValidator.h"

namespace stardew::maps {

MapService::MapService(
    const MapRepository& repository,
    MapRequirements requirements)
    : repository_(repository), requirements_(std::move(requirements)) {}

MapLoadResult MapService::loadMap(const std::string& path) const {
    // Se conserva el mapa aunque tenga errores semanticos para permitir corregirlo en el editor.
    auto map = repository_.loadMap(path);
    auto issues = validateMap(map, requirements_);
    return {std::move(map), std::move(issues)};
}

std::vector<MapValidationIssue> MapService::saveMap(
    const std::string& path,
    const MapDefinition& map) const {
    auto issues = validateMap(map, requirements_);
    if (!issues.empty()) {
        return issues;
    }

    repository_.saveMap(path, map);
    return {};
}

}  // namespace stardew::maps