# Stardew Map Editor

Proyecto independiente para el editor de mapas del trabajo final. Actualmente muestra el menu inicial; las opciones de crear y cargar mapas son solo puntos de entrada y todavia no implementan esas operaciones.

## Estructura

- `src/ui/components/`: componentes reutilizables de interfaz.
- `src/ui/screens/`: pantallas de la aplicacion.
- `src/ui/theme/`: estilos compartidos.
- `src/main.cpp`: inicializacion y composicion de la ventana.
- `src/map/`: modelos tipados del mapa y reglas de validacion.
- `src/map/validation/`: validacion pura del modelo.
- `src/map/persistence/`: lectura y escritura TOML y repositorio concreto.
- `src/map/service/`: servicios que coordinan repositorio y validacion.

## Mapas compartidos

Los mapas se compartiran entre este editor y el juego. La ubicacion prevista es una carpeta `maps/` al mismo nivel que ambos proyectos. La capa de persistencia resolvera esa ruta para que no dependa del directorio desde el que se ejecute cada programa. Los requisitos del mapa estan en `config/map_requirements.toml`, separados de la implementacion del validador.

### Formato del mapa (version 1)

Cada mundo se guarda en un TOML independiente. El archivo contiene areas con coordenadas locales, elementos colocados y transiciones entre areas. `width` y `height` se expresan en celdas, no en pixeles. El terreno usa IDs convertidos a `TileType`, y los elementos usan `ElementType` y `ElementLayer`; el editor decide como representarlos visualmente.

El siguiente fragmento muestra el formato, no un mundo completo valido:

```toml
format_version = 1
name = "Valley"

[[areas]]
id = "farm"
type = "farm"
width = 2
height = 2
tiles = [["grass", "grass"], ["path", "grass"]]

[[areas]]
id = "dungeon-1"
type = "dungeon_floor"
floor = 1
width = 2
height = 2
tiles = [["stone", "stone"], ["stone", "stone"]]

[[elements]]
id = "farm-cabin"
area = "farm"
type = "cabin"
layer = "structure"
x = 0
y = 0
width = 1
height = 1

[[elements]]
id = "farm-gate"
area = "farm"
type = "door"
layer = "structure"
x = 1
y = 0
width = 1
height = 1

[[transitions]]
id = "farm-to-village"
from_area = "farm"
trigger_element = "farm-gate"
to_area = "village"
target_x = 0
target_y = 0
```

Tipos de area permitidos: `farm`, `village` y `dungeon_floor`. Tipos de terreno: `grass`, `dirt`, `tilled_soil`, `water`, `path`, `stone`, `cave_floor`, `cave_wall`, `wood_floor` y `sand`. Capas de elementos: `structure`, `object` y `npc`. `x` e `y` señalan la celda superior izquierda; `width` y `height` indican cuantas celdas ocupa el elemento. Una transicion identifica el elemento que la activa, el area de destino y la celda donde aparece el jugador.

El parser de mapas y el serializador estan en `src/map/persistence/MapFile.*`. Los nombres desconocidos de tiles, elementos, areas o capas se rechazan. Los campos semanticos omitidos como `format_version`, el tipo de area o el tipo/capa de un elemento se mantienen ausentes para que el validador produzca el diagnostico correspondiente.

`config/map_requirements.toml` contiene los requisitos del juego: cantidad de cada tipo de area, pisos obligatorios, elementos requeridos y rutas entre areas. Las reglas actuales conectan granja y pueblo, pueblo y primer piso, cada piso con la salida al pueblo y los pisos consecutivos. El validador conserva en codigo reglas estructurales como IDs unicos, grillas rectangulares, referencias validas, puertas de transicion y posiciones dentro de los limites; las reglas configurables de contenido y destino no estan hardcodeadas en el.

En `examples/maps/` hay dos mapas de prueba: `basic_valid.toml` cumple las reglas actuales y `basic_missing.toml` es sintacticamente valido pero tiene areas, elementos y rutas faltantes. `make test` los carga desde archivo, valida ambos y comprueba sus resultados.

## Requisitos

En Ubuntu, instala un compilador de C++, Make, `pkg-config` y Qt 6 Widgets:

```sh
sudo apt install build-essential pkg-config qt6-base-dev libtomlplusplus-dev
```

## Compilar y ejecutar

Desde esta carpeta:

```sh
make
make run
```

Para correr las pruebas del validador del mapa:

```sh
make test
```

Para borrar el ejecutable generado:

```sh
make clean
```
