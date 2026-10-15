# Stardew Map Editor

Proyecto independiente para el editor de mapas del trabajo final. Actualmente muestra el menu inicial; las opciones de crear y cargar mapas son solo puntos de entrada y todavia no implementan esas operaciones.

## Estructura

- `src/ui/components/`: componentes reutilizables de interfaz.
- `src/ui/screens/`: pantallas de la aplicacion.
- `src/ui/theme/`: estilos compartidos.
- `src/main.cpp`: inicializacion y composicion de la ventana.

## Mapas compartidos

Los mapas se compartiran entre este editor y el juego. La ubicacion prevista es una carpeta `maps/` al mismo nivel que ambos proyectos. La capa de persistencia resolvera esa ruta para que no dependa del directorio desde el que se ejecute cada programa.

## Requisitos

En Ubuntu, instala un compilador de C++, Make, `pkg-config` y Qt 6 Widgets:

```sh
sudo apt install build-essential pkg-config qt6-base-dev
```

## Compilar y ejecutar

Desde esta carpeta:

```sh
make
make run
```

Para borrar el ejecutable generado:

```sh
make clean
```
