# Dynamic Environment Simulation with Acculumated Snow Effect

## About the folder structure
- **common** contains some some common C++ classes and methods.
- **data** contains the environment data simulator, which is a Python script, and a generated data file.
- **external** contains some external C/C++ libraries.
- **models** contains required 3D model and relavent texture map.
- **external** contains all required GLSL shaders.

## How to run
1. `sudo apt install cmake make g++ libx11-dev libxi-dev libgl1-mesa-dev libglu1-mesa-dev libxrandr-dev libxext-dev libxcursor-dev libxinerama-dev libxi-dev libopencv-dev`
2. **Config** the project in CMake.
3. **Run** the project in CMake.
4. You should see two windows, one for OpenGL rendering and another for OpenCV capturing with statistics infotmation.

## Reference
The project code is based on [OpenGL Tutorial 16 Shadow Mapping](http://www.opengl-tutorial.org/intermediate-tutorials/tutorial-16-shadow-mapping/), [GitHub Repository](https://github.com/opengl-tutorials/ogl/tree/master/tutorial16_shadowmaps)