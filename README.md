# Proyecto: Algoritmo en GPU para generación de arte ASCII

Integrantes: Marcelo Chuang, Oscar Garrido

## Requisitos

- Compilador C++
- CMake 3.24 o superior
- OpenCL 3.0 o superior

(Probado con GCC 16.1.1 bajo Linux)

## Compilacion:

```sh
cmake -B build -DCMAKE_EXPORT_COMPILE_COMMANDS=YES
cmake --build build
```

## Ejecucion

```sh
build/ascii -h

# Ejemplos
build/ascii assets/cat.png -W 4 -H 4 -r CPU
build/ascii assets/cat.png -W 4 -H 4 -r OpenCL

build/ascii assets/dog.png -W 4 -H 4 -r CPU
build/ascii assets/dog.png -W 4 -H 4 -r OpenCL

build/ascii assets/landscape.png -W 8 -H 8 -r CPU
build/ascii assets/landscape.png -W 8 -H 8 -r OpenCL
```
