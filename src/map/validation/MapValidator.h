#pragma once

#include <string>
#include <vector>

#include "map/MapDefinition.h"
#include "map/MapRequirements.h"
#include "map/MapValidationIssue.h"

namespace stardew::maps {

std::vector<MapValidationIssue> validateMap(
	const MapDefinition& map,
	const MapRequirements& requirements);

}  // namespace stardew::maps