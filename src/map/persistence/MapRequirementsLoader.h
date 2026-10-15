#pragma once

#include <string>

#include "map/MapRequirements.h"

namespace stardew::maps {

// Carga reglas de validacion desde TOML.
MapRequirements loadMapRequirements(const std::string& path);

}  // namespace stardew::maps