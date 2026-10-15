#pragma once

#include <string>

#include "map/MapDefinition.h"

namespace stardew::maps {

// Interfaz de almacenamiento para desacoplar el servicio del formato de archivo.
class MapRepository {
public:
    virtual ~MapRepository() = default;

    virtual MapDefinition loadMap(const std::string& path) const = 0;
    virtual void saveMap(const std::string& path, const MapDefinition& map) const = 0;
};

// Adaptador concreto que persiste mapas usando el formato TOML.
class TomlMapRepository final : public MapRepository {
public:
    MapDefinition loadMap(const std::string& path) const override;
    void saveMap(const std::string& path, const MapDefinition& map) const override;
};

}  // namespace stardew::maps