# Virtual Koç University Campus Tour

COMP410 Computer Graphics — Spring 2026
Team: Alperen Kars & Uğur Öner

An interactive 3D virtual tour of Koç University campus built with C++ and OpenGL.

## Controls

| Key | Action |
|-----|--------|
| W/A/S/D | Move forward/left/back/right |
| Mouse | Look around |
| Left Shift | Sprint |
| Left Alt | Turbo sprint |
| F | Toggle fly mode |
| Space / C | Move up/down in fly mode |
| 1 / 2 / 3 | Jump to Rectorate / Library / Student Center demo views |
| ESC | Quit |

## Build & Run

```bash
# Install dependencies (macOS)
brew install glm glfw glew assimp

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

## Runtime Asset Pipeline

- Campus/building scenes are imported at runtime from exported mesh files (OBJ/glTF/GLB) via Assimp.
- Diffuse textures are loaded at runtime with stb_image.
- Lighting stays in our GLSL pipeline (ambient + diffuse + Blinn-Phong specular) and textures modulate the lit result.
