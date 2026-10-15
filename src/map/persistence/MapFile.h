#pragma once

#include <string>

#include "map/MapDefinition.h"

namespace stardew::maps {

// Parsea un mapa TOML; los enums desconocidos y los campos estructurales invalidos lanzan error.
MapDefinition loadMapDefinition(const std::string& path);

// Serializa el modelo tipado a TOML y lanza error si no puede abrir o escribir el archivo.
void saveMapDefinition(const std::string& path, const MapDefinition& map);

}  // namespace stardew::maps