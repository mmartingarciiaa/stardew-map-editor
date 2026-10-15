#pragma once

#include <string>

#include "map/MapDefinition.h"

namespace stardew::maps {

// Parsea un mapa en formato TOML; los enums desconocidos y los campos estructurales invalidos lanzan error.
MapDefinition loadTomlMapDefinition(const std::string& path);

// Serializa el modelo tipado a TOML y lanza error si no puede abrir o escribir el archivo.
void saveTomlMapDefinition(const std::string& path, const MapDefinition& map);

}  // namespace stardew::maps