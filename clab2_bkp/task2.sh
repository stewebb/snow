#!/bin/bash
cd build/
Shaders="texture normal blinn-phong bump"
Shadings="Flat Gouraud Phong"
for shader in $Shaders; do
    for shading in $Shadings; do
        echo "Running task2 with shader: $shader and shading: $shading"
        ./task2 $shader $shading "${shader}_${shading}.png"
    done
done
