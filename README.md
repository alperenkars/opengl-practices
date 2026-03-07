# Virtual Koç University Campus Tour

COMP410 Computer Graphics — Spring 2026
Team: Alperen Kars & Uğur Öner

An interactive 3D virtual tour of Koç University campus built with C++ and OpenGL.

## Controls

| Key | Action |
|-----|--------|
| W/A/S/D | Move forward/left/back/right |
| Mouse | Look around |
| ESC | Quit |

## Build & Run

```bash
# Install dependencies (macOS)
brew install glfw glew

# Build
make

# Run
./campus
```

## Project Structure

```
src/          C++ source files
shaders/      GLSL vertex and fragment shaders
assets/       Textures and 3D models
utilities/    Angel.h OpenGL helper library
docs/         Proposal and reports
```
