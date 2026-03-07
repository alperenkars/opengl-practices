# KU Campus Tour — CLAUDE.md

## Project
Virtual Koç University Campus Tour — COMP410 Computer Graphics, Spring 2026
Team: Alperen Kars & Uğur Öner
Instructor: Yücel Yemez

## Tech Stack
- C++17, OpenGL (shader-based / core profile)
- GLSL shaders (vertex + fragment)
- Angel.h utilities (from Angel & Shreiner textbook)
- GLFW for windowing and input
- GLEW for OpenGL extension loading

## Build
```bash
# macOS
g++ -std=c++17 -o campus src/main.cpp src/*.cpp utilities/InitShader.cpp \
    -I utilities/include \
    -framework OpenGL -lGLFW -lGLEW

# Or use make:
make
```

## Run
```bash
./campus
```

## Project Structure
```
src/            # C++ source files
shaders/        # GLSL vertex and fragment shaders
assets/
  textures/     # PNG/JPG texture files
  models/       # OBJ model files (if used)
utilities/      # Angel.h, InitShader.cpp (from textbook)
docs/           # Proposal, report, notes
```

## Conventions
- Use Angel.h types: `vec4`, `mat4`, `Angel::mat4` for all math
- Shader files: `shaders/vshader.glsl`, `shaders/fshader.glsl`
- All OpenGL objects (VAO, VBO, shaders) initialized in `init()`, drawn in `display()`
- Camera uses first-person view matrix: `LookAt(eye, at, up)` from Angel.h
- Coordinate system: Y-up, 1 unit ≈ 1 meter

## Milestones
- March 9: Proposal submitted
- Late April: Progress presentation (basic scene + navigation + lighting)
- End of semester: Final (full campus, textures, Phong shading)

## Key Constraints (from syllabus)
- Must use C/C++ and OpenGL only — no Unity, no other rendering APIs
- Software implementation is mandatory
